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

### The frame, counted (RenderDoc capture, `sponza`, 2026-09-09)

Read from the capture's STRUCTURED DATA (the serialised command stream — no GPU replay, see the
`renderdoc-capture` command in the consuming project for why replaying a multi-GB capture
segfaults). X11, 1920×1080; draw counts are resolution-independent, timings are not.

**51 render passes, 1082 draws, 2 queue submits:**

| Block | Passes | Draws |
|---|---|---|
| Shadow map (its own submit) | 1 | 454 |
| `ScenePass` (multi-pass forward, MRT G-buffer) | 1 | 579 |
| Post-process chain + final composite | **49** | **1 each** |

Plus 59 `vkCmdPipelineBarrier`, 6 `vkCmdDispatch`, 1 `vkCmdBuildAccelerationStructuresKHR`, and 2
`vkCmdCopyImageToBuffer` (the tone mapping's auto-exposure and the DoF autofocus CPU readbacks).

Three things this settles:

- **The grab pass reads exactly as § 3 specifies.** Immediately after the scene pass, outside any
  render pass: `[barrier] vkCmdBlitImage + vkCmdCopyImage ×5 [barrier]`. The batched-barrier
  contract holds (two barriers, not twelve), and it is a direct view of the **five copies nothing
  overwrites afterwards** — the redundancy § 4b prices at ~0.5 ms.
- **The chain is one render pass per fullscreen draw**, 49 of them. ONE of those draws is the RTGI
  trace at ~40 ms; the other 48 share ~13 ms.
- ⚠️ **The multi-pass forward is NOT the ceiling on this scene.** 579 scene draws against 454
  shadow casters is a ratio of 1.28 — a single light. The `objects × (1 + lights)` cost only bites
  with several lights; here the scene pass is 8.6 ms and 13 % of the frame. Do not cite Sponza as
  evidence for or against a clustered-forward rewrite.

⚠️ **RenderDoc attributes per DRAW and cannot split a fullscreen pass.** It is the right tool for
the pass/draw/barrier census above, and the wrong one for the 40 ms inside a single trace draw —
that needs the engine's own profiler scopes (§ 4b) or intra-draw hardware counters (Nsight).

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

Real chains, in the Sep 2026 slot order (`Graphics/EffectSlot.hpp`):
`[CS,RTGI,RTAO,VL] | … | [LF]` → **2 combines**, measured on `sponza` (GPU profiler:
786 `Combine` samples over 393 frames). The group is closed by `TemporalAA`, which is not an
overlay; `LensFlare` opens the second one after `MotionBlur`. With `AtmosphericFog` enabled the
count is unchanged — `[CS,RTGI,RTAO,VL] | fog | … | [LF]` — where the previous order produced
**three** (`[RTGI,RTAO,CS] | fog | [VL] | [LF]`): moving `VolumetricLight` to the fog's upstream
side merged it into the indirect group. One generated full-res pass less on any scene with fog.

Net effect of the phase: the ~17 per-effect full-res RGBA16F output targets are gone, replaced by
the two shared combine targets. ⚠️ **Measured cost of what remains: `Combine` 0.138 ms and
`SharedDenoise` 0.118 ms per frame on `sponza` at 2880×1620** — 0.4 % of a 63.9 ms frame. Pass
merging is DONE as a lever; there is nothing left to win here (see § 4).

## 3c. The shared denoise pass (phase E)

The **five** overlay effects with a "trace → separable blur H → blur V" working chain
(RTR, SSR, RTAO, SSAO, ContactShadows) delegate the blur pair to the PostProcessor's
**DenoisePass**: per combine group, ONE horizontal + ONE vertical multi-render-target pass
replace two passes per effect. The protocol mirrors §3b:

> [!CAUTION]
> ⚠️ **This paragraph said SEVEN and listed `RTGI/SSGI` among them until Sep 2026. That was
> false.** Neither declares `usesSharedDenoise()`, so both inherit the base `false`
> (`IndirectPostProcessEffect.hpp:524`) and record their whole chain through
> `recordOverlayPasses()`. They cannot delegate: their denoiser is `GIDenoiser`, a temporal
> resolve plus a multi-iteration à-trous, not a separable bilateral blur — a different algorithm,
> not a missing wiring. The GPU profiler is what exposes it: RTAO and ContactShadows appear as
> `…/trace` + `…/temporal` around a shared `SharedDenoise` scope, while RTGI appears as a single
> opaque `RTGIEffect/internal`. **Do not "fix" RTGI by making it answer true.**

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

> [!NOTE]
> This table's ContactShadows row used to be followed by "the only mask effect working in
> **full resolution** (4× RGBA16F) — a known anomaly to revisit". **Phase E closed that
> anomaly** (§3c and the phase list below): ContactShadows shares RTAO's
> pixel-doubling-gated half resolution. The sentence was left behind and contradicted §3c
> for two releases — removed Sep 2026.

## 4b. MEASURED per-pass GPU cost (`sponza`, 2026-09-09)

Pass COUNTS (§ 4) say nothing about where the time goes. These are GPU timestamps from the
engine's own profiler (`Core/Graphics/GPUProfiler/Enabled`), harvested behind the in-flight
fence, on `sponza` at the default spawn pose, **2880×1620**, RTX 3070 Ti, validation layers ON,
0 VUID. Full chain: ContactShadows + RTGI + RTR + RTAO + VolumetricLight + AtmosphericFog +
LensFlare + TAA + Sharpen, camera HDR/Glare/DoF/MotionBlur. Captured through
`tools/demo-capture-bench.py --gpu-timings` after convergence.

| Scope | avg (ms) | share of frame |
|---|---|---|
| **Frame** | **62.86** | 100 % (≈ 15.9 fps) |
| TLASBuild | 0.11 | 0.2 % |
| ScenePass (raster, MRT G-buffer, multi-pass forward) | 8.63 | 13.7 % |
| **PostFXChain** | **53.53** | **85.2 %** |
| ↳ `RTGIEffect/internal` (trace + GIDenoiser, occlusion lane included) | 43.91 | **69.9 %** |
| &nbsp;&nbsp;&nbsp;↳ `RTGIEffect/trace` | **39.80** | **63.3 %** |
| &nbsp;&nbsp;&nbsp;↳ `GIDenoiser/atrous` (4 iterations) | 3.63 | 5.8 % |
| &nbsp;&nbsp;&nbsp;↳ `GIDenoiser/temporal` + `/moments` + `/normalHistory` | 0.22 + 0.19 + 0.08 | 0.8 % |
| ↳ `RTAOEffect/trace` (DERIVED from the lane) | **0.06** | 0.1 % |
| ↳ `RTREffect/trace` | 3.79 | 6.0 % |
| ↳ `ContactShadowsEffect/trace` | 0.80 | 1.3 % |
| ↳ `DepthOfFieldEffect` | 0.85 | 1.3 % |
| ↳ `SharedDenoise` + `Combine` (phases A+E) | 0.69 + 0.66 | 2.1 % |
| ↳ `VolumetricLightEffect` / `LensFlareEffect` | 0.47 / 0.09 | 0.8 % |
| ↳ `TAAEffect` | 0.31 | 0.4 % |
| ↳ `BloomEffect` / `ToneMappingEffect` | 0.17 / 0.15 | 0.5 % |
| ↳ `AtmosphericFogEffect` | **0.00** | — (no-op, see below) |
| FinalComposite (PP quad + display + lens + overlay) | 0.08 | 0.1 % |
| *unaccounted = the un-scoped `PostProcessor::recordBlit`* | **0.52** | 0.8 % |

Three readings worth keeping:

- **`PostProcessor::recordBlit()` is the whole 0.49 ms residual** (nothing else in the frame is
  un-scoped, and `sponza` has no TranslucentGB object, so the material `GrabPass` never runs).
  That is the measured price of copying the six full-resolution G-buffer images into the grab
  pass (0.49 ms on the same scene without the fog effect declared) — the ONE structural saving available outside ray cost, and it is 0.7 % of the frame.
  Five of those six copies are redundant on the internal-target path (nothing writes depth,
  normals, material properties, albedo or velocity after the TranslucentGB pass, so a layout
  transition would do); only the colour needs a real copy, for its mip chain and for the
  pre-translucency write-back hazard. **Size the work accordingly: it is worth ~0.4 ms.**
- **`Combine` and `SharedDenoise` rose from 0.14/0.12 to 0.66/0.66 when RTR joined the group** —
  RTR contributes the expensive combine snippet (24-tap Vogel disk gather over the pyramid) and
  a third denoise entry. Still 1.9 % of the frame: merged passes remain cheap.
- **2 combine groups**, measured (650 samples over 325 frames) **with the fog effect present** —
  the count the § 3b reorder predicts, against **3** under the pre-Sep-2026 order, where the fog
  sat between the indirect terms and `VolumetricLight` and split the group in two.

> [!CAUTION]
> **THE WHOLE `Fog` SLOT IS DEAD ON THIS SCENE, and it is instrumented as such: 0.000 ms over
> 300+ frames for whichever occupant holds it.** `AtmosphericFog` and `VolumetricScattering`
> share the slot on purpose and both early-out on the same missing
> `Scene::setParticipatingMedium()`, each emitting one warning; the march additionally needs a
> CSM directional light. A `sponza` run with `VolumetricScattering` added after the fog measured
> `VolumetricScatteringEffect` 0.000 ms and no `AtmosphericFogEffect` scope at all — but swapping
> them back measured `AtmosphericFogEffect` 0.000 ms. **Adding the effect is not what turns fog
> on; declaring the medium is.**
>
> Two distinct structural gaps sit behind that, both **OPEN**:
> - **Selection is "last `addEffect()` wins"**, with no notion of whether the winner can run in
>   this scene. An occupant that early-outs unconditionally still owns the concept and silences
>   a sibling that might have worked. A runtime AVAILABILITY predicate on the effect, consulted
>   by the stack when it picks a slot occupant, is the shape of the fix.
> - **A permanently-inert effect still costs its `requires*()`.** That aggregation ignores
>   `isEnabled()` by contract (so a runtime A/B can switch to a disabled alternative without
>   reallocating), which also means a slot nobody can turn on keeps its G-buffer attachments
>   allocated for the life of the scene target.

**Reproducibility: ×1.002** (two independent runs of the same configuration gave RTGI 43.89 and
43.96 ms). Any difference above ~0.2 ms on this scope is a result, not noise — the GPU-timing
metric is far tighter than the pixel peak-to-peak temporal metric, whose floor on this scene is
saturated by the animated foliage: 8.5 % of pixels move by more than 2/255 between two captures
of the SAME run, and that floor is not monotone in the sample count, so it cannot rank
configurations here. ⚠️ Prefer GPU timestamps over pixel statistics for anything about cost.

### The cost is ray throughput, and nothing else

`RTGIEffect/internal` is **linear in the sample count**, measured by settings A/B
(`Core/Graphics/RayTracing/GlobalIllumination/SampleCount`). ⚠️ This sweep was run on the same
scene and pose but on the chain **before RTR was enabled**; RTGI itself is unaffected by that
(43.96 there against 43.53 in the table above, within 1 %), only the frame totals shift:

| samples | RTGI (ms) | Frame (ms) | fps |
|---|---|---|---|
| 8 (default) | 43.96 | 64.29 | 15.6 |
| 4 | 24.11 | 44.08 | 22.7 |
| 2 | 13.91 | 33.60 | 29.8 |
| 1 | 8.52 | 28.35 | 35.3 |

⇒ **≈ 5.05 ms per sample per frame, plus ≈ 3.5 ms fixed.**

### Directly instrumented (Sep 2026), and it corrected the estimate above

`RTGIEffect/internal` was ONE opaque scope until Sep 2026 — the single most expensive thing the
engine does had no internal attribution, and the split above had to be inferred from settings
A/B. It is now instrumented per pass (`RTGIEffect/trace`, and `GIDenoiser/{temporal, moments,
normalHistory, atrous}`, which nest under whichever GI producer owns the denoiser — SSGI gets the
same four for free):

| Sub-scope | avg (ms) | share of RTGI |
|---|---|---|
| `RTGIEffect/trace` (occlusion lane included) | **39.80** | 90.6 % |
| `GIDenoiser/atrous` (4 iterations) | 3.63 | 8.3 % |
| `GIDenoiser/temporal` | 0.22 | 0.5 % |
| `GIDenoiser/moments` | 0.19 | 0.4 % |
| `GIDenoiser/normalHistory` | 0.08 | 0.2 % |
| **sum of children vs parent** | **43.913 vs 43.914** | — |

**The children close the parent to 0.001 ms**: there is no unattributed pass left inside RTGI.

⚠️ **The direct measurement DISAGREES with the A/B estimate, and the A/B was wrong.** Turning
`Denoiser/Iterations` from 4 to 1 costs 2.23 ms, which extrapolates to 0.74 ms per iteration and
~2.97 ms for four — the instrumented value is **3.63 ms, i.e. 0.906 ms per iteration, 22 %
higher**. The per-iteration cost is not uniform (the footprint doubles at each step, so the
cache behaviour does not), and a linear extrapolation from a 3-iteration delta underestimates it.
**Prefer a scope over an A/B whenever the pass can be scoped.**

So RTGI = **~39.8 ms of trace, ~4.1 ms of denoise** (of which the second bounce is ~0.9 ms,
measured by A/B inside the trace, where no scope can separate it): **91 % of it is rays**.

> [!IMPORTANT]
> **The budget is allocated 91 % to tracing and 9 % to denoising, when it is the denoiser that
> makes a low sample count viable.** On a converged static camera the 8-, 2- and 1-sample images
> are visually indistinguishable on this scene and the GI energy is preserved (frame mean
> luminance 85.95 / 86.73 / 85.70, σ +2.2 % at 1 sample) — the à-trous plus the temporal
> accumulation (`MaxAccumulation` 64) absorb the loss. ⚠️ That comparison is **converged and
> static by construction of the bench**: the sample count is paid on MOTION, when the temporal
> history is rejected. Rebalancing therefore means spending some of the ~35 ms on a stronger
> denoiser/temporal path, not simply lowering the count. **Unmeasured in motion — OPEN.**

> [!IMPORTANT]
> **RTAO used to duplicate rays RTGI had already cast — 7.22 ms, 10.4 % of the frame. FIXED
> (Sep 2026) by the occlusion-LANE protocol**, and this is the measurement that closed it:
> `RTAOEffect/trace` **7.22 → 0.06 ms** (×118), `RTGIEffect/internal` **43.45 → 43.77 ms**
> (inside the 43.45–43.96 envelope of five runs, i.e. the lane costs nothing measurable),
> **frame 69.14 → 62.77 ms (−9.2 %)**, 0 VUID. Both effects trace a cosine-weighted hemisphere
> at the same half resolution with the same sample count, and the GI ray already carries its hit
> distance, so the producer reduces the occlusion in its own loop and publishes it in the ALPHA
> lane of its trace target — which nothing else reads (`GIDenoiser` samples `.rgb` only:
> verified in the shader sources, not assumed from the constant `1.0` the producer used to
> write there).
>
> The derived term is **more accurate than the one it replaces**: RTAO traces with
> `TerminateOnFirstHit`, so its hit distance is *a* hit inside its range, while the GI ray
> commits the CLOSEST hit over the whole sky distance — same occlusion set, exact distance
> weight. Measured direction confirms it: the derived AO is darker on 35.9 % of pixels and
> brighter on 13.1 % (signed mean −0.68/255), i.e. slightly MORE occlusion, which is what a
> larger distance weight produces. Contract and traps: `src/Graphics/AGENTS.md` § "The
> ambient-occlusion LANE".

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
- **Phase E (done)** — the shared denoise pass (§3c): the five separable-blur effects
  delegate their blur pairs (up to 8 passes → 2 per group), and ContactShadows moved to
  the pixel-doubling-gated half resolution (the §4 anomaly). NOTE: the phase B+A Linux
  measurement showed NO FPS gain on Sponza RT — the bottleneck there is the RT TRACE
  passes, which none of these phases touch. E was executed as architectural cleanup with
  that expectation on record; the next real lever for Sponza-class scenes is trace cost
  (resolution, sample counts), to be driven by per-pass GPU timings.
- **Slot reorder (done, Sep 2026)** — three ordering-LOGIC fixes, measured cost-neutral
  (63.75 / 64.29 → 63.94 ms): `ContactShadows` first (it occludes DIRECT light),
  `VolumetricLight` before `Fog` (the shafts finally take the fog's extinction), `LensFlare` out
  of the scene phase (it no longer accumulates through TAA). `Glare` deliberately NOT moved — it
  is paired with the ToneMapping. Full rationale on the enum members, `Graphics/EffectSlot.hpp`.
- **The per-pass GPU timings this roadmap asked for now EXIST (§ 4b)**, and they settle it: pass
  merging is FINISHED as a lever (`Combine` + `SharedDenoise` = 1.4 ms, 2.2 % of the frame),
  while **~70 % of the frame is the RTGI trace alone**. Every structural lever left is about
  RAYS, not about passes. Do not open a phase F on pass merging.
- **Occlusion lane (done, Sep 2026)** — the first ray-side lever: the ambient occlusion is
  derived from the indirect-diffuse trace instead of being traced again. **−7.16 ms, −9.2 % of
  the frame**, no quality trade (the derived estimator is the more accurate of the two).
- **Sub-scopes inside RTGI (done, Sep 2026)** — the 44 ms block is attributed per pass and the
  children close the parent to 0.001 ms. Cost: nothing (a `ScopedZone` on a null profiler is a
  no-op, and the profiler is a setting). It immediately corrected a settings-A/B estimate that
  was 22 % low on the à-trous.
- **Next ray-side lever, OPEN: the sample budget.** 91 % of RTGI is rays and 9 % is denoising,
  when it is the denoiser that makes a low count viable (§ 4b). Unmeasured IN MOTION, which is
  where a low count is paid.

## Known issues

- `Core.RendererService.screenshot()` triggers `UNASSIGNED-non-acquired-swapchain-image-used`
  (the capture path transitions a presentable image outside its acquire window).
  Pre-existing, only fires on capture, cosmetic for the capture itself — to fix in the
  `capture()` path.
