---
id: light-shadow-runtime-toggle
title: Switching a light's shadow off at runtime puts the scene IN shadow instead of out of it
status: open
priority: unranked
scope: Scenes/Component/AbstractLightEmitter, Scenes/Scene.lighting.cpp, shadow passes
opened: 2026-09-27
tags: [shadows, lights, csm, console]
---

# Switching a light's shadow off at runtime puts the scene IN shadow instead of out of it

## Why

`AbstractLightEmitter::enableShadowCasting(bool)` is public but has only ever been called with `true`, at
light creation (`DirectionalLight.cpp`, `PointLight.cpp`, `SpotLight.cpp`). Called with `false` on a
running scene, it does NOT stop the shadow: measured on `forest` (2026-09-27, validation layers on, zero
VUID), the CSM sun switched "off" darkened the whole frame — mean luminance 121 → 104 with the lighting
family off (`setLightingMode("None")`), the floor entirely shadowed, the foliage dark — and switching it
back on restored the image. The flag only gates the cascade FITTING (`Scene::updateCSMCascades()`,
`DirectionalLight::move()`) and the RT shadow-ray flag (`LightSet` `pad0`); the shadow pass keeps drawing
the map and the raster shaders keep sampling it with cascades that are no longer refreshed.

The console command that would have exposed it (`<Light>.setShadowCasting`) was withdrawn before release
for that reason.

## What remains

1. Decide whether a runtime shadow on/off is wanted at all (an owner decision: it may stay a
   creation-time property, in which case `enableShadowCasting()` should become non-public or refuse
   `false` after creation).
2. If wanted: the raster shaders must read a per-light "shadowed" flag (the light UBO) and return full
   visibility when it is off, the shadow pass must skip the light, and the CSM/light-space matrices must
   be refreshed again when it is switched back on — then add the console command back.

## ⚠️ Traps

- Compare at a PINNED exposure and look at the image: under auto-exposure a darkening can be absorbed.
- Test both lanes AND `setLightingMode("None")`: the RT lane reads the same flag through `LightSet`.

## References

- `src/Scenes/Component/AbstractLightEmitter.hpp` `enableShadowCasting()`.
- `src/Scenes/Component/LightConsoleAdapters.cpp` (the note where the command would sit).
