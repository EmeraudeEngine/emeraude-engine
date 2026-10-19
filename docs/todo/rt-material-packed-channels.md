---
id: rt-material-packed-channels
title: RT material data — carry the channel of every packed texture (opacity, AO, metalness…)
status: open
priority: unranked
scope: Graphics/Material (GPURTMaterialData), RT shaders (RTR, RTGI, alpha test, irradiance probes)
opened: 2026-10-20
blocked-by: [material-texture-canonical-repack]
tags: [raytracing, material, texture-packing]
---

# RT parity with the packed textures

## Why

`GPURTMaterialData` (7 vec4) carries six bindless indices; only roughness and metalness have a channel field
(`RoughnessChannelShift` / `MetalnessChannelShift`). The RT alpha test reads opacity `.r` in hard. Once opacity
lives in the albedo's A and the data maps share one texture (graphics § 39), RT must read the right channels.

## What remains

- A channel field for opacity (and any other packed role RT reads); `collectRTTextures()` and `SceneMetaData`
  fill it; a shared image keeps one bindless slot (already the case: cached by texture pointer).
- Shaders: `RTAlphaTestGLSL.hpp`, `RTR.cpp`, `RTGI.cpp`, `IrradianceProbeVolume.cpp` — kept in sync with the C++
  shifts; the stride `* 7u` is hard-coded in GLSL: change it everywhere if the struct grows.
- Proof: RT reflections / GI of a packed material match the unpacked one.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `material-source-channel-ignored-by-hard-coded-components`.
