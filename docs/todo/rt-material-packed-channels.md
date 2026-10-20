---
id: rt-material-packed-channels
title: RT material data — carry the channel of every packed texture (opacity, AO, metalness…)
status: in-progress
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

- DONE on Linux (2026-10-20, uncommitted): the opacity channel — `GPURTMaterialData::OpacityChannelShift` (flags
  bits 20-21), filled by `SceneMetaData`, read by the shared alpha test (`RTOpacityChannelShift`). Without it a packed
  opacity (the albedo's A) was read as the albedo's RED: Grid003 +2.7 brighter under RT (graphics § 39).
- Any other packed role RT reads gets the same treatment when it is packed: today RT reads albedo, normal, roughness
  / metalness (channels already carried), emission, opacity — none other is packed.
- The UV transform parity (RT uses the raw `hitUV`) is a separate known gap.
- Validation by the peers (macOS, Windows).

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `material-source-channel-ignored-by-hard-coded-components`.
