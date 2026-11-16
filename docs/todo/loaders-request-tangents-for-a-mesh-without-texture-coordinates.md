---
id: loaders-request-tangents-for-a-mesh-without-texture-coordinates
title: The scene loaders ask for tangents on a mesh without texture coordinates — an error line, an ignored refusal, a tangent-space vertex format for nothing
status: open
priority: high
scope: Scenes/Loaders (USDLoader, FBXLoader, GLTFLoader — the geometry creation functions), emeraude-base VertexFactory::Shape (computeTriangleTangent(): its message names no shape)
opened: 2026-11-16
tags: [defect, loaders, measured]
---

# The scene loaders ask for tangents on a mesh without texture coordinates — an error line, an ignored refusal, a tangent-space vertex format for nothing

## Why

The testbed's `world-lobby` traces one error while its USD asset loads (seen in the ThreadSanitizer sweep of
2026-11-16, right after the USD parser's lines; the owner ranked it a defect):

`[Error][VertexFactory::Shape] Shape::computeTriangleTangent(), there is no texture coordinates !`

The three loaders create a geometry the same way: a creation function calls `shape->computeTriangleTangent()` then
`shape->computeVertexTangent()` and loads the shape, with the flags `EnableTangentSpace |
EnablePrimaryTextureCoordinates` — whatever the mesh holds.

| Loader | Call site | Condition |
|---|---|---|
| `USDLoader.cpp` | the finest level's creation function, and each coarser level's in `buildLevelsOfDetail()` | none |
| `FBXLoader.cpp` | the geometry's creation function | none |
| `GLTFLoader.cpp` | the geometry's creation function | skipped only when every primitive authored its tangents |

For a mesh WITHOUT texture coordinates the base refuses (a tangent is the direction of U) and says so; the loader
ignores the `bool` it returns and goes on: the geometry is loaded with the default tangent of every vertex, in a
tangent-space vertex format — nine floats per vertex that carry nothing — and the journal holds an error for an asset
that is valid.

## What remains

- **The intent for a mesh without texture coordinates — an owner decision**: e.g. a geometry WITHOUT the tangent space
  (normals only: the flags follow what the shape holds), no tangent request, no error; or keep one vertex format for
  every mesh of an asset and make the request conditional. A material that samples a normal map needs texture
  coordinates anyway.
- Check the return value where a tangent IS requested (a refusal for another reason — an inconsistent shape — must fail
  the geometry's creation, not load it half-built).
- The base's message names no shape: with several hundred meshes in an asset the journal does not say which one.
  Naming it needs the caller's context (the base only has the shape) — decide where the name is added.
- Identify the mesh of `world-lobby`'s asset and whether other assets of the stores do the same (a sweep of the
  loaders' journals).
- Proof: `world-lobby` loads with 0 error line, the same picture; the three loaders' fuzz targets still pass
  (`fuzz_usd`, `fuzz_fbx`, `fuzz_gltf` call the same pair).

## ⚠️ Traps

- glTF: authored tangents must be USED (their handedness is lost by a recomputation): the existing skip stays.
- The coarser levels of detail of the USD loader go through the same pair: a fix in one creation function leaves the
  other.

## References

- Base `docs/subsystems/vertexfactory/07-critical-attention-points.md` (the tangent frame, `setTangent()`'s W).
- Scenes doc 19 (the loaders).
