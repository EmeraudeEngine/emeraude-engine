---
id: atmospheric-fog-lit-by-a-disabled-sun
title: AtmosphericFog keeps lighting the fog with a main directional light that is switched off
status: open
priority: high
scope: engine Graphics/Effects/Atmosphere/AtmosphericFog.cpp
opened: 2026-10-26
tags: [post-process, fog, celestial-clock, night]
---

# AtmosphericFog keeps lighting the fog with a main directional light that is switched off

## Why

`AtmosphericFog::recordOverlayPasses()` reads `mainLight->illuminance()` and `emissionChromaticity()` without checking
`isEnabled()`. A celestial clock switches its sun OFF at sunset and leaves its last illuminance and red chromaticity on
it, so a scene with this fog would keep a sunset-coloured inscatter all night. VolumetricLight had exactly this defect
(a saturated red veil after sunset in WaterWorld, fixed 2026-10-26); VolumetricScattering, VolumetricClouds and
LensFlare already check (graphics doc 13-12 § 03, "A DISABLED main light lights nothing").

Not observed yet: WaterWorld's stack has no AtmosphericFog. Found by reading the five effects that consume the main
light.

## What remains

- Take a zero fog luminance when the light is disabled (the extinction still applies: a sunless fog only
  attenuates). The look of a night with fog changes in every scene whose clock switches its sun off: owner check.
- Runtime proof on a demo with AtmosphericFog and a celestial clock, before / after the sunset, 0 VUID.

## References

- `src/Graphics/Effects/Atmosphere/VolumetricLight.cpp` (the fix), `VolumetricScattering.cpp` (the pattern).
