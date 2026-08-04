# Post-Processing Pipeline

How a frame flows through the post-processor, what each pass costs, and the contracts
that keep the chain correct. Read this before touching `PostProcessor`, `GrabPass`,
`PostProcessStack`, any `Effects/Framebuffer/*` effect, or the swap-chain render passes.

## 1. Two effect families — only one is multi-pass

| Family | Base class | Execution | Cost model |
|--------|-----------|-----------|------------|
| **Lens (direct)** | `DirectPostProcessEffect` | The Saphir `PostProcessing` generator compiles the stack's DISPLAY effects + the camera's lens list into **ONE fragment shader**, drawn as a fullscreen quad in the final swap-chain pass. | One pass total, whatever the list size. |
| **Display (direct)** | `DirectPostProcessEffect`, added via `PostProcessStack::addDisplayEffect()` | `Effects::Display::{Sharpen, FXAA, FXAASharpen}` — display-referred single-pass effects folded into the final pass, BEFORE the camera lens effects (grain/scanlines must not be sharpened). At most ONE fetch-overriding effect (FXAA*) per stack. | Zero passes, zero render targets (phase B — they replaced the retired single-pass `Framebuffer` versions). |
| **Framebuffer (indirect)** | `IndirectPostProcessEffect` | Each effect owns its render targets, render passes and pipelines. The chain executes sequentially through `PostProcessor::executeIndirectPostProcessEffects()`. OVERLAY effects (see §3b) skip their own apply pass. | 1 to ~22 GPU passes **per effect** (see §4). This is where frame time goes. |

Ownership: the Scene owns the `PostProcessStack` (indirect chain + display list); the
Camera owns the lens list and *declares* the photographic effects (`enableHDR`/
`enableBloom`/`enableDepthOfField`/`enableMotionBlur`) that
`PostProcessStack::syncCameraEffects()` materializes each frame in canonical order
(DoF → MotionBlur → Bloom → ToneMapping), inserted before the first
`runsAfterToneMapping()` effect. `syncCameraEffects()` also PAIRS the camera Bloom with
the camera ToneMapping (phase C): the tone mapping samples `Bloom::bloomTexture()` and
adds the glare in its own pass (`setBloomSource()`, EM_BLOOM shader variant baked at
create), while the bloom bypasses its full-res composite (`setCompositeBypassed()`) —
a standalone Bloom without tone mapping downstream keeps compositing itself. A bloom
(de)materialization rebuilds the tone mapping (pipeline variant change).

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

## 3b. The overlay protocol and the combine pass (phase A)

The nine "screen overlay" effects — RTR/SSR, RTAO/SSAO, RTGI/SSGI, ContactShadows,
VolumetricLight, LensFlare — no longer own a full-resolution apply/composite pass.
They implement the OVERLAY protocol of `IndirectPostProcessEffect`:

- `producesOverlay()` → true; the PostProcessor never calls their `execute()`.
- `recordOverlayPasses()` records their internal passes (trace, pyramid, blur…) only.
- `combineContribution()` returns what the shared **CombinePass** needs: a GLSL
  identifier `prefix`, the effect's own textures (`<prefix><Suffix>` samplers), the
  shared context samplers it reads (`emDepth`/`emNormals`/`emMaterialProps`/`emAlbedo`),
  up to two `vec4` per-frame scalar slots (`emDyn.<prefix>Dynamics0/1`), and the GLSL
  snippet applying its result onto the running `em_Color` — the EXACT math of its
  retired apply shader.

`PostProcessor::executeIndirectPostProcessEffects()` groups CONTIGUOUS overlay effects
and applies each group in ONE generated full-res pass (`Graphics/CombinePass`, shader +
pipeline cached per group signature, two ping-pong output targets). Sequential
exactness is preserved by the **flush rule**: the group is flushed (combine emitted)
before any non-overlay effect, and before any overlay effect whose UPSTREAM passes
sample the chain color — `readsChainColorUpstream(context)`: SSGI (trace gather), SSR
(color pyramid + resolve), LensFlare (threshold), RTGI only when the albedo G-buffer
is missing (trace fallback binding).

