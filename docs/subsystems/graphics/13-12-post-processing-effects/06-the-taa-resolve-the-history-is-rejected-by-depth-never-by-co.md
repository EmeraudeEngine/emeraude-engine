## 12. Post-Processing Effects

### The TAA resolve: the history is rejected by DEPTH, never by colour on unchanged geometry (Sep 2026)

`Effects/Resolve/TAA.cpp`, owner's design (2026-09-23): *"c'est le buffer de couleur qui doit être
anti-aliasé, pas le depth buffer"* — the COLOUR buffer is what accumulates, the DEPTH buffer (never
filtered) decides whether the history is still valid.
- The history target's **alpha holds the linear depth** of the surface it accumulated (nothing
  downstream reads that alpha: the tone mapper writes 1). The history is **rejected** (variance
  clip, `VarianceGamma`) **only where that depth is outside this frame's 3×3 depth range** (±1 %) —
  a disocclusion, or a reprojection that landed on another surface. The range, not the centre
  depth: the depth is jittered too, so an edge pixel sees each surface in turn.
- **Why**: the colour clip compared the history (the filtered accumulation of every jitter phase)
  with the RAW jittered 3×3; on sub-pixel detail they differ by construction, the clip threw the
  valid history away every frame, and a PARKED camera shimmered. Measured on `relief` (far ground,
  16 consecutive frames, `temporalCapture`), per-pixel peak-to-peak: before, mean 3.73, **9.5 %** of
  pixels > 8/255, max 163, far band 21.2 with 61 % > 8; after, mean 0.69, **0.01 %** > 8, max 12, far
  band 2.4 with 0.09 % > 8. Jitter off (the reference of a still frame): mean 0.30. A 0.3 m camera
  jump reconverges in ~15 frames (1.6 % → 0.08 % of pixels > 16 away from the converged image).
- **Karis weights on the EXPOSED luminance**: `1/(1 + L·E)` with `E` =
  `ToneMapping::displayExposure()`, carried by `FrameContext::displayExposure` (0 = no tone
  mapper: plain EMA). In nits `1/(1+L)` is `1/L` — a harmonic mean of the samples that let a DARK
  history texel outweigh a bright current one thousands of times (it stuck dark seams along the
  clouds of `relief` as soon as the clip stopped hiding it). UE4 `HdrWeight4(C, Exposure)`, FSR2
  `PrepareRgb`, HDRP pre-exposure all feed an exposed value.
- **History deringing** (FSR2): the Catmull-Rom result is clamped to its 2×2 bilinear footprint.
- **Stationary alpha**: `alpha · mix(0.5, 1, motion in pixels)` — the EMA at 0.1 lets 13.6 % of the
  8-phase jitter's first harmonic through, 0.05 half of it.
- ⚠️ **What the depth cannot see**: a SHADING change on unchanged geometry — a moving shadow, a
  light switched on, an animated texture, a reflection, a translucent surface absent from the depth
  buffer. Those now FADE over the accumulation (~1/alpha frames) instead of snapping. If one becomes
  visible, add a colour test reserved to LARGE changes; never bring back a colour clip on still,
  unchanged geometry.
- **Cross-OS (2026-09-23)**: macOS (M2, TAA forced on) whole frame > 8 = 0.01 %, max 11 — the Linux
  figure; Windows (RTX 3060, 720p, RayTracing lane) 0.30 % against 1.21 % with its TAA OFF; Linux in
  the RayTracing lane 0.27 %. The ScreenSpace/RayTracing gap is the lane, not the machine. ⚠️ OPEN: in
  the RayTracing lane the far band stays 1.9 % > 8 with TAA against 0.02 % without — the half-res
  traced effects vary with the jitter phase and the EMA does not fully average it (was 65 % before the
  fix; not visible to the owner). Both Windows and macOS had `TemporalAA/Enabled = false` in their
  settings: that, not the machine, is why they "did not shimmer".
- **Debug view of the resolve's decision (2026-09-28)**: `Core/Graphics/PostProcessing/TemporalAA/DebugView`, read
  at creation (0 = off). **1** red = history rejected (clipped), green = what the velocity dilation changed (the
  nearest neighbour's velocity against the pixel's own, 1 px = full), blue = reprojection length (8 px = full);
  **2** the history depth nearer (red) or farther (blue) than the 3×3 range; **3** the exposed luminance gap
  between the history the blend keeps and the current reconstruction (1/8 of the display range = full red).
  Magenta = off-screen reprojection. It is the SAME resolve recorded a second time into its own target
  (`TAA_DebugView`) and shown instead: the history keeps accumulating for real, so the view explains the
  actual feedback loop. ⚠️ Painting the decision into the resolve's output would feed the colours back as
  history — the output IS the history.
  ⚠️ "A neighbour's velocity was taken" is not a signal: on a grazing ground the nearest 3×3 depth is
  always another pixel, so the whole ground lit up. Only the velocity DIFFERENCE is.
- ⚠️ **What the depth cannot separate (measured 2026-09-28, `animation-debug`)**: an object from the ground it
  stands on, at a grazing angle — the ground's depth range over a 3×3 (and more so a 5×5) contains the depth of
  the object's feet, blade, shadow-caster. The centre-depth tag also LAUNDERS thin moving objects: a 1-3 px
  blade is a jittered edge almost everywhere, its colour enters the history tagged with the ground's depth half
  the time, and it is never rejected once the blade has gone (1-px lines, one per logic tick). Tagging the
  nearest 3×3 depth cuts those lines by 60 % but rejects a parked silhouette's own history every other jitter
  phase (`relief` horizon: max 7 → 39); a 5×5 acceptance range removes that cost and the benefit with it.
  Numbers and the open owner decision: `docs/todo/taa-trails-under-motion.md`.
- An HDRP-style anti-flicker (the clip widened with stationarity and temporal contrast) was tried
  the same day and reached far band 11.6 / 7.9 (base gamma 1.0 / 1.5, full strength): it only
  softens the wrong test. Removed.

> [!CAUTION]
> **Never read the folder to know how an effect executes.** `Resolve/` holds both mechanisms —
> TAA is an `IndirectPostProcessEffect`, FXAA/Sharpen/FXAASharpen are `DirectPostProcessEffect`s
> folded into the final swap-chain shader. That mix is the deliberate price of ordering by
> concept. The base class is the single answer to "how does this run", `EffectSlot::slot()` to
> "when", and the folder to "on what".

> [!NOTE]
> **Two renames landed with the reshuffle.** `Effects::Framebuffer::Bloom` is
> `Effects::Camera::VeilingGlare` (aligned on its `EffectSlot::Glare`, and no longer one letter
> away from `Effects::Style::PhosphorBloom` — a different effect entirely); its `ClassId` and
> tracer tag are now `"VeilingGlareEffect"`, which is the name the GPU profiler and RenderDoc
> emit. `ToneMapping::setBloomSource()` is `setGlareSource()`.
> ⚠️ The CAMERA's switch keeps the photographer's word — `Camera::enableBloom()`,
> `bloomThreshold()`, `bloomIntensity()` — because it is the user-facing API and it names the
> *intent*, not the class. `Scenes::EffectsToolkit::LensPresets` became `StylePresets` in the
> same pass, since it composes `Effects::Style::*`.
