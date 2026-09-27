## 12. Post-Processing Effects

### RTR shades its hits with the EFFECTIVE ambient, not the LightSet value (Aug 2026)

> [!CAUTION]
> **`LightSet::ambientLightIntensity()` is NOT what the raster shades with.** When the sky drives
> the ambient (`applyAmbient`), the ambient pass reads the baked irradiance cubemap and the scalar
> pushed to the view UBOs is ZERO — the LightSet keeps the manifest's value (17 000 lx for a
> daylight sky) for whoever reads the sky's photometry. RTR shaded its hit points with the
> LightSet value, so under a sky-driven scene every reflection carried a flat 17 000 lx ambient
> ON TOP of its IBL term — the reflected world was brighter than the world it reflected, which
> breaks the "the reflection matches the raster" contract.
>
> `Scene::effectiveAmbientIlluminance()` is the single site of the rule (the UBO refresh reads it
> too), and it reaches the effects as `FrameContext::ambientIlluminance`. **Anything that SHADES
> its own hit points must read that field, never the LightSet.** RTR's IBL term at hits stays: it
> is its estimate of the indirect light at points RTGI cannot reach, and removing it would make
> the reflections too dark instead of too bright.
