---
id: rt-blended-materials-in-tlas
title: Blended (alphaMode BLEND) materials enter the TLAS as opaque instances
status: open
priority: unranked
scope: Scenes/SceneMetaData, Graphics/Effects/Framebuffer (RTGI, RTAO, RTR, ContactShadows)
opened: 2026-08-30
tags: [ray-tracing, transparency, measurement]
---

# Blended (alphaMode BLEND) materials enter the TLAS as opaque instances

## Why

`SceneMetaData` flags an instance `FORCE_NO_OPAQUE` only when a sub-geometry is ALPHA-TESTED. A
BLEND material (Sponza's `dirt_decal` quads at opacity 0.35, its ivy) is an opaque triangle for
every ray: GI bounce rays and AO/shadow rays hit the whole quad, transparent texels included, and
the RTGI bounce shades the hit with the decal's material (glTF-default metal → black bounce).

## What remains

1. Measure on Sponza (`post-processor-effect-debug` cannot: no blended object yet — add one) how
   much the decal quads darken RTGI bounces and RTAO around them.
2. Decide: treat BLEND as a cutout for rays (`FORCE_NO_OPAQUE` + the shared alpha-test rule at 0.5,
   the industry default), or exclude blended renderables from the TLAS.

## ⚠️ Traps

- ⚠️ The raster side was fixed separately (the albedo attachment is opacity-blended, Aug 2026);
  the RT side is a different path with a different rule. Both must agree on what a 35 % decal is.

## References

- `src/Scenes/SceneMetaData.cpp` (instance flags), `src/Graphics/Effects/Framebuffer/RTAlphaTestGLSL.hpp`.
- `docs/caution-points.md` § "big dark squares under Sponza's decals".
