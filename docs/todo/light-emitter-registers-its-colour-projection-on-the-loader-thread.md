---
id: light-emitter-registers-its-colour-projection-on-the-loader-thread
title: A light emitter registers its colour projection texture from the texture's loading thread
status: open
priority: high
scope: Scenes/Component/AbstractLightEmitter (PointLight, SpotLight, DirectionalLight)
opened: 2026-11-15
tags: [concurrency, defect, code-reading]
---

# A light emitter registers its colour projection texture from the texture's loading thread

## Why

`AbstractLightEmitter` observes a colour projection texture that is not loaded yet; `onNotification()` answers
`ResourceTrait::LoadFinished` with `registerColorProjectionInBindless()`, on the thread that completed the load (a pool
worker). That function WRITES three plain members — `m_colorProjectionBindlessIndex`, `m_colorProjectionIsCubeArray`,
`m_colorProjectionFrameIndex` — and calls `requestVideoMemoryUpdate()`, while the logic thread reads them
(`PointLight::processLogics()` advances the frame index of an animated projection; the light's uniform block carries
the bindless index). The bindless set itself is locked (`BindlessTextureSet::m_access`); the emitter's members are not.

**By reading the code** (audit of the load handlers, 2026-11-15) — NOT reported by ThreadSanitizer: one run of the
testbed's `light-and-shadow-debug`, which gives its lights a projection, showed no report on this class (the textures
were loaded before the lights asked).
The observation site also has the check-then-act window: a texture that finishes loading between `isLoaded()` and
`observe()` is never registered — the light keeps no projection.

## What remains

- The contract exists since 2026-11-15 (scenes doc 12 § *A resource's load reaches a scene object on the logic
  thread*): the handler calls `Component::Abstract::recordResourceLoaded()`, `processLogics()` consumes it and
  registers on the logic thread. `LoadFailed` only traces: it can stay where it is.
- The observation site: observe, THEN test `isLoaded()` again (`Scenes/Component/CloudVolume.cpp`).
- Proof: a TSan run where a light takes a projection texture still loading: the report before, 0 after; the projection
  is drawn (a capture).

## References

- `CloudVolume::registerShape()` is the counter-example that needs nothing: its only published state is an atomic and
  the registration is idempotent — it was written to run on the loading thread.
