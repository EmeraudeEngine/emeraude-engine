---
id: material-source-channel-ignored-by-hard-coded-components
title: SourceChannel is parsed then silently ignored by AO, opacity, height and other components
status: in-progress
priority: high
scope: Graphics/Material (StandardResource codegen), shadow pass, RT alpha test
opened: 2026-10-20
tags: [material, shaders, texture-packing]
---

# SourceChannel is silently ignored by most components

## Why

A defect: the JSON key `SourceChannel` (and the `Component::Texture::sourceChannel()` it fills) is accepted on
every texture component, but the code generation honours it only for roughness, metalness, specular, clear coat,
clear-coat roughness and sheen roughness. Elsewhere the swizzle is hard-coded and the setting is dropped without a
word. The texture packing (graphics § 39, decision D3) needs every component to read its channel.

## What remains

- DONE on Linux (2026-10-20, uncommitted): every scalar component of the code generation reads
  `sourceChannelSwizzle()` (AO, opacity, height in the fragment parallax and the mesh stage, reflectivity, subsurface,
  subsurface thickness, transmission, iridescence, the two thickness maps); the shadow pass reads the opacity
  component's channel; a component that names no channel reads `StandardResource::defaultSourceChannel()` (green for
  the two thickness maps — the setters and the JSON path, through the new `Texture` JSON-constructor argument).
  Proof in graphics § 08-8 (relief, per pixel: packed 0.72 / 1.07 vs the 0.99 launch-to-launch noise; without the
  key 8.73). Gate: builds GCC + clang 0 / 0, cppcheck 0, IWYU clean, clang-tidy 0 new (the `Texture` JSON
  constructor split, cognitive complexity 43 → under 25).
- The RT alpha test (`RTAlphaTestGLSL.hpp`, opacity `.r`): carried by `rt-material-packed-channels`.
- Validation by the peers (macOS, Windows) after the push; MSVC sees the new forward declaration
  `enum class FillingType : uint32_t;` in `Component/Texture.hpp` (the definition carries `EMEN_API`).

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `08-8-material-component-system.md`.
