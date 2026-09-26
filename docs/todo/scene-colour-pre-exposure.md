---
id: scene-colour-pre-exposure
title: Pre-expose the fp16 scene colour so no target stores absolute nits past 65 504
status: in-progress
priority: unranked
scope: Graphics (Renderer, PostProcessor, ToneMapping, Effects/Lighting, Effects/Atmosphere, Effects/Camera, GIDenoiser, IrradianceProbeVolume), Saphir (Generator/SceneRendering, LightGenerator)
opened: 2026-09-25
tags: [hdr, fp16, exposure, photometry, overflow, owner-decision, design]
---

# Pre-expose the fp16 scene colour so no target stores absolute nits past 65 504

> **Owner decision (2026-09-25):** the fp16 overflow of the scene colour is fixed by
> **PRE-EXPOSURE**, and the design is settled **with the owner before any code is written**. This
> item holds the decisions to take (§ What remains, A), then the build order (§ B). Citations are
> against engine `223880f4` and projet-alpha `f997201`.
>
> **State (2026-09-26):** package D1-D10 decided. **B1a DONE on Linux**: the missing TRANSFER→HOST
> readback barriers of the tone mapper and of the depth of field (C0) and the **overflow census** (C1)
> are validated at runtime and their baselines are recorded (§ B.1); the macOS and Windows self-tests
> (V1) are owed by the peers. B1b (the pre-exposure override) and B2 onward are not started.

## Why

**The buffer.** The scene HDR target is `VK_FORMAT_R16G16B16A16_SFLOAT`
(`src/Graphics/Renderer.cpp:1165-1168`). Its stated unit is **absolute luminance in nits**
(`src/Graphics/Types.hpp:357-362`). The exposure is applied only by the tone mapper
(`src/Graphics/Effects/Camera/ToneMapping.cpp:391` manual, `:491` + `:505` auto). The only other
uses are local ones in TAA and LensFlare. Nothing clamps at the source: the light radiance at
`src/Saphir/LightGenerator.PBR.cpp:841` is added at `:941`. The largest finite half is **65 504**,
and anything above it is stored as +Inf.

**The measured case (projet-alpha `basic-scenery`, pure-blue flying light).**
- The light is 64 000 000 lm (`src/Builtin/BasicScenery.cpp:162`, HEAD), so
  I = 64e6 / 4π = **5.09e6 cd** (`Photometry.hpp:92`).
- It flies 8 m up (`:100`). The falloff window `(1 - (8/500)^4)^2` is about 1
  (`Graphics/Effects/Shared/LightFalloffGLSL.hpp:53-60`), so the ground right under it gets
  E = 5.09e6 / 64 = **79.6 klx**.
- Lambertian ground luminance is ρ·E/π = 25 330·ρ nits. A light colour is now a **unit-luminance
  chromaticity** (`AbstractLightEmitter.hpp:186-196`, `emeraude-base Color.hpp:1003-1023`). Pure
  blue is therefore (0, 0, 1/0.0722 = **13.85**).
- **Blue channel = 3.51e5·ρ_b nits.** It overflows when ρ_b > 0.187. A ground with ρ_b = 0.23
  reads ~80 700 nits and becomes +Inf.
- Before the chromaticity contract, the same light peaked at 25 330·ρ_b in blue and fitted. The
  2026-09-25 decision is what exposed the defect, not what caused it.
- A bounce off that ground carries ρ_hit·3.5e5 into `RTGI_Trace`
  (`Effects/Lighting/RTGI.cpp:677`, averaged at `:686`, RGBA16F `:749`). The probe atlas gets the
  same load (`IrradianceProbeVolume.cpp:701`), and that atlas has **no finite guard** in its
  hysteresis blend (`:413`). One Inf texel there never leaves.

**The engine already documents this overflow class:**
- `Effects/Lighting/SSR.cpp:686-692`: a mirror metal under a 100 klx sun writes +Inf into the scene
  colour. The 7×7 black squares of 2026-09-13 came from that.
- `Effects/Camera/LensFlare.cpp:52-56`: a ~5e7-nit sun disc overflowed. LensFlare already works
  around it with a **local** pre-exposure (display units in, back to nits out).
- `docs/caution-points.md` § *A half-float target silently turns a physical luminance into NaN* SAID (until
  2026-09-25) that the scene buffer "escapes it only because nothing physical that bright is ever rasterized there".
  **That was false** and is corrected there (see above).