Real chains: `[RTR,RTAO,RTGI,CS,VL]` → 1 combine (5 full-res applies → 1);
SSR variant `[SSR,SSAO] | [SSGI,VL]` → 2 combines; LightAndShadowDebug
`[RTR,RTAO,RTGI,CS] | fog | [VL] | [LF]` → fog and the flush before LensFlare split
the groups. Net effect: the ~17 per-effect full-res RGBA16F output targets are gone,
replaced by the two shared combine targets.

## 3c. The shared denoise pass (phase E)

The seven overlay effects with a "trace → separable blur H → blur V" working chain
(RTR/SSR, RTAO/SSAO, RTGI/SSGI, ContactShadows) also delegate the blur pair to the
PostProcessor's **DenoisePass**: per combine group, ONE horizontal + ONE vertical
multi-render-target pass replace two passes per effect. The protocol mirrors §3b:

- `usesSharedDenoise()` → true; `recordPreDenoisePasses()` records the trace only;
  `denoiseContribution()` provides the source (trace output), the effect-owned H/V
  targets, the guides (`emDepth`/`emNormals`), one `vec4` dynamics slot and the GLSL
  kernel snippet (assigns `vec4 <prefix>Result`; direction comes as `emDenoiseDir`).
  Each effect keeps its EXACT kernel — the bilateral weights, radii and early-outs of
  the retired per-effect blur shaders are transcribed verbatim, prefixed.
- `recordPostDenoisePasses()` hosts what ran AFTER the blur: the RTGI temporal resolve
  + normal-history copy (and its ping-pong flip / combine-source selection).
- Group execution order: every member's pre-denoise passes → DenoisePass H → V →
  every member's post-denoise passes → CombinePass.
- The group is PARTITIONED BY EXTENT (a pixel-doubling-gated effect can coexist with an
  unconditionally half-res one): one H+V pair per resolution present in the group.
- The MRT render pass allows mixed attachment formats (R8/RG16F/RGBA16F), same
  conventions as the IntermediateRenderTarget pass (DONT_CARE load — zero clear values,
  the `CommandBuffer::beginRenderPass()` template now accepts an empty array — STORE,
  ends SHADER_READ_ONLY, FULL non-by-region dependencies).
- VolumetricLight (radial march) and LensFlare (ghosts) keep their own non-separable
  passes.

ContactShadows also moved to the SAME pixel-doubling-gated working resolution as RTAO
(it was the only full-res mask chain) — the assumed quality trade of this phase; its
combine snippet upsamples bilinearly like every other overlay.

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
  Removes ~28 redundant barrier calls and one full swap-chain clear pass per frame.
- **Phase B (done)** — the post-tonemap LDR effects became DISPLAY effects
  (`Effects::Display::*`, §1) compiled into the final pass: −1 to −2 full-res passes
  and render targets per frame. The single-pass `Framebuffer` versions were retired.
- **Phase C (done)** — the camera ToneMapping applies the bloom itself (§1 pairing):
  −1 full-res pass on EVERY scene, including the Bloom+ToneMapping floor.
- **Phase A (done)** — the overlay protocol + combine pass (§3b): the nine overlay
  effects lost their apply/composite passes; worst realistic chains save 3-4 full-res
  passes and ~15 full-res RGBA16F render targets.
- **Phase E (done)** — the shared denoise pass (§3c): the seven separable-blur effects
  delegate their blur pairs (up to 8 passes → 2 per group), and ContactShadows moved to
  the pixel-doubling-gated half resolution (the §4 anomaly). NOTE: the phase B+A Linux
  measurement showed NO FPS gain on Sponza RT — the bottleneck there is the RT TRACE
  passes, which none of these phases touch. E was executed as architectural cleanup with
  that expectation on record; the next real lever for Sponza-class scenes is trace cost
  (resolution, sample counts), to be driven by per-pass GPU timings.

## Known issues

- `Core.RendererService.screenshot()` triggers `UNASSIGNED-non-acquired-swapchain-image-used`
  (the capture path transitions a presentable image outside its acquire window).
  Pre-existing, only fires on capture, cosmetic for the capture itself — to fix in the
  `capture()` path.
