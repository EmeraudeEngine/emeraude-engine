## 12. Post-Processing Effects

### LensFlare — the source is probed in the depth buffer (Aug 2026)

The flare knew only whether the main directional light was INSIDE the frustum (`lightOnScreen`,
CPU side, with an edge fade): a sun behind a wall, but in the field of view, produced its ghosts
and halo as if it were visible — owner report on Sponza, "il passe à travers la géométrie".

- **The probe**: the ghost + halo pass (binding 1 of its own 2-sampler layout, per-frame set) reads
  the scene depth on a 16-tap Vogel disk around the projected light — radius
  `Parameters::occlusionRadius` 0.012 of the screen HEIGHT, scaled per axis so the disk is round —
  and counts the taps that read the far plane (`>= 0.99999`). A directional source is at
  infinity: anything written in the depth buffer there hides it. Fixed rotation (no shimmer); the
  result multiplies the whole flare; `requiresDepth()` is true so the depth is always bound.
- **Verified on the bench** (`post-processor-effect-debug`, bench 1: white open-roof room, the
  "AutumnFieldPureSky" sun created by hand on its manifest direction, the fixed camera aimed 15°
  below it, a 1 m cube on the line of sight on option): a diagnostic tint of the visibility read
  **+21/255 on the neutral walls with the sun uncovered, 0.00 with the cube** — the probe sees the
  occluder. A skybox is NOT geometry here (the sky region reads the far plane).
- ⚠️⚠️ **The threshold is EXPOSURE-RELATIVE (fixed 2026-09-24, owner decision over a nits threshold
  like the VeilingGlare's)**: the bright pass compares `max(r, g, b) × FrameContext::displayExposure`
  (nit → display value; 1 without a tone mapper) with `Parameters::threshold` = **4** display units
  (1 = the sensor's white). It was 0.8 compared with NITS, a leftover of the display-referred era: on
  `forest` the whole 15 380-nit sky fed the ghosts, split into saturated rainbow streaks through the
  leaves. Measured at the sun pose (`setPosition(30, 0, -40)` + `lookAt(100.5, 48.6, 11.7)`): 0.8
  display still streaks, 2 faintly, 4 and 8 are clean. ⚠️ And at 4 there is NO flare by day there:
  the only sun on screen is the one PAINTED in the HDRI, clipped, under 4 once exposed (open-sky pose:
  the flare adds 0.02/255) — the analytic light is not in the image. A nits threshold was refused
  because a daylight sky exceeds any sensible value of it (the VeilingGlare's 1000 nits included).

#### The ghosts and the analytic sun — the Sep 2026 rewrite (owner decision)

**Why:** the former "ghosts" read the bright image at a FIXED distance from the light (0.3, 0.6, …)
in each pixel's own direction: every pixel of a radial line read the same value — radial STREAKS (the
rainbow bands through the leaves), and a point-like sun, sampled at distance 0, made nothing. With the
threshold fixed, a daylight scene had no flare at all.

**Now, two half-resolution passes:**

1. **Source** (`LF_Source_FS`): the bright pass (display-unit threshold) PLUS the **analytic sun** — a
   soft disc (`1 - smoothstep(0.6, 1, r)`, area 0.648 π r²) at the main directional light's projected
   position, radius `sunDiscRadius` 0.015 of the screen height (it stands for the defocused source,
   wider than the real 0.27°), carrying the light's ILLUMINANCE spread over its solid angle (flux
   conserved), times its visibility: the 16-tap depth probe × the clouds' transmittance, evaluated only
   inside the disc, and the edge fade. No main light (a night scene): the image's bright pixels alone.
2. **Ghosts** (`LF_Ghost_FS`): John Chapman's pseudo lens flare (2013) — the image mirrored through the
   centre, `ghostCount` 6 samples marching toward it (`ghostDispersal` 0.35), a halo ring of radius
   `haloWidth` 0.45 (round on screen), a chromatic distortion (0.004 UV), weighted by the distance of
   the SOURCE to the centre with a GENTLE power 2 (Chapman's 10 left a sun a quarter of the screen
   off-centre 1 % of its ghosts; the reflectance already damps the rest). `intensity` 5e-4 plays the
   lens reflectance (owner kept it: discreet, "a real coated optic"); only a source far above the
   exposure leaves a visible ghost. The combine adds the ghosts with no global factor any more.

⚠️⚠️ **Three traps met on the way, each invisible until measured:**
- **A half float stops at 65 504.** The disc carries ~5e7 nits; written in nits the source target
  overflowed to +inf, then NaN once filtered, and NO ghost ever showed, without any validation error.
  The source pass works in DISPLAY units (nits × exposure, ~5 000 for the sun, clamped at
  `MaxSunDisplayLuminance` 50 000); the ghost pass returns to nits (× `inverseExposure`).
- **Chapman's sum does not conserve the flux.** Ghost i is the source magnified 1 / |1 - dispersal·i|
  times with its FULL luminance: at 0.35 the fourth ghost was 20× the sun (400× its flux) — a white disc
  over half the frame. Each ghost is weighted by (1 - dispersal·i)²; the halo, which spreads the disc
  over a circumference (~120 disc areas), by `haloIntensity` 0.01.
- **A sun at the centre of a washed-out frame hides its ghosts** (they converge on it, and the frame is
  saturated there): judge the flare with the sun OFF-centre over a sky that is not white — `forest`,
  `setPosition(0, 124, 0)` + `lookAt(49.8, 172.6, 79.9)`: five ghosts on the axis, the two central
  ones clearly iridescent, +10 to 15 levels on a 240/255 sky; behind `Cloud145` they fade out.
- ⚠️ A modeled sun DISC (an emissive quad) is geometry: the probe would call it an occluder. A
  directional light's disc belongs in the sky cubemap.
