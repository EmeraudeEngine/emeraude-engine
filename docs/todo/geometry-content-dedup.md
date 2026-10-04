---
id: geometry-content-dedup
title: Loaders share identical geometry by content — JungleRuins built 6 trees 65 times (23 GB)
status: in-progress
priority: unranked
scope: src/Scenes/Loaders
opened: 2026-10-04
tags: [memory, loaders, usd, gpu]
---

# Loaders share identical geometry by content

## Why

JungleRuins option 5 lost its device (NVIDIA Xid 109) on the RTX 3070 Ti, with or without ray tracing. VMA's report
at the end of the load (2026-10-04): 28.9 GB allocated, the 8 GB heap full at its budget and 22 GB placed in system
memory. 23 GB of it are DUPLICATES: in `PI_S_QueenForest` and `PI_S_RiverForest`, 195 PointInstancers each point at
one of 3 tree prototypes, and the loader built one geometry per instancer — 6 vertex buffers × 65 copies (21.4 GB)
and their index buffers × 65 (1.75 GB). Without them the scene needs about 5.8 GB.

## Owner decisions (2026-10-04)

- By CONTENT, in the engine (not by USD source identity, which composition loses).
- In a helper the LOADERS call where they create a geometry — not in `SceneDataConsumer`, which runs after the
  geometries are created and uploaded (the peak and the system-memory placements would remain).

## What remains

Done for USD 2026-10-04 (`Scenes/Loaders/GeometryDeduplicator`: key = vertex and triangle counts + a 64-bit content
hash; a mesh with the same geometry, material and sides shares its renderable too). JungleRuins option 5: 381 forest
meshes share, GPU memory 28.7 → 5.57 GB; WorldLobby and option 1 unchanged (nothing to share, same counts).

- glTF, FBX, WAD through the same call (measure on an asset that repeats a mesh first: a glTF that instances by
  duplicating nodes, FBX duplicated meshes).

## References

- `docs/scene-loaders-usd.md` § the JungleRuins re-measure; item `jungle-ruins-fence-timeout-abort`.
