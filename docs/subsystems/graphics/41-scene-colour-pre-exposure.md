## 41. Scene-colour pre-exposure — the fp16 scene colour holds nits × E_pre (Oct 2026)

> Owner decisions D1-D10 (2026-09-26), D2 amended and D7's k = 4 (2026-10-24). Built and measured 2026-10-24
> (engine `defb1464` → the step-7 commit). Ideas from Frostbite (Lagarde & de Rousiers 2014, § 5.2), UE
> (`PostProcessEyeAdaptation.cpp`, ideas only), HDRP (ideas only), FSR2 (`PrepareRgb`).

### Why
The scene HDR target is `RGBA16F`; a half float stops at **65 504**. Until Oct 2026 it held absolute nits:
`basic-scenery`'s pure-blue light put 3.5e5·ρ_b nits on the ground (+Inf), a mirror under a 100 klx sun overflowed,
and the RT traces of `sponza` sat at the ceiling every frame. A physical sun disc (~1.6e9 nits, the dynamic sky's)
cannot be stored at all. Pre-exposure fixes **range**, not precision: fp16 keeps its ~2^-11 relative step at any
scale.

### The unit contract
- **Scene colour = nits × E_pre**, E_pre being the frame's pre-exposure (`Renderer::preExposure()`). A value read
  back from the scene colour is a "chain value".
- **E_pre** is latched once per frame by `Renderer::latchPreExposure()`, right before the rendering strategy runs
  (after the camera sync and the scene target): manual exposure → the camera's exact APEX exposure; auto exposure →
  the metered exposure quantised to a whole stop, moved only after one stop of drift (hysteresis); no tone mapper →
  1. `Core.RendererService.setPreExposureOverride(v)` / `Core/Graphics/PostProcessing/PreExposure/Override`
  forces it (0 = automatic). ⚠️ Never accumulated: recomputed every frame.
- The tone mapper applies the **residual** `E_display / E_pre`; the luminance extract meters `L / E_pre`, so the
  whole metering chain (EMA, plausibility window, floors, readback, the ISO shown) stays absolute — no feedback
  through E_pre.
- `FrameContext::preExposure` and `FrameContext::residualExposure` (chain value → display; named
  `displayExposure` before, when it was the absolute one) reach every effect.

### The writers (every radiance written into the scene colour × E_pre)
| Writer | How |
|---|---|
| Every scene program (`SceneRendering`) | ONE epilogue `svOutputFragment.rgb *= ubView.preExposure`, every branch (lit, unlit, magenta, instrument lanes); only the **Multiply** blend is exempt (it writes a factor) |
| Transport | the view UBOs' float after `iblDiffuseWeight` (2D, cubemap, CSM); `ViewMatricesInterface::setPreExposure()` patches the region this frame already uploaded (the main view is uploaded BEFORE the latch) |
| Grab-pass transmission | read `/ ubView.preExposure` (`LightGenerator::sceneRadianceToNits()`): it is this frame's scene colour |
| Render-target reflection / refraction source | `/ the source target's own scale` (`renderTargetScale()` at material setup) |
| `DeferredLightResolve` | × the view's pre-exposure (push constant) |
| RTGI, RTR traces | × E_pre on their RGB (RTGI `bounceParams.y`, RTR's trace UBO); confidence / AO lanes are unitless |
| Fog, clouds (sun + sky), volumetric scattering, SSR sky miss, SSGI sky term | the CPU constant × E_pre |
| VolumetricLight | its mask is UNITLESS; the radial pass applies colour × intensity × E_pre |
| LensFlare | source threshold on `chain × residual`; the analytic sun (from lux) × `residual × E_pre`; ghosts `/ residual` |
| VeilingGlare | threshold and firefly ceiling × E_pre; Karis weight on `chain × residual` (DISPLAY units, D8) |
| TAA | Karis weights and anti-flicker on `chain × residual` (display luminance) |

### The world caches: ONE fixed scale (D7)
`Graphics::WorldCacheScale` = 2^-4 (owner, k = 4: finite to ~1.05e6 nits, normal down to ~1e-3). A world cache is
read by every view and outlives any exposure, so it does not follow E_pre:
- a render-to-texture cubemap of ≥ 16 bits per channel (a reflection probe) renders with its view's pre-exposure =
  S; an 8-bit target and the imposter bake keep 1. Its readers divide by the source's scale;
- the irradiance probe volume stores E/π × `ProbeCacheScale` (the GLSL literal, `static_assert`ed equal to S):
  the blend pass encodes, `probeIrradiance()` decodes. Both blends carry a FINITE GUARD (`probeIsStorable()`, a
  range test — Metal's fast math may fold `isnan`/`isinf`): an unstorable sample is dropped, an unreadable history
  replaced. One Inf texel used to live forever under a 0.97 hysteresis.

### Histories (D5)
A temporal history stores the pre-exposure it was written at and rescales by `r = E_now / E_history` (1 after any
reset — the HDRP white-flash bug): the TAA history (`historyRescale`), the GIDenoiser's colour (`rgb × r`) and
moments (`m1 × r`, `m2 × r²`) for RTGI, SSGI and RTR. From the HISTORY's own frame, not the previous rendered one:
a producer that skipped frames still rescales right.
Measured (sponza, E_pre 1 → 2^-4): before the rescale the mean went 27.2 → 69.6 and took ~10 s to settle; after,
27.1 → 27.1 at once.

### Measured (Linux, RTX 3070 Ti, 2026-10-24)
- **Invariance** (E_pre forced, pinned triad): bit-exact with the scene effects bypassed; with every effect on and
  bloom on, `post-processor-effect-debug` alternating 2^-4 / 2^-10 stays inside the A/A noise 0.3 s after each
  switch (max 7/255 vs A/A 12/255).
- **T1** `basic-scenery`: automatic E_pre 3.9e-3 → 0 overflow over 274 frames, scene-colour peak 163; forced 1 →
  1 frame overflowing, peaks 13 000.
- **T2** `light-and-shadow-debug 0,1`: peaks 56 032 → 1.4. A 1-texel NaN in `SceneColour` remains in ~12 % of the
  frames at ANY E_pre — a NaN producer, not a range overflow (`todo/blended-lit-sprite-writes-nan.md`).
- **T3** auto metering: the metered luminance is bit-identical with automatic E_pre and forced 1 (54.8118401). The
  image converges monotonically as E_pre shrinks toward the automatic value (sponza, pixels > 1/255 vs automatic:
  E_pre 1 33.6 %, 2^-4 17.7 %, 2^-8 11.7 %, A/A 9.8 %): at a large E_pre the GIDenoiser's second moment (luma²)
  overflows its fp16 target — the automatic image is the right one.
- **T4** a 4-stop step of the triad: no ramp (sponza right after the step vs 3 s later inside the A/A noise).

### Squared quantities are stored as their ROOT (fixed 2026-10-24, after the peers' verification)
The GIDenoiser's second moment `m2 = luma²` and the à-trous variance were stored in RGBA16F: they overflow past a
luminance of 256, which a pre-exposed signal reaches at a large E_pre. Measured by macOS-PA and Windows-PA on
`sponza`: fireflies from E_pre 2^-8 up (Windows: 2 564 pixels ≥ 30/255 brighter at 2^-4, A/A 24), a ramp and a
residue after the step; `disable(IndirectDiffuse)` removed them. sponza's own automatic value (2^-9) sat just below
the onset, so a darker scene in automatic mode would have shown them. The moments' G now holds `sqrt(m2)` (the RMS
luminance) and the à-trous alpha the standard deviation: the luminance's range, the same relative precision.
After: 13 pixels at 2^-4 and 2^-2 against 39 for the A/A (Linux). Rule: **store a squared radiance as its root**.

### Known, accepted: a non-power-of-two E_pre reads ~0.1-0.2/255 brighter (Windows-PA, 2026-10-24)
A manual camera's E_pre is the exact APEX value (D3), not a power of two; every power of two renders identically
(within the A/A) while the exact value differs from them by a uniform ~0.13 % (mean |d| 0.27/255, contours on the
smooth walls of `post-processor-effect-debug`). The fp16 mantissa rounding through the chain. Invisible; kept.

### ⚠️ Traps
- ⚠️⚠️ **Any radiance you synthesise from a CPU constant in nits (a light's lux, a sky luminance, an emission) must
  be × E_pre**; anything sampled from the scene colour already is. A forgotten writer shows only when E_pre moves:
  run the invariance test (force E_pre, compare against automatic) on any new effect.
- ⚠️ **An absolute epsilon or threshold on a radiance** (a floor in nits, a firefly ceiling, a Karis weight) changes
  meaning once the signal is ~1e-4 of its nits: scale it with E_pre, or decide it is meant in display units.
- ⚠️ **A history that is not rescaled** shows as a transient after every E_pre step (auto exposure steps one stop at
  a time): store the E_pre it was written at, rescale by the ratio.
- ⚠️ **A mean-luminance A/B is blind** under auto exposure: compare per pixel at a pinned exposure.
- The capture JSON carries `preExposure` and `displayExposure` per frame (D9).

### D8 — the glare's Karis weight reads DISPLAY units (owner decision, 2026-10-24)
Karis's intent: `1 / (1 + L)` on the displayed luminance, not on nits (where it is `1 / L`). Measured before the
choice (pinned f/8 · 1/125 s · ISO 100, one build per variant): `water-world` posed (`Act.setPosition(0, 60, 0)`,
`lookAt(200, 0, 200)`) — the sea −2.1/255 on average over the lower half against the nits weighting (81 % of its
pixels > 1/255, A/A 39 %), the sky unchanged; `light-and-shadow-debug 0,1` and `forest` inside their A/A noise.
⚠️ Without the pose the water-world comparison is invalid: the spawn framing differs between launches.
