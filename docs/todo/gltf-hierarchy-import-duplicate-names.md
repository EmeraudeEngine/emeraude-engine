---
id: gltf-hierarchy-import-duplicate-names
title: The node-HIERARCHY import path refuses sibling nodes with duplicate names and fails the whole build
status: open
priority: high
scope: Scenes/Loaders (SceneDataConsumer), Scenes/Node (createChild)
opened: 2026-09-15
tags: [gltf, scene, defect]
---

# The node-HIERARCHY import path refuses sibling nodes with duplicate names and fails the whole build

## Why

`TransmissionTest` lost two of its twelve spheres because three nodes named `BlueTransWithMask` collided in a registry
keyed by name. The fix covers the static-entity path only: it renames the duplicates
(`src/Scenes/SceneDataConsumer.cpp:444-453`, `-<nodeIndex>` suffix). The node paths (the `+ModelViewer` file path,
`AssetRoot`) still create children by name, and the code confirms the loss:
- `Node::createChild()` refuses a name already used by a sibling (`src/Scenes/Node.cpp:817-822`);
- the hierarchy path (`SceneDataConsumer.cpp:554-562`) and the flattened path (`:130-137`) then skip the node — the
  hierarchy path its whole subtree — and set `m_buildFailed`;
- the whole build then returns `false` (`:166-170`).

## What remains

- Rename the duplicates in both node paths the way the static-entity path does (`:448-453`): the first holder keeps
  its name verbatim, the next ones get the node index.
- Prove it by entity count: the engine's entity count against the asset's mesh-bearing node count
  (`Core.SceneManagerService.getSceneInfo()`) on an asset with duplicate node names, loaded through `+ModelViewer`,
  before and after.

## References

- `docs/subsystems/scenes-loaders/07-gltf-conformance-bench-results.md` § The structural control; split out of the
  former item `gltf-conformance-loader-gaps` (2026-10-11).
