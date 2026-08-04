# Post-Processing Pipeline

How a frame flows through the post-processor, what each pass costs, and the contracts
that keep the chain correct. Read this before touching `PostProcessor`, `GrabPass`,
`PostProcessStack`, any `Effects/Framebuffer/*` effect, or the swap-chain render passes.

## 1. Two effect families — only one is multi-pass

| Family | Base class | Execution | Cost model |
|--------|-----------|-----------|------------|
| **Lens (direct)** | `DirectPostProcessEffect` | The Saphir `PostProcessing` generator compiles the camera's whole effect list into **ONE fragment shader**, drawn as a fullscreen quad in the final swap-chain pass. | One pass total, whatever the list size. Nothing to optimize here. |
| **Framebuffer (indirect)** | `IndirectPostProcessEffect` | Each effect owns its render targets, render passes and pipelines. The chain executes sequentially — each `execute()` receives the previous effect's output texture (`PostProcessor::executeIndirectPostProcessEffects()`). | 1 to ~22 GPU passes **per effect** (see §4). This is where frame time goes. |

Ownership: the Scene owns the `PostProcessStack` (indirect chain); the Camera owns the
lens list and *declares* the photographic effects (`enableHDR`/`enableBloom`/
`enableDepthOfField`/`enableMotionBlur`) that `PostProcessStack::syncCameraEffects()`
materializes each frame in canonical order (DoF → MotionBlur → Bloom → ToneMapping),
inserted before the first `runsAfterToneMapping()` effect (FXAA/Sharpen/FXAASharpen).

> A camera with `enableHDR`+`enableBloom` (the `Player` baseline in consuming projects)
> forces a stack even on a scene that declares none: the runtime floor is
> **Bloom + ToneMapping ≈ 18 passes**.

## 2. Frame structure (internal-target path)

When post-processing is active (`Renderer::needsInternalTarget()`), the frame is:

1. **Shadow maps** (per shadow-casting light).
2. **RP-scene (CLEAR)** — MRT into the internal scene target: color (+ normals,
   material properties, albedo, velocity as required) + depth.
3. *(TranslucentGB only)* refraction `GrabPass::recordBlit()` + **RP-scene-load** pass.
4. **`PostProcessor::recordBlit()`** — copies the full G-buffer into the
   post-processor's own grab pass (§3).
5. **Indirect effect chain** — each enabled effect runs its own passes, chained.
6. **RP-final (swap-chain offscreen-composite)** — fullscreen PP quad (all lens
   effects in one shader) + editor gizmos + overlay, then PRESENT.

The direct path (no internal target) differs at step 2 (scene renders straight into the
swap chain) and step 6 (the `postProcess` LOAD pass restarts around the blit instead).

### The offscreen-composite swap-chain pass

The swap chain owns **three** render passes (`SwapChain.cpp`):

| Pass | Load ops | Initial layouts | Used by |
|------|----------|-----------------|---------|
| `createRenderPass()` (main) | CLEAR/CLEAR | UNDEFINED | Direct path RP1 (scene draws) |
| `createPostProcessRenderPass()` | LOAD/LOAD | ATTACHMENT | Direct path RP2 (content must survive the mid-frame blit) |
| `createOffscreenCompositeRenderPass()` | CLEAR color / CLEAR depth (store DONT_CARE) | UNDEFINED | Internal-target path RP-final |

The offscreen-composite pass exists so the internal-target path does **not** need the
former *empty layout-establishing pass* (a full-screen CLEAR with zero draw calls whose
only job was transitioning the acquired image out of UNDEFINED before a LOAD pass).
One pass now does transition + clear + composite + present.

> [!CAUTION]
> **Render pass compatibility includes subpass dependencies.** The composite pass reuses
> the pipelines created against the `postProcess` pass (PP quad, gizmos, overlay). The
> Vulkan compatibility rules exempt **only** load/store ops and image layouts — attachment
> formats/samples AND the **subpass dependency list must be identical**, or every draw
> triggers `VUID-vkCmdDrawIndexed-renderPass-02684` ("dependencyCount is incompatible").
> The composite pass therefore carries a verbatim copy of the two `postProcess`
> dependencies; its acquire-semaphore ordering need is covered by the EXTERNAL→0
> dependency's `COLOR_ATTACHMENT_OUTPUT` source stage (chains with the
> `vkAcquireNextImageKHR` wait — see `createRenderPass()` for the hazard rationale).