- The limit already leaks into content — see § *Waiting on this item: basic-scenery's palm belt* below.
- Other overflow sites in absolute units, independent of the scene target:
  - the GIDenoiser second moment `m2 = luma²` (`GIDenoiser.cpp:449`), which overflows for
    luma > 256;
  - the VolumetricLight mask EMA, which stores lux × chromaticity
    (`Effects/Atmosphere/VolumetricLight.cpp:114`, `:123`).
- An Inf texel that reaches the metering extract (`ToneMapping.cpp:195-197`) can push the frame's
  mean log-luminance out of the plausibility window (`:272-273`). The adaptation then **holds**,
  and the only trace is `meteredRejectedCount()`, which is shown in ImGui only (`Core.cpp:1270`).

**Why a clamp at the source is only a partial fix:**
1. Each light is a separate pass, blended `ONE/ONE` into the fp16 attachment
   (`src/Vulkan/GraphicsPipeline.cpp:659-697`). The running sum is **stored in fp16 between
   passes**. Clamping each pass at 65 504 still lets ambient + N lights add up to +Inf. Only the
   sum could be clamped, and fixed-function blending does not clamp a float attachment
   (`Types.hpp:359-361`). Frostbite avoids the case by lighting in one fp32 pass (References [F],
   p. 91). This engine cannot.
2. A clamp throws away the energy that matters. Every source above the ceiling becomes the same
   value. VeilingGlare's threshold-proportional firefly ceiling exists precisely to keep sources
   apart (`Effects/Camera/VeilingGlare.cpp:630-634`), and so does the "full relative punch" of an
   HDR sun (`Graphics/AGENTS.md:3646`).
3. The clamp would have to sit in **the same writers** as pre-exposure: scene programs, RT traces,
   probe volume, denoiser moments. It costs the same plumbing and fixes less.
4. It does nothing for the bottom of the range. A moonlit albedo-0.2 surface
   (`Photometry.hpp:61`, 0.25 lx) is 0.016 nits, only 8 stops above fp16's smallest normal
   (6.1e-5). A darker night drops into subnormals.

**What pre-exposure buys.** Pre-exposure stores L·E_pre with E_pre ≈ the display exposure. The
tone mapper then applies E_display / E_pre. In steady state the storage window is centred on the
metered middle grey (0.104, `Photometry.hpp:80`):
- **19.3 stops** of headroom above it (log2(65 504 / 0.104));
- **10.7 stops** below it before subnormals.

With the default camera (f/2.8, 1/60 s, ISO 100-12 800, `Scenes/Component/Camera.hpp:914-921`),
E ∈ [1.77e-3, 0.227]. The storable ceiling is then 3.7e7 nits at ISO 100 and 2.9e5 nits at
ISO 12 800. Even pinned at the ISO ceiling, the measured case stores 7.96e4·ρ_b, which fits up to
ρ_b = 0.82.

**What pre-exposure does NOT cure (computed, not measured). Every existing guard stays:**
- `distributionGGX` has no roughness floor (`LightGenerator.PBR.cpp:62-72`). At roughness 0 and
  N·H = 1 it computes 0/0, which is NaN.
- A metal sun specular peaks near F0·E_sun / (4π r⁴). Under 100 klx it still overflows after
  pre-exposure for r < 0.12 at E = 1.77e-3, and for r < 0.043 at f/11, 1/250 s, ISO 100
  (E = 2.75e-5).
- HDRI texels are clamped at 65 504 × the sky level (`Graphics/CubemapResource.cpp:94-111`). With
  the 8 000-nit default sky (`Renderable/SkyBoxResource.hpp:85`) that is 5.2e8 nits, which
  overflows for any E_pre > 1.25e-4, i.e. at every exposure the default camera allows.
- Any brightening faster than the adaptation (EMA rates 1.5 / 2.0 per second,
  `ToneMapping.hpp:112-113`, `ToneMapping.cpp:315-316`).

## Waiting on this item: basic-scenery's palm belt (measured 2026-09-25)

