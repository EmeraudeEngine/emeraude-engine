---
id: effects-read-the-live-shadow-cascades
title: Three effects read the sun's cascade matrices from the logic state, not from the frame's render state
status: open
priority: high
scope: Graphics/ViewMatricesCascadedUBO (cascadeViewProjectionMatrix(), splitDistance()), Graphics/Effects/Atmosphere (VolumetricClouds, VolumetricScattering, WaterVolume)
opened: 2026-11-28
tags: [defect, render-state, threading, shadows]
---

# Three effects read the sun's cascade matrices from the logic state, not from the frame's render state

## Why

Found on 2026-11-28 while moving the main directional light to its published record (scenes doc 16): three effects
copy the sun's cascades into their own block while recording a frame, on the render thread, with the accessors that
answer the LOGIC state:

| Effect | Read |
|---|---|
| `Effects/Atmosphere/VolumetricClouds.cpp` (`execute()`, the cascade block) | `cascadeViewProjectionMatrix(cascade)`, `splitDistance(cascade)` |
| `Effects/Atmosphere/VolumetricScattering.cpp` (`execute()`, the cascade block) | the same two |
| `Effects/Atmosphere/WaterVolume.cpp` (`bindShafts()`) | the same two |

`ViewMatricesCascadedUBO::cascadeViewProjectionMatrix(size_t)` and `splitDistance(size_t)` return
`m_logicState`, which `Scene::updateCascades()` refits on the logic thread every tick
(`DirectionalLight::updateCascades()`). So the march of these effects can use cascades one tick ahead of the shadow
map the frame samples, and the read races the write. Read in the code, not reported by ThreadSanitizer so far, not
measured on a picture.

## What remains

- `splitDistance(readStateIndex, cascadeIndex)` already answers the published state (the scene's cascade culling
  uses it). There is NO published overload of `cascadeViewProjectionMatrix()`: check what `m_renderState` holds
  (the published buffer carries the matrices the lit shaders read) and add the accessor.
- Convert the three effects (`frameView.readStateIndex()`), then a moving camera in a scene with the scattering or
  the water shafts: the shafts' shadow edges must not swim against the shadow map.
- ThreadSanitizer on `light-and-shadow-debug` (the scattering) with the camera moving.

## References

- Scenes doc 16 § *The MAIN directional light is published per render state slot* (the sibling fix).
- `src/Graphics/ViewMatricesCascadedUBO.hpp` / `.cpp`, `src/Scenes/Scene.lighting.cpp` (`updateCascades()`).