## 3. The grab pass — batched barrier contract

`PostProcessor::recordBlit()` transfers up to six full-resolution images (color via
blit when HDR, plus depth/normals/materialProperties/albedo/velocity copies) from the
scene target into sample-able textures. `GrabPass::recordBlit()` (refraction) does the
same for up to four.

Both are structured as exactly **three command groups** — do not regress this:

1. **One batched `pipelineBarrier()`**: every source → `TRANSFER_SRC`, every
   destination → `TRANSFER_DST` (single call, one `VkImageMemoryBarrier` per image).
2. **All copies back-to-back** — independent transfers need no barriers between them.
3. **One batched `pipelineBarrier()`**: destinations → `SHADER_READ_ONLY`, sources
   restored to their attachment layouts.

The stage masks of each batched call are the **union** of the per-image stages
(`COLOR_ATTACHMENT_OUTPUT | FRAGMENT_SHADER`, plus `LATE_FRAGMENT_TESTS`/
`EARLY_FRAGMENT_TESTS` when depth participates); per-image precision is preserved by
the access masks inside each `VkImageMemoryBarrier`. The previous shape — one
`pipelineBarrier()` per transition — serialized the GPU up to ~30 times per frame for
the exact same correctness.

## 4. Per-effect GPU pass counts (reference, 1080p half-res)

Measured from the `execute()` implementations (render passes + compute dispatches):

| Effect | Passes/frame | Notes |
|--------|-------------|-------|
| SSR | ~22 | Hi-Z pyramid (~11 dispatches) + pre-convolved color pyramid (~8) + trace/resolve/blur H/V/composite |
| Bloom | 10 (fixed) | 5 down + 4 up + composite; `MipLevels = 5` constexpr |
| RTR | ~10 | trace + compute pyramid (~6 mips) + blur H/V + composite |
| ToneMapping (auto-exposure) | ~8 + CPU readback | log-luminance chain to 1×1 + adaptation + tonemap; 1 pass in manual mode |
| DepthOfField | 4–7 + CPU readback | autofocus 1×1 + CoC setup + optional near-field dilate/gather + far gather + composite |
| RTGI | 4–6 | + temporal resolve + normal-history copy when enabled |
| RTAO / SSAO / SSGI / MotionBlur / ContactShadows | 4 each | trace/extract + blur/tile pair + apply. MotionBlur early-outs to 0 without velocity/shutter. |
| VolumetricLight / LensFlare | 3 each | extract + blur/ghosts + composite |
| TAA / FXAA / Sharpen / FXAASharpen / AtmosphericFog | 1 each | |

Worst realistic chain (RT + TAA + player DoF/MotionBlur):
RTR → RTAO → RTGI → ContactShadows → VolumetricLight → TAA → DoF → MotionBlur → Bloom
→ ToneMapping → Sharpen ≈ **58 passes** (SSR variant ≈ 64).

ContactShadows is the only mask effect working in **full resolution** (4× RGBA16F) —
a known anomaly to revisit.

## 5. Optimization roadmap (owner-approved, 2026-08)

Executed in phases; each phase is measured (RenderDoc), committed, then Linux-tested
before the next:

- **Phase D (done)** — batched grab-pass barriers (§3) + offscreen-composite pass (§2).
  No pass-count change inside effects; removes ~28 redundant barrier calls and one full
  swap-chain clear pass per frame.
- **Phase B+A (next)** — B: fold the post-tonemap LDR effects (Sharpen/FXAA/FXAASharpen)
  into the final direct uber-shader (`overrideFragmentFetching()` exists for this).
  A: merge the per-effect apply/composite passes (RTR/RTAO/RTGI/ContactShadows/
  VolumetricLight/LensFlare each pay a full-res pass) into a single *GBufferCombine*
  pass — requires extending the `IndirectPostProcessEffect` contract so effects can
  return an overlay (buffer + composition mode) instead of writing color themselves.
  C (attached here): sample the bloom chain directly from the ToneMapping pass instead
  of a separate composite.
- **Phase E (if measurements justify)** — shared MRT bilateral denoise for AO/GI/contact
  shadows (same depth/normals guides).

## Known issues

- `Core.RendererService.screenshot()` triggers `UNASSIGNED-non-acquired-swapchain-image-used`
  (the capture path transitions a presentable image outside its acquire window).
  Pre-existing, only fires on capture, cosmetic for the capture itself — to fix in the
  `capture()` path.
