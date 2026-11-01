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
- **Karis weights on the EXPOSED luminance**: `1/(1 + L·E)` with `E` = the RESIDUAL exposure
  `ToneMapping::displayExposure() / E_pre`, carried by `FrameContext::residualExposure` (0 = no tone
  mapper: plain EMA), since the scene colour holds nits × E_pre (graphics doc 41). The history is
  rescaled by `E_now / E_history` before it is blended (`historyRescale`). In nits `1/(1+L)` is `1/L` — a harmonic mean of the samples that let a DARK
  history texel outweigh a bright current one thousands of times (it stuck dark seams along the
  clouds of `relief` as soon as the clip stopped hiding it). UE4 `HdrWeight4(C, Exposure)`, FSR2
  `PrepareRgb`, HDRP pre-exposure all feed an exposed value.
- **History deringing** (FSR2): the Catmull-Rom result is clamped to its 2×2 bilinear footprint.
- **Stationary alpha**: `alpha · mix(0.5, 1, motion in pixels)` — the EMA at 0.1 lets 13.6 % of the
  8-phase jitter's first harmonic through, 0.05 half of it.
- ⚠️ **What the depth cannot see**: a SHADING change on unchanged geometry — a moving shadow, a
  light switched on, an animated texture, a reflection, a translucent surface absent from the depth
  buffer. Those FADE over the accumulation (~1/alpha frames) instead of snapping. A LARGE one is now
  caught by the large-change test below (2026-11-01); never bring back a per-pixel colour clip on still,
  unchanged geometry.
- **The LARGE-CHANGE test (owner decision 2026-11-01)** — the colour test that bullet foresaw, made
  visible by the macOS peer: after a time-of-day jump on `city` (0.55 → 0.95), the M6 took ~3 s to settle,
  and switching `TemporalAA` off alone made it final from +0.5 s (macOS-PA round 3, engine item
  `lighting-change-converges-over-frames`). The stationary alpha 0.05 is ~20 frames per e-fold: 0.2 s at
  100 FPS, ~3 s to settle at 35-45 FPS.
  - **The test**: the mean EXPOSED luminance (× the residual exposure, the history × `historyRescale`) of
    this frame's 5×5 neighbourhood against the history's, reprojected with the pixel's own motion. Above
    `TemporalAA/LargeChangeThreshold` (default **0.25**, range (0, 1]) of the larger mean, plus a 1e-3
    exposed floor for the near-black, the history is clipped like a disocclusion (the same variance box),
    ramped to full at twice the threshold so the noise cannot toggle it. The relative gap never reaches 1:
    1 switches the test off. Over 25 pixels a sub-pixel detail's jitter phase weighs 1/25 — the per-pixel
    gap that made the former colour clip shimmer. The neighbourhood statistics are NVIDIA's RELAX / ReBLUR
    (D. Zhdan, Ray Tracing Gems II, 2021), as the GI denoiser's anti-lag (§ 16).
  - **Cost**: the 5×5 box mean comes from 3×3 BILINEAR taps at −1.5, 0, +1.5 texels weighted 2, 1, 2 (a tap
    1.5 texels off the centre averages texels 1 and 2 equally): 9 fetches per image instead of 25, the exact
    mean on the frame's texel-centred taps. ⚠️ Exact only under LINEAR filtering: the history is an
    `IntermediateRenderTarget` (always linear); with `Core/Graphics/Texture/MagFilter = "nearest"` on a scene
    target the taps pick one texel of each pair, still a 9-texel neighbourhood mean. GPU time of
    `TAAEffect`, Linux RTX 3070 Ti, 1920×1080, city quay, day, no layer, profiler on, 2 alternated pairs ×
    3 windows of 6 s: test off 0.20-0.21 ms, 25 + 25 point taps 0.47-0.52 ms (rejected), **bilinear 0.30 ms**
    (+0.09 ms).
  - **Measured** (Linux, RTX 3070 Ti, `FrameRateLimit` 40 to stand in for the M6, TAA on, exposure pinned
    f/8 · 1/125 · ISO 25600, the land region's 8-bit mean after the jump):

    | Run | +0.2 s | +1 s | +2 s | +3 s |
    |---|---|---|---|---|
    | GI off, test off | 240 | 127 | 38 | 20 |
    | GI off, test on | 16 | 15.2 | 14.9 | 14.9 (final) |
    | Default (RTGI), test off | 247 | 170 | 57 | 23 |
    | Default (RTGI), test on | 230 | 66 | 22 | 17 |

    With RTGI on, what remains is the GI denoiser's own history (engine item
    `lighting-change-converges-over-frames`).
  - **Parked shimmer** (16 consecutive frames, per-pixel 8-bit peak-to-peak, y < 1400): `relief` mean 0.61 /
    0.01 % > 8 / max 38 (off) against 0.61 / 0.009 % / max 27 (on) — unchanged. `city` at the quay (day):
    façade 0.19 / 0.18, sky 0.04 / 0.04, trees 1.26 / 1.35 (> 8: 1.40 / 1.82 %, the wind); the SEA 22 → 34. The
    sea is translucent, absent from the depth buffer: TAA used to average its sun glints over ~20 frames,
    the test lets them live. **Owner verdict 2026-11-01, eye A/B at the quay: "sur B les reflets sont plus
    nets" — B validated, the test on by default.**
  - ⚠️ **It unmasks what TAA used to average away**: three near-black frames (~75 ms) right after a
    time-of-day jump, with `IndirectDiffuse` off, one jump out of two on Linux (macOS-PA saw one with TAA off).
    They are upstream of TAA: engine item `time-jump-renders-black-frames`.
