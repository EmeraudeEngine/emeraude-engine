---
id: uv-transform-slots-for-extension-maps
title: KHR_texture_transform reaches only six maps — the extension maps have no UV slot
status: open
priority: medium
scope: Graphics/Material
opened: 2026-09-14
blocked-by: []
tags: [gltf, material, measured]
---

# `KHR_texture_transform` reaches only six maps

## Why

The material UBO carries **six** UV transform slots — albedo, roughness, metalness, normal, ambient
occlusion, emissive — one scale/offset vec4 and one (cos, sin) vec4 each. Every other component
type falls back to the plain texture coordinates, so a transform declared on one of those maps is
parsed, logged and dropped.

That was harmless while those maps were unread. It is not any more: **three extensions now read
maps that have no slot**, and one conformance model fails on it alone.

| map | component type | asset that needs it |
|---|---|---|
| `sheenColorTexture`, `sheenRoughnessTexture` | `Sheen`, `SheenRoughness` | **`SheenCloth`** |
| `specularTexture`, `specularColorTexture` | `Specular`, `SpecularColor` | none in the bench |
| `clearcoatTexture`, `clearcoatRoughnessTexture`, `clearcoatNormalTexture` | `ClearCoat`, `ClearCoatRoughness`, `ClearCoatNormal` | none in the bench |

## Measured, 2026-09-14

`SheenCloth` tiles its 256×256 maps **30 times in U and V** (`KHR_texture_transform`,
`scale: [30, -30]`, declared on both of its sheen textures, which are the same image). With the maps
wired but untransformed, its sheen reads at **1/30** of the authored frequency: the cloth changes on
13.1 % of its pixels and its mean drops 158.2 → 80.7, so the maps demonstrably reach the shader —
but the pattern is a large-scale wash instead of the fabric's weave. It is the only thing left
between that model and a PASS.

## What remains — a DESIGN decision first

- [ ] Decide how the slots are addressed before adding any. Three shapes, and the difference
      matters more than the byte count:
      1. **Grow the fixed table** to cover the extension maps. Simple, but the material UBO grows by
         two vec4 per map added (the block went 320 → 416 bytes for the rotation alone, and that
         took the shared-UBO bank from 204 to 146 seats — banks grow on demand since 2026-09-14, so
         this is no longer fatal, only wasteful).
      2. **One transform per TEXTURE rather than per component type**, deduplicated: assets reuse
         the same transform across maps (SheenCloth uses one for both of its sheen textures), so a
         small indexed table plus a per-component index would cover far more with far less.
      3. **Only where an asset asks**, resolved at material build time — the codegen already emits
         per-material GLSL, so a material that transforms nothing pays nothing.
- [ ] ⚠️ Whichever is chosen, `transformedTexCoords()` currently takes a `ComponentType` and maps it
      to a fixed slot. That signature is the thing to change; the loaders' warnings
      (`GLTFLoader.cpp:1295` and the sheen/clearcoat ones next to it) are the list of what must stop
      being dropped.
- [ ] The per-`TextureInfo` **`texCoord` override** (the multi-UV gap) is a DIFFERENT problem that
      also blocks here — it needs a second UV set on the geometry, and walls into
      [`vertex-attribute-presence-belongs-to-geometry.md`](vertex-attribute-presence-belongs-to-geometry.md).

## References

- `src/Graphics/Material/StandardResource.cpp` — `transformedTexCoords()` and the six-slot table.
- Capture: `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260914-v2/SheenCloth_three-qtr.png`.