Under the lost falloff (2026-08-12 → 2026-09-25) the red `Bulb` of projet-alpha's `basic-scenery` lit every palm
within 300 m; with the inverse square back, its palm belt reads 0.09 of that look (linear, spawn, pinned exposure).
A HIGH STATIC FLOOD was designed to bring it back (a six-agent study: two unshadowed LightRed lamps, 9e10 lm at 610 m
and 1.3e10 lm at 380 m). Its binding constraint was **≤ 20 000 lx on the nearest palm, i.e. fp16 headroom** (bark
worst-view overflow ~48 klx for LightRed: 1.3568 × E × f_BRDF > 65 504), and it was measured against a build of the
old law (3 launches per build — the palms are re-drawn at every launch — linear ratios, 0 VUID, no non-finite
pixel):

| Region | old law | without the flood | with the flood |
|---|---|---|---|
| palm crop at the spawn | 1.00 | 0.09 | 0.28 (0.24-0.31) |
| palm-belt horizon row | 1.00 | 0.06-0.21 | 0.21-0.30 |
| obelisks / foreground ground | 1.00 | 1.26-1.39 / 0.52 | 1.34-1.44 / 0.65 |

It cost +3.8 ms/frame (52.6 → 56.4, RTX 3070 Ti, validation on) and missed its keep condition (≥ 0.45; the model
had predicted 0.42-0.92, over-estimating the spawn view ~2.3×), so the owner DROPPED it until this item lands.
With the fp16 cap lifted, the study's variant with a 48 klx cap modelled 0.70 / 0.79 on the inner belt: the refit
is projet-alpha item `basic-scenery-palm-belt-lost-bulb-flood`, **blocked by this one**, and it is test case T1's
neighbour (same scene).

## What remains

### A. Decisions for the owner

> **DECIDED (owner, 2026-09-26): the recommended package, all ten** — D1 (b) multiply at every writer's output; D2 (b)
> a new frame-indexed per-frame uniform; D3 (c) the metered exposure quantised to whole stops with hysteresis (manual
> mode: the exact triad), the tone mapper applying E_display / E_pre; D4 (a) metering divides back; D5 (a) rescale the
> histories by r, r = 1 after any reset; D6 grab pass ÷ E_pre, render-target reflection sources × E_pre / E_target,
> Multiply blend exempt, the unlit no-emission branch scaled like the rest; D7 (b) + (b) one fixed cache scale S for
> the probe volume and the render-target probes, plus the finite guard; D8 mechanical, the glare's Karis weighting
> MEASURED before any compensation; D9 E_pre written into the capture JSON; D10 the overflow census and the
> pre-exposure override built FIRST. The option lists below stay as the record of what was weighed.

Each decision lists its options. "→ discuss" marks the audit's recommendation; it is not a
decision.

**D1 — Where the scale is applied.**
- (a) **At the source**, in the light constants (Filament [Fi]). This breaks the threading
  contracts:
  - light UBOs are published on the logic thread with a generation cache
    (`AbstractLightEmitter.cpp:101-122`, `:162-197`), so every light would be re-uploaded every
    frame;
  - the RT light SSBO is a single buffer rewritten every frame (`Scenes/LightSet.cpp:251`);
  - the view UBO is single-buffered (`Graphics/AGENTS.md:5340`, Rule 4);
  - static material emission, sky, IBL and fog would each need their own path, and any missed
    emitter shows as a wrong brightness.
- (b) **At every writer's output** (the Frostbite [F] / UE [U] / HDRP [H] epilogue). Writers:
  - `rgb *= E_pre` at `Saphir/Generator/SceneRendering.cpp:729` (lit) and `:944`, `:948`
    (unlit), never on alpha (it drives blending and the discard at `:1035`);
  - the RT traces `RTGI.cpp:701` and `RTR.cpp:907`, `:936` (RGB only; the AO and confidence
    alphas are unitless);
  - the radiance that post effects synthesise from CPU constants (D8).

  The scale is linear, so the additive sum of passes stays exact.

  → discuss (b).

**D2 — How E_pre reaches the scene programs.** It is a per-frame **and** per-target scalar. The
view UBO (Rule 4), the material UBOs (static) and the light UBOs are all excluded.
- (a) **A per-draw push constant**, following the jitter precedent
  (`Saphir/AGENTS.md:1420-1464`). It is per-frame and per-target by construction, and costs 4 B
  on a 76 B block, but it needs the FRAGMENT stage added. It has **three holes**: MDI and
  cubemap/CSM push nothing today, and the 132 B advanced fallback has no room left under the
  128 B min-spec. A missing member is either a glslang error or a **silent** read of garbage.