- **Cross-OS (2026-09-23)**: macOS (M2, TAA forced on) whole frame > 8 = 0.01 %, max 11 — the Linux
  figure; Windows (RTX 3060, 720p, RayTracing lane) 0.30 % against 1.21 % with its TAA OFF; Linux in
  the RayTracing lane 0.27 %. The ScreenSpace/RayTracing gap is the lane, not the machine. ⚠️ OPEN: in
  the RayTracing lane the far band stays 1.9 % > 8 with TAA against 0.02 % without — the half-res
  traced effects vary with the jitter phase and the EMA does not fully average it (was 65 % before the
  fix; not visible to the owner). Both Windows and macOS had `TemporalAA/Enabled = false` in their
  settings: that, not the machine, is why they "did not shimmer".
- **Debug view of the resolve's decision (2026-09-28)**: `Core/Graphics/PostProcessing/TemporalAA/DebugView`, read
  at creation (0 = off). **1** red = history rejected (clipped, by the depth or by the motion marker), green =
  what the velocity dilation changed (the nearest neighbour's velocity against the pixel's own, 1 px = full),
  blue = reprojection length (8 px = full); **2** the history depth nearer (red) or farther (blue) than the 3×3
  range, green = the history carries the motion marker, yellow = marked and rejected; **3** the exposed luminance gap
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
- **The motion marker (owner decision 2026-09-28, "option B")** answers it without a colour test. The history
  alpha is the SIGNED linear depth: **negative = the history holds moving content**. Written negative where the
  dilation brought a FOREIGN motion (the nearest neighbour's velocity more than 0.5 px away from the pixel's
  own), and kept negative while the pixel stays on an EDGE (3×3 depth range above 5 %) — between two logic
  ticks nothing moves, so the mark must survive them. A marked history whose 3×3 is no longer an edge has lost
  its object: rejected (clipped) like a disocclusion. A still scene never writes a mark, so the rule above (no
  colour clip on unchanged geometry) holds by construction. ⚠️ The alpha is read with a `textureGather` and
  blended by hand on the MAGNITUDES (the exact bilinear read of an unsigned depth): a filtered signed value
  would average a marked and an unmarked texel of the same surface towards 0 and reject it. Nothing downstream
  reads the TAA output's alpha (the DoF's `extractFromAlpha` reads its OWN setup target).
  Measured (Linux, RTX 3070 Ti, 2880×1620, 16 frames, exposure pinned):
  - `animation-debug` (lane `None`, DebugView 3, 1-2 px crests of the kept-history gap on the ground): the fans of
    1-px lines behind both swords are gone; crests 11 500-12 400 → 6 800-8 600 per frame (the rest is ground
    texture and actor outlines).
  - `relief` parked (f/8 · 1/125 · ISO 100, ScreenSpace): mean 0.436, 0.00 % > 8, max 7 — unchanged.
  - `forest --demo-options 0,0,0`, parked, wind set to 0 for the bench: mean 1.180 / 1.83 % > 8 / max 116
    against 1.176 / 1.82 % / 119 — unchanged on thousands of silhouettes (TAA off: 2.48 / 8.3 %, the scene has
    temporal noise of its own).
  - Same forest in its wind: canopy gradient 16.3 against 15.5 (TAA off 41.8), temporal ptp 9.96 against 8.34 —
    the image follows the motion a little better; the canopy stays softer than with the TAA off (wind motion
    stays under the 0.5 px foreign-motion threshold).
- **Foliage in the wind — owner decision 2026-09-28: "le feuillage est bon visuellement".** No colour
  rectification while moving ("GATE") and no FSR2-style lock: both would trade against the parked shimmer, and
  the owner judges the canopy fine as it is. Do not reopen without a new owner report.
