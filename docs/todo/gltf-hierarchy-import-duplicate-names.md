---
id: gltf-hierarchy-import-duplicate-names
title: The node-HIERARCHY import path may drop nodes with duplicate names — verify by entity count
status: open
priority: high
scope: Scenes/Loaders (SceneDataConsumer), Scenes/Node (createChild)
opened: 2026-09-15
tags: [gltf, scene, defect, unverified]
---

# The node-HIERARCHY import path may drop nodes with duplicate names

## Why

`TransmissionTest` lost two of its twelve spheres because three nodes named `BlueTransWithMask` collided in a registry
keyed by name. The fix covers the flat path. `SceneDataConsumer` also builds children with
`createChild(nodeDesc.name)` (the `+ModelViewer` file path, `AssetRoot`). Whether that registry tolerates a duplicate
name is UNVERIFIED.

## What remains

- Measure it the same way: the engine's entity count against the asset's mesh-bearing node count
  (`Core.SceneManagerService.getSceneInfo()`) on an asset with duplicate node names, loaded through `+ModelViewer`.
- If nodes are lost, key the registry by identity, not by name, as the flat path does.

## References

- `docs/subsystems/scenes-loaders/07-gltf-conformance-bench-results.md` § The structural control; split out of the
  former item `gltf-conformance-loader-gaps` (2026-10-11).