- (b) **A new frame-indexed frame uniform** (Rule 1 pattern, `Graphics/AGENTS.md:5298-5307`), one
  copy per frame in flight, with a dynamic offset per render target. There is one site to write
  and room for future per-frame data. The cost is a new set in every scene pipeline layout, which
  touches the program cache keys (see `program-cache-key-codegen-inputs-audit`).
- (c) **A GPU texture** (the adapted 1×1, `ToneMapping.hpp:348-350`). Only relevant if D3 takes
  the GPU source.

→ discuss (b): each hole in (a) needs its own path.

**D3 — Which exposure value.** Manual mode is settled: use the current frame's APEX value
(`ToneMapping.cpp:942-991`), which is exact (every surveyed engine does this). For auto mode:
- (a) **CPU**: `displayExposure()` fed by the existing readback ring (`ToneMapping.cpp:819-823`,
  `:993-1010`). It lags framesInFlight frames (2-3), like UE [U], and works for every CPU-baked
  constant.
- (b) **GPU**: the previous frame's adapted 1×1 (one-frame lag, HDRP [H]). Every shader then
  samples a texture, and CPU constants (fog, clouds, glare threshold) cannot use it.
- (c) **(a) quantised to whole stops with hysteresis**: E_pre = 2^round(log2 E), changed only when
  it is more than one stop off. The ratio r is then an exact power of two (a lossless multiply),
  histories are rescaled only on a rare step, and E_pre is deterministic in captures. The cost is
  at most 1 of the 19.3 stops of headroom.
