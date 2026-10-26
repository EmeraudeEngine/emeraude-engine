## 12. Post-Processing Effects

### DepthOfField — Production-Grade Gather DoF (Jul 2026)

7-pass physical camera DoF (technique refs: Jimenez, "Next Generation Post Processing
in COD:AW", SIGGRAPH 2014):

1. **Focus** (1x1 RG32F ping-pong): auto-focus measurement (5x5 Gaussian around screen
   center) + exponential rack focus EMA (R = focus distance, G = timestamp for the frame
   delta; manual focus pulls are smoothed too). First-frame reset flag (DONT_CARE images);
   on the reset frame the shader must NOT read the undefined history — it falls back to the
   manual focus distance (an inherited NaN would survive every later EMA and kill the DoF
   until recreation).
2. **Setup** (half-res RGBA16F): downsampled color + SIGNED thin-lens CoC in alpha
   (positive = far field, negative = near field; sky lands in the far field naturally).
   ⚠️ The alpha is the CoC radius in half-res PIXELS (`sensorFraction * targetWidth / 2`,
   clamped to ±MaxRadius at the source) — NOT a normalized fraction. Every downstream pass
   consumes pixels directly; skipping that conversion once left the physically-correct CoC
   ~80x too weak (the ~0.01 fraction was read as pixels).
3/4. **Near-CoC dilation** (H/V max filter, R16F): spreads the near coverage BEYOND the
   silhouettes — the foreground blur must bleed over the sharp background.
5. **Far gather** (half-res): golden-angle spiral disc (circular bokeh), scatter-as-gather
   weighting (a sample contributes when its own CoC reaches the shaded pixel), near-field
   samples excluded.
6. **Near gather** (half-res): same spiral driven by the DILATED near CoC, no occlusion
   rejection (foreground freely covers the background). Coverage in alpha.
7. **Composite** (full-res): sharp base → far blend by CoC factor → near OVER (bleed),
   modulated by the per-pixel material DoF mask (matprops A low nibble, HUD exemption).

Optics come from the ACTIVE CAMERA (`FrameContext::camera`); quality knobs from
`Core/Graphics/PostProcessing/DepthOfField/` settings (`MaxRadius`, `SampleCount`, `AutoFocusSpeed`,
`NearField`). `MaxRadius` (default 32, half-res pixels) is a pure performance/quality
ceiling — the blur AMOUNT is the thin-lens CoC alone, there is no scale factor
(`CoCScale` was removed 2026-07-26; a stale persisted key is ignored, but a persisted
`MaxRadius` from an older settings file still caps the blur). `NearField=false` skips
passes 3/4/6 entirely.

⚠️ **The SKY is at INFINITY** (2026-10-26): a depth of 1.0 takes the thin lens's limit A f / (fd − f), never the far plane's
distance. At the far plane the sky stood IN FRONT of a focus set beyond it: WaterWorld's GoTo on Mars (78 mm × 1000 = a
78-m focal length, f/8 — a 9.75-m aperture — focused at 1 000 km, the far plane at 600 m) drew a 38-px bokeh flower.
For an ordinary focus (10 m, 50 mm, f/8) the two give the same CoC within 2 %. ⚠️ The MANUAL focus racks like the auto
one (an exponential toward the target): a jump from 10 m to 1 000 km keeps the far field blurred for seconds — a
telescope view switches the depth of field off (`Camera::enableDepthOfField(false)`).