- **Trails under motion (owner report 2026-09-26, closed 2026-09-28) — what each one was:**
  - Camera turns: mostly the camera's MOTION BLUR (on by default since 2026-09-13, an owner decision): a
    camera step puts 25-30 % of the pixels near silhouettes above 8/255 for one frame with it, 5-7 % without.
  - A pure camera rotation (1°, 5°) leaves no ghost: the residual is symmetric (trailing/leading 0.59) and
    decays at 0.95 per frame — the history softened by the resampling, re-sharpened over ~10 frames.
  - A sideways step (`basic-scenery --demo-options 1`, 1 m) leaves a TAA-only residual: 5 % of the edge band
    above 8/255 right after the step, 2.3 % eight frames later (floor from the next frame with the TAA off).
    Not treated: judged acceptable after the fixes below.
  - The Paladin's sword comb: a velocity SOURCE, the skinned pose history advanced per logic tick (fixed,
    `docs/subsystems/scenes/13-instance-transforms.md`), then the 1-px lines: the motion marker above.
  - The FFT ocean's streaked whitecaps: another velocity source (`Graphics::OceanWaves` reported its current
    position as the previous one). **First check for any new trail report: does the moving thing report a
    previous position at all?** Then the debug view.
  - ⚠️ The first "provenance tag" (TAG, 2026-09-26: the history alpha = the depth its colour came from, 1 % and
    5 % separation) measured NO effect on camera steps (sideways 1 m: 2.30 / 2.31 / 2.39 % of the edge band
    above 8/255; forward 2 m: 3.18 / 3.12 / 3.15 %) — it was designed from a 1D scanline model, exact only for
    a vertical edge. Treat a model's ranking as a hypothesis to measure, never as a result.
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

### The REACTIVE mask — light the velocity cannot explain (2026-09-28)

The depth rule cannot see a SHADING change on unchanged geometry, and an additive overlay outside the depth buffer
is exactly that: a beam that re-strikes its arc draws a new shape in place. The scene target carries a reactive
mask (`R8_UNORM`, location 5, copied to the grab pass, `FrameContext::reactive`, binding 4), written by a material
with `Material::Interface::reactiveMaskExpression()`. The resolve:

- `blendAlpha = mix(blendAlpha, 1, reactive)`: the current frame wins where the mask is set;
- the history ALPHA of that pixel is written **0** — the REACTIVE tag (a linear depth is never 0; negative is still
  the motion marker). A history carrying a 0 tag is rejected UNCONDITIONALLY next frame, edge or not.

⚠️ Why a tag of its own: marking reactive pixels with the motion marker left the ghost line EXACTLY on the horizon —
the marker is deliberately kept on a depth edge (the sword-comb fix), and the horizon is one. Measured on `beams`:
before the mask, twelve re-strikes a second averaged into a straight white line between the arc's fixed ends; with
the zero tag, TAA on and TAA off leave the same persistent light (20 116 vs 20 461 pixels lit in all of 5 captures).
Details: `docs/subsystems/graphics/33-beams-lasers-and-electric-arcs.md`.

⚠️⚠️ **Two rules found by the macOS peer (2026-09-29), both needed** — the first version protected the beam's CURRENT
pixels only, and a sweeping laser left a fan of red streaks under TAA (macOS, then reproduced on Linux on the
ScreenSpace lane; the 2026-09-28 measurement above looked at the arc only and could not see it):
1. **The mask is read over the SAME 3x3 footprint as the reconstruction** (max of the 9 taps): the resolve's current
   colour is a Mitchell-Netravali blend of the 3x3, so a pixel NEXT to the beam borrows its light; read at the centre
   only, that pixel accumulated the light into an untagged history nothing ever rejected.
2. **A reactive-tagged history is REPLACED (blendAlpha = 1), not clipped**: a pixel next to the beam's new position has
   the beam in its 3x3 neighbourhood, the variance box spans it, and the old light survived the clip.
Validated: 10 captures over a full turn of the laser (ScreenSpace lane, TAA on), no streak, no trail, 0 validation
message.

**A region, not only a material, can set the mask (2026-10-13)**: `SceneInstanceTransforms::invalidateTemporalHistory()`
declares a world box whose history is invalid this frame; `Renderer::markInvalidTemporalHistory()` clears its screen
footprint to 1 in the reactive attachment at the end of the scene pass. The case: a path whose point count changed —
its OLD shape sat inside the ±1 % depth acceptance (a ribbon a few centimetres above the ground) and stayed as a
ghost for ~30 frames (2 565 pixels; 0 after). Graphics doc 35 § The point-count change.
