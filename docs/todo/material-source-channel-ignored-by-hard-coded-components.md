---
id: material-source-channel-ignored-by-hard-coded-components
title: SourceChannel is parsed then silently ignored by AO, opacity, height and other components
status: open
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

- `StandardResource::generateFragmentShaderCode()`: use `sourceChannelSwizzle()` for AO (`.r`), opacity (`.r`),
  height / displacement (`.r`, also the mesh stage), subsurface, subsurface thickness, transmission, iridescence,
  reflectivity map (`.r`), iridescence thickness and volume thickness (`.g` — keep that as the glTF default).
- The shadow pass hard-codes `r` (opacity) / `a` (albedo fallback): follow the alpha-source component's channel.
- The RT alpha test (`RTAlphaTestGLSL.hpp`) reads opacity `.r`: carried by `rt-material-packed-channels`.
- Runtime proof: a material reading AO / opacity / height from a non-red channel renders like the unpacked one.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `08-8-material-component-system.md`.
