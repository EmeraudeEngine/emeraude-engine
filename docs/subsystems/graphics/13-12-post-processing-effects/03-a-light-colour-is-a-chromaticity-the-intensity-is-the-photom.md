## 12. Post-Processing Effects

### A light colour is a CHROMATICITY — the intensity is the photometric quantity (2026-09-25)

`AbstractLightEmitter::setColor()` and `LightSet::setAmbientLightColor()` store the authored colour and send the GPU
its **unit-luminance chromaticity** (`Color< float >::unitLuminanceChromaticity()`: the RAW components divided by
their Rec.709 luminance, no sRGB decode — like every colour constant of the engine; black gives zero). So
`intensity × chromaticity` delivers exactly the lux or candela set, whatever the hue (owner decisions 2026-09-25).
Before, the colour multiplied the intensity as is: an orange (255, 140, 40) emitted 38 % less than white, a DarkBlue
ambient at 5000 lx delivered 120 lx.
- **One writer per consumer family, all fed the chromaticity**: the classic light UBO (`onColorChange()`), the CSM
  block (`DirectionalLight::updateCascades()` — the second directional writer), the RT light SSBO (`LightSet`
  packer → RTGI, RTR, the probes), the five effects (VolumetricScattering, VolumetricClouds, LensFlare,
  AtmosphericFog, VolumetricLight — their colour overrides are normalised the same way), the view UBOs' ambient,
  RTR's and the probes' ambient. `color()` / `ambientLightColor()` were REMOVED so the compiler lists every reader;
  `authoredColor()` / `ambientLightAuthoredColor()` are for traces and UI only.
- ⚠️ **A channel may exceed 1** (pure blue 13.85, a 2000 K sun 1.65 in red): carry it as floats, never through a
  clamping `Color< float >` (the probe-volume ambient did, capping every ambient above 1 lx at 1 — fixed, with its
  missing 1/π). The lens flare caps its sun luminance on the BRIGHTEST CHANNEL, not the scalar (half-float range).
- ⚠️ **A colour no longer dims a light.** Dim through the intensity: `LampFlicker::luminanceForHealth()` carries the
  dying-lamp dimming; the hand-lit demo ambients were migrated to the illuminance they really delivered.
- Gains versus before (1 / raw luminance): white ×1; 6500 K ×1.02; 5500 K ×1.06; 3000 K ×1.32; 2000 K ×1.65; the
  18 demo lights written `{255U, …}` were WHITE (clamped) before the same day, so they keep their brightness. The
  saturated demo LIGHTS take their nominal level — `basic-scenery` Blue ×13.85 in its channel, Red ×4.70 (and
  `simple-room`'s red spot), Green ×1.40 (the `lighten-marbles` glows), LightRed ×1.36 — owner decision
  2026-09-25: the numbers mean what they say. Only the AMBIENTS were migrated to keep their look.
- Imported glTF / USD lights keep the KHR meaning (white-equivalent intensity × colour):
  `SceneDataConsumer::attachLight()` folds the colour's luminance into the intensity, so they render as before.
