---
id: directional-light-direction-accessor
title: DirectionalLight::direction() reads the UBO lane — invalid before the first update
status: open
priority: high
scope: Scenes/Component/DirectionalLight
opened: 2026-08-30
tags: [api, lighting]
---

# DirectionalLight::direction() reads the UBO lane — invalid before the first update

## Why

`direction()` returns `m_buffer[DirectionOffset…]` (`DirectionalLight.hpp:303-305`), the value staged for the GPU by
the last update (`m_useDirectionVector ? forward : -position.normalized()`). Read right after creation — a demo's
`onBuilding()` / `onEnabled()` — it returns the buffer's initial (0, 1, 0) (`DirectionalLight.hpp:597`). The bench had to own the
direction it created the light with (`PostProcessorEffectDebug`, Aug 2026).

## What remains

The expression already exists as a public method: `resolveDirection(worldCoordinates)`
(`DirectionalLight.cpp:86-104`, since `c31a1970a`, null-position guard included), and `setDirection()`
(`:109-116`) writes it to the buffer. What remains:

- [ ] Make `direction()` return `resolveDirection(this->getWorldCoordinates())`, or document it as GPU-state-only
      and point the callers to `resolveDirection()`.
- [ ] Then let the bench read the light instead of owning its constants: projet-alpha
      `src/Builtin/PostProcessorEffectDebug.cpp:306` and `:361`.

## References

- `src/Scenes/Component/DirectionalLight.cpp:163` (`move()`) and `:255` (`createOnHardware()`): the two update
  sites.
