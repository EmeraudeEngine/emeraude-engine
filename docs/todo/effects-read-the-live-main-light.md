---
id: effects-read-the-live-main-light
title: Six post-process effects read the main directional light live, not from the frame's render state
status: open
priority: high
scope: Graphics/Effects (Atmosphere, Camera, Lighting), Scenes/LightSet
opened: 2026-11-03
tags: [defect, render-state, threading, lighting, threadsanitizer]
---

# Six post-process effects read the main directional light live, not from the frame's render state

## Why

The render thread must read the frame's PUBLISHED state (the latched render state slot), never the live state the
logic or main thread is changing (scenes doc 16). `LightSet::mainDirectionalLight()` hands the live light object, and
six effects read its enabled flag, direction and illuminance from it while recording a frame:

| Effect | Read |
|---|---|
| `Effects/Atmosphere/VolumetricClouds.cpp` (~982) | `mainLight->isEnabled()` |
| `Effects/Atmosphere/VolumetricLight.cpp` (~418) | `isEnabled()`, `intensity()` |
| `Effects/Atmosphere/VolumetricScattering.cpp` (~443) | `isEnabled()`, `illuminance()` |
| `Effects/Atmosphere/AtmosphericFog.cpp` (~331) | `isEnabled()`, `illuminance()` |
| `Effects/Camera/LensFlare.cpp` (~438) | `isEnabled()` (and the light's direction) |
| `Effects/Lighting/SSContactShadows.cpp` (`recordPreDenoisePasses()`) | `direction()` |

A time-of-day jump (`CelestialClock::apply()` on the main thread) or any switch can therefore land in these effects one
frame before the rest of the frame — the defect fixed for the light lists on 2026-11-03
(`AbstractLightEmitter::publishedEnabled()`, graphics doc 13-12 § 43, where it fed TAA an unmarked frame of day sky
over a sunless land). Found while inventorying the live reads; not measured in a scene that runs these effects.

The sixth, `SSContactShadows`, was not in the inventory: ThreadSanitizer reported it on 2026-11-27 (macOS, `city`, one
launch of two, 2 reports) — the render thread in `DirectionalLight::direction()` from
`SSContactShadows::recordPreDenoisePasses()`, the logic thread in `DirectionalLight::setDirection()` from
`DirectionalLight::move()` (the celestial clock's pivot moving), which holds two mutexes the reader does not take.
A data race, so a defect, on top of the frame that leads.

## What remains

- Decide the source (owner): the main light's `DirectionalLight::publishedEmission(readStateIndex)` (chromaticity,
  direction, intensity, whatever its block layout — the RT light SSBO reads it since 2026-11-11, scenes doc 16) plus
  `publishedEnabled()`, or a published main-light record of the LightSet itself (which light is "main" may change
  too).
- Convert the six effects, then a `temporalCapture(N, triggerFrame, command)` jump in a scene with fog and volumetric
  light (TAA off) to prove no frame leads.

## References

- Scenes doc 16 (the published enabled state; the RT light SSBO, the same class, closed 2026-11-11), graphics doc
  13-12 § 43.
