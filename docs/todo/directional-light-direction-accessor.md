---
id: directional-light-direction-accessor
title: DirectionalLight::direction() reads the UBO lane — invalid before the first update
status: open
priority: unranked
scope: Scenes/Component/DirectionalLight
opened: 2026-08-30
tags: [api, lighting]
---

# DirectionalLight::direction() reads the UBO lane — invalid before the first update

## Why

`direction()` returns `m_buffer[DirectionOffset…]`, the value staged for the GPU by the last update
(`m_useDirectionVector ? forward : -position.normalized()`). Read right after creation — a demo's
`onBuilding()` / `onEnabled()` — it returns the buffer's initial (0, 1, 0). The bench had to own the
direction it created the light with (`PostProcessorEffectDebug`, Aug 2026).

## What remains

Compute the direction on the fly in the accessor (same expression as the update), or document the
accessor as GPU-state-only and add a `computedDirection()`.

## References

- `src/Scenes/Component/DirectionalLight.cpp` lines ~280 and ~409 (the two update sites).