- (d) **A FIXED 2^-k** (like UE's cached-lighting offset). There is no lag, no history rescale and
  no metering change, and k ≥ 3 already fixes basic-scenery. But it does not follow the scene: a
  night scene loses k stops at the bottom.

Whatever the source, the tone mapper multiplies by **E_display / E_pre, using the E_pre this
frame's writers used**. The displayed exposure then stays exact and lag-free [U].

The GPU source buys 1-2 frames against an adaptation that moves about 9.5 % of the log gap in
50 ms, which is not measurable. The overflow risk on a transient comes from the adaptation speed,
not from the lag.

→ discuss (a) or (c).

**D4 — Metering on pre-exposed data.**
- (a) **Divide back in the extract**: `log(max(L / E_pre, 1e-4))` (`ToneMapping.cpp:195-197`),
  which is a subtraction in log space. The EMA, the readback, the ImGui "nits", the
  [-9.3, 16] window and the 1e-4 / 0.001 floors (`:272-273`, `:491`) all stay **absolute**, and
  there is no feedback loop (UE histogram [U], HDRP `ExposureCommon.hlsl:98-106` [H], FSR2 [A]).
- (b) Meter in pre-exposed space. The window and floors then move every frame, and each readback
  slot must remember its own E_pre. No benefit.

→ discuss (a).

**D5 — History policy.** In every case r = E_N / E_{N-1} is taken over **rendered** frames (the
frame-history contract, `Graphics/AGENTS.md:5316-5338`), never over the logic slots. The histories
concerned:
- TAA: `rgb × r` right after the sample (`Effects/Resolve/TAA.cpp:368`); the depth alpha is left
  alone.
- The GIDenoiser, which has three owners (RTGI, SSGI, RTR): `rgb × r`, `m1 × r`, `m2 × r²`
  (`GIDenoiser.cpp:448-452`). The confidence alpha in reflection mode is never scaled, and the
  1e-4 epsilon at `:564` is absolute.
- The VolumetricLight mask: better made **unitless** (`isLit`), with
  colour·intensity·gain·E_pre applied in the radial pass (`VolumetricLight.cpp:191`), so it never
  needs rescaling.

Options:
- (a) Rescale by r (UE, FSR2 `PrepareRgb` [A]).
- (b) Rescale, but reject the history when r ≥ 2 (HDRP `TemporalFilter.compute:219-236`). Note
  that under D3(c) every step is exactly 2×.
- (c) Store histories absolute. Impossible in RGBA16F (it is this very defect), unless they go
  fp32 at twice the VRAM.
- (d) No correction (HDRP's TAA, Filament). Ramps or flashes at every step.

→ discuss (a). **r = 1 on the frame after any reset or effect (re)creation** (the HDRP bug [H2]).

**D6 — Inputs inside scene programs that are already exposed** (they must not be exposed twice):
- The grab-pass transmission is from the same frame: multiply it by 1/E_pre before the output
  multiply (`LightGenerator.cpp:995-997`; flag `LightGenerator.hpp:1319`).
- Render-target reflection and refraction sources (`Material/StandardResource.cpp:4772-4774`,
  `:4926-4928`): multiply by E_pre / E_target (see D7).
- `BlendingMode::Multiply` writes a factor, not a radiance (`GraphicsPipeline.cpp:729-736`), so it
  is exempt.
- Debug lanes that overwrite `fragmentColor` (`SceneRendering.cpp:803`) are written after the
  multiply, or exempted (UE's `IsPreExposureRelevant` [U]).
- **Sub-decision:** the unlit no-emission branch (`:944`) writes a raw [0,1] colour. Either scale
  it like everything else (it keeps today's ~1-nit meaning), or introduce a per-material
  **exposure weight** (Filament `emissive.w` [Fi], HDRP `_EmissiveExposureWeight` [H]), or neither.

**D7 — World caches.**

*Probe volume* (RGBA16F atlas, hysteresis 0.97, `IrradianceProbeVolume.cpp:413`):
- (a) Absolute. The overflow stays, and so does the immortal Inf.
- (b) A **fixed cache scale S = 2^-k**, with a reset when it changes (UE
  `r.EyeAdaptation.CachedLightingPreExposure`, default 4 EV [U]). In E/π units, k = 4 stores
  [9.8e-4, 1.05e6] and k = 8 stores [1.6e-2, 1.7e7]. The cache stays view-independent.
- (c) Follow E_pre, with `previous × r` at `:413`. This ties a world cache to the view: every
  camera cut rescales the whole atlas.

Under (b) or (c), every reader converts to its own scale: `RTGI.cpp:307-316`, `RTR.cpp:839-868` and the
volume's own feedback `IrradianceProbeVolume.cpp:326`. **Add a finite guard at `:413` whatever is chosen.**

*Render-to-texture probes* (`Renderer.cpp:2469-2600`; continuous, "once", or suspended under
SSR/RTR, `Scenes/Scene.rendering.cpp:349-354`):
- (a) E = 1 (today's content, today's overflow).
- (b) The **same S** as the probe volume, so a stale bake stays valid.
- (c) The view's E_pre, stored with the target. A stale bake then carries a stale exposure.

→ discuss one S for every world cache, i.e. (b) and (b).

**D8 — Post effects that synthesise radiance or hold nit thresholds.** Mostly mechanical; one
choice at the end.
- Multiply the CPU pushes by E_pre:
  - AtmosphericFog `Effects/Atmosphere/AtmosphericFog.cpp:363`, `:397-406`;
  - VolumetricClouds UBO `VolumetricClouds.cpp:985-989`, `:1020`;
  - VolumetricScattering `:441`, `:513-515`;
  - the SSR miss `SSR.cpp:741`;
  - the SSGI sky `SSGI.cpp:613`.

  Leave the chain-sampled terms (SSR `:677-686`, SSGI `:578`) unscaled.
- LensFlare: the source pass uses the **residual** E_display / E_pre (`LensFlare.cpp:134-157`,
  `:408`), and the ghost pass uses its inverse (`:256`, `:493`).
- TAA Karis weights use the residual (`TAA.cpp:414-424`).
- The VeilingGlare threshold and firefly ceiling are multiplied by E_pre (`VeilingGlare.cpp:622-634`).
- **Choice:** the glare's Karis `1/(1+L)` (`:79-86`) would then see roughly display units, which
  is what Karis intends but changes the look. Measure it, then accept it or compensate.

Plumbing:
- `FrameContext` gains `preExposure` and `preExposureRatio`. `displayExposure` becomes the
  residual, or is renamed (`IndirectPostProcessEffect.hpp:456-459`).
- It is built at chain start, **after** the scene pass is recorded (`PostProcessor.cpp:1272-1276`),
  so E_pre must be latched earlier.
- **E_pre = 1 whenever no ToneMapping runs.** A chain can require HDR without a tone mapper.

**D9 — Captures, recording, measurement.**
- PNG, `temporalCapture(N)` and RushMaker copy the LDR swap chain (`Renderer.cpp:1867-1870`,
  `:1877-1883`, `:2830-2873`), so they are **unaffected**.
- To decide: write E_pre and the metered exposure into the capture JSON. It records the triad
  only today (`Graphics/FrameCapture.cpp:565`).
- HDR downloads (`SceneRenderTarget.cpp:170-200`, no caller; `dumpRenderTarget` via
  `Vulkan/TransferManager.cpp:275-309`) already misread RGBA16F as RGBA8. Out of scope.
- The debug markers written as nit constants (`TAA.cpp:305`, `SSR.cpp:712`,
  `GIDenoiser.cpp:1060-1073`) must stay saturated: check them, no redesign.

**D10 — Validation instruments** (decide before step B1):
- **DebugNonFinite is not enough.** It only paints what SSR's resolve and TAA's 3×3 see (keys read
  at `SSR.cpp:781`, `TAA.cpp:453`), and elsewhere a NaN turns black on NVIDIA (`TAA.cpp:299-302`).
  A clean DebugNonFinite proves nothing.
- Proposed **overflow census**: per frame, count the texels that are non-finite or ≥ 65 504 in the
  scene colour at the tone-map input, `RTGI_Trace`, `RTR_Trace` and the probe atlas. Read the
  counts back through a ring like the metered exposure's, and print them in
  `PostProcess.getStatus()`. It uses a **range test**, because Metal fast math folds away
  `isnan` / `isinf` (`ToneMapping.cpp:266-270`).
- Also put `meteredRejectedCount()` on the console.
- Proposed **pre-exposure override** key (UE `r.EyeAdaptation.PreExposureOverride` [U]), which
  forces E_pre while the display exposure stays pinned. It drives the invariance test in B8.

### B. Implementation, once A is decided (each step has its own gate)

1. **Instruments (D10) first.** Record a baseline for T1-T4 (B8) in this item.
   - **B1a, the overflow census — DONE (2026-09-26, Linux, RTX 3070 Ti, 2880×1620).**
     `Graphics::OverflowCensus` (mechanism, traps, cost: `src/Graphics/AGENTS.md` § "The overflow
     census"; commands: `docs/ai-runtime-control.md` § 6). Channels `SceneColour` (added: the SSR/TAA
     guards scrub the tone-map input, owner-confirmed P10), `ToneMapInput`, `RTGI_Trace`, `RTR_Trace`,
     `ProbeIrradiance`; an integer-only classification of the IEEE bits (not a float range test) into
     NaN / Inf / ceiling (≥ 65 504) / peak. Created in every session, **disarmed by default**
     (`Core/Graphics/PostProcessing/OverflowCensus/Enabled`, owner-confirmed P8/P15).
     `meteredRejectedCount()` is on the console (`getStatus()` `Metering:` line, `getFrameDiagnostics()`).
   - **Validation (Linux, validation layers ON, 0 VUID in every run):**
     - V1: `testOverflowCensus()` = `PASS` (tested 256, NaN 21, Inf 12, ceiling 8, peak 65 472).
       **macOS and Windows: owed by the peers.**
     - V2 (`post-processor-effect-debug 0,0`, pinned f/8 · 1/125 s · ISO 100, `Reflections` off): the
       scene is not bit-reproducible frame to frame — the A/A control differs by max 3/255 on 4.0 % of
       the pixels — and armed vs disarmed differs by the same (max 3/255, 4.3 %): no effect beyond the
       noise of the reference itself.
     - V3: `OverflowCensus` inside `PostFXChain`, **0.21 ms avg, 0.42 ms max** (5 channels,
       `basic-scenery`). The second machine's figure is still to take.
     - V4: `tested == expected` on every channel (4 665 600 for `SceneColour`, `ToneMapInput` and
       `RTR_Trace`, 1 166 400 for the half-res `RTGI_Trace`, 131 072 for the atlas).
     - V7 (`light-and-shadow-debug 0,1`, both lanes, synchronization validation listed as enabled):
       **0 hazard on the census barriers**. SSR's colour pyramid (F11) did NOT show either. The one
       hazard of the run is unrelated and pre-existing: a `WRITE_RACING_WRITE` between two uploads of
       the `Notifier` overlay surface on two round-robined queues (its own item,
       `overlay-surface-reupload-races-on-two-queues`).
     - V8: `rejected` stayed at 0 over 3 minutes of auto exposure.
   - **Baselines (10 s windows):**
     - T1 `basic-scenery` (default options): **no overflow in any channel** over 143 frames (the GPU
       profiler was on); `SceneColour` peak 23 232. The BlueLight's pass over the ground did not fall in
       that window: T1 is not settled — re-take it over a full flight period.
     - T2 `light-and-shadow-debug 0,1`: `SceneColour` overflows by **1 texel** in 53 of 421 frames
       (screen-space lane) and 24 of 191 (traced lane), peak finite 56 288; `ToneMapInput` 0 in both.
     - T3 `sponza`, f/8 · 1/125 s · ISO 100: **the premise is false** — `SceneColour` overflows in
       every one of 82 frames (1-3 texels) and **`RTGI_Trace` holds 315-358 texels at the ceiling in
       every frame** (peak finite 65 472); `ToneMapInput`, `RTR_Trace` and the atlas stay clean.
   - B1b, the pre-exposure override (`Core/Graphics/PostProcessing/PreExposure/Override`,
     `setPreExposureOverride`): not started.
2. **E_pre plumbing.** Latch E_pre once in `Renderer::renderFrame`, **right after the scene-target
   block and before the rendering-strategy dispatch** (`Renderer.cpp:1834`/`:1845` at `a42a7410`) —
   ⚠️ AMENDED 2026-09-26 (owner-confirmed P18): this text said "beside `prepareFrameJitter` (`:1665`)".
   Every input is final at the new site (the camera-effect sync, `m_postProcessingActive`, the scene
   target), and D7(b) removed the only reason to latch before the RTT pass (`:1683`); the probe
   updates (`:2056`, `:2218`) still come after it. Keep one value per frame in flight plus the
   previous *rendered* one, and derive r from them.
   *Gate:* with E_pre forced to 1, the PNGs are bit-identical.
3. **Tone map ÷ E_pre and extract ÷ E_pre (D3, D4).**
   *Gate:* with an override ≠ 1 and no writer changed yet, the image scales by exactly that factor.
4. **Scene programs**: the transport chosen in D2, the multiply from D1, the exemptions and
   compensations from D6, and the RTT scale from D7.
5. **RT lanes, the probe volume and its scale (D7), and the finite guard at
   `IrradianceProbeVolume.cpp:413`.**
6. **Post effects and the `FrameContext` fields (D8).**
7. **Histories (D5).**
8. **Tests.** Validation layers ON, on all three platforms (the peers). Each one is measurable:
   - **Invariance test.** Pin the triad and alternate E_pre between 2^-4 and 2^-10 every 30
     frames. `temporalCapture` must stay within 1/255 away from pixels that overflowed before. A
     region that moves names a missed writer or a double exposure (glass = grab, sky = background,
     fog, flare). A ramp names a history that was not rescaled.
   - **T1**, `basic-scenery` BlueLight. Census > 0 before (scene colour when ρ_b > 0.19 under the
     light; RTGI_Trace and the atlas when bounce rays land on the patch), 0 after. The lights fly,
     so compare the max over a 10 s window.
   - **T2**, `light-and-shadow-debug --demo-options 0,1` mirror floor (the documented SSR +Inf).
     No DebugNonFinite square after. The reflected/direct ratio (RTR 0.82, SSR 0.40, projet-alpha
     `.claude/rules/build-and-run.md`) stays unchanged.
   - **T3**, a scene that never overflowed (`sponza`), exposure pinned with `Act.setExposure`.
     PNG A/B within 1/255 on ≥ 99.9 % of pixels; in auto mode, the metered luminance is identical
     to within fp16 rounding.
   - **T4**, a step between two pinned triads 4 stops apart. The static region steps in exactly
     one frame, with no ramp over the GIDenoiser accumulation length.
9. **Docs, in the same session:**
   - `Types.hpp:357-362` (the unit contract; the case against a Screen blend mode still holds);
   - `docs/caution-points.md` § *A half-float target silently turns a physical luminance into NaN* (already corrected
     on 2026-09-25: rewrite it once pre-exposure lands);
   - `Graphics/AGENTS.md` (a scene-colour unit and cache-scale section, plus the TAA Karis note at
     `:1793-1797` and LensFlare at `:4044-4050`);
   - `docs/ai-runtime-control.md` (override key, census);
   - unblock projet-alpha `basic-scenery-palm-belt-lost-bulb-flood` (refit the flood without the fp16 cap).

## ⚠️ Traps

- ⚠️⚠️ **Pre-exposure fixes RANGE, not precision.** fp16 keeps its ~2^-11 relative step at any
  scale, so the ONE/ONE multi-pass quantisation is untouched. Do not expect it to cure banding.
- ⚠️⚠️ **Never accumulate E_pre** (`E *= r`). Recompute it every frame from the metered value, and
  compute r from two stored values.
- ⚠️ **E_pre comes from the frame's LATCHED camera state**, the same one the tone mapper resolves.
  Otherwise an animated manual triad desyncs the two for one tick, and the frame pops.
- ⚠️ **Transient frames.** On the first frame `displayExposure()` returns √(min·max)
  (`ToneMapping.cpp:1009`), a 3.3e6-nit ceiling with the default camera. A history reset means
  r = 1. A light switched on or a teleport can still overflow for as long as the eye takes to
  adapt. **Keep every finite guard:** `SSR.cpp:686-693`, `TAA.cpp:371`, `VeilingGlare.cpp:167`,
  `SSGI.cpp:618`, `GIDenoiser.cpp:170-173`, and the range test in `ToneMapping.cpp:265-320`.
- ⚠️ **LensFlare already pre-exposes locally.** Moving it to the residual exposure is required, or
  it gets exposed twice.
- ⚠️ **A mean-luminance A/B is blind**, because the auto-exposure absorbs the difference. Compare
  per pixel, at a pinned exposure.
- ⚠️ **Placement.** This is an engine item. projet-alpha only hosts test cases T1 and T3.

## References

**Engine:** all file:line citations above. Related items: `blended-lit-sprite-writes-nan` (a NaN
producer writing into the same buffer), `cubemap-peak-luminance-nits` (the absolute sky level),
`program-cache-key-codegen-inputs-audit` (touched by D2(b)).

**External** (licences: UE and HDRP code gives **ideas only**; Filament and FSR2 are
LGPLv3-compatible with citation):
- **[F]** S. Lagarde, C. de Rousiers (EA DICE), *Moving Frostbite to Physically Based Rendering
  3.0*, SIGGRAPH 2014 course notes v3.2, § 5.2 "Manipulation of high values" pp. 89-91 (Listing
  31), § 5.1.1 p. 84, § 5.1.3 p. 87 —
  https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
- **[U]** Epic Games, Unreal Engine 5.8.3 @ `396c9f05`, `PostProcessEyeAdaptation.cpp`
  (UpdatePreExposure `:1504-1609`, CachedLightingPreExposure `:201-207` / `:235-243`, override
  `:69-75`), `PostProcessHistogram.usf:116-118`, `PostProcessTonemap.usf:311`,
  `TemporalAA.cpp:824` — https://github.com/EpicGames/UnrealEngine (Epic-linked account, Unreal
  EULA). *Auto Exposure (Eye Adaptation)*, Epic Games, UE 4.27 docs —
  https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-eye-adaptation?application_version=4.27
- **[H]** Unity Technologies, HDRP @ `a7e4c051` (`ShaderConfig.cs:39-40`,
  `Deferred.compute:176-183`, `ExposureCommon.hlsl:98-106`, `TemporalFilter.compute:219-236`,
  `HDRenderPipeline.PostProcess.cs:1271-1302`) —
  https://github.com/Unity-Technologies/Graphics (Unity Companion License). *HDRP features*,
  Unity Technologies —
  https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition@17.0/manual/HDRP-Features.html
- **[H2]** pmavridis (Unity Technologies), *[HDRP] Fix white flash with SSR when resetting camera
  history*, PR #5089 — https://github.com/Unity-Technologies/Graphics/pull/5089 (the previous
  exposure was wrong for the frame after a history reset).
- **[Fi]** R. Guy, M. Agopian (Google), *Physically Based Rendering in Filament*, § "Pre-exposed
  lights" — https://google.github.io/filament/Filament.md.html (Apache-2.0).
- **[A]** AMD, *FidelityFX Super Resolution 2* (`ffx_fsr2_common.h:493-509` PrepareRgb /
  UnprepareRgb, `ffx_fsr2_reproject.h:110`, README § Exposure) —
  https://github.com/GPUOpen-Effects/FidelityFX-FSR2 (MIT).
- The audit's local extracts (Frostbite text, sparse clones, UE excerpts) were session-temporary and must not be
  redistributed: re-fetch from the URLs above.
