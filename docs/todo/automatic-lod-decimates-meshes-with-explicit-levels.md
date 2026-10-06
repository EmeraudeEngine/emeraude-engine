---
id: automatic-lod-decimates-meshes-with-explicit-levels
title: Automatic LOD decimates and uploads every level of a mesh that carries its own levels, then drops them all
status: open
priority: unranked
scope: Graphics/Renderable/MeshResource, MultiLayerMeshResource (onDependenciesLoaded, generateLODLevel)
opened: 2026-10-04
tags: [lod, performance, owner-decision]
---

# Automatic LOD decimates and uploads every level of a mesh that carries its own levels, then drops them all

## Why

`Core/Graphics/LOD/EnableAutomaticGeneration` (OFF by default). Measured 2026-10-07 on citadel (Linux): 65 meshes,
130 automatic levels asked, **94 kept, 36 dropped** — exactly the 3 levels of each of the 12 tree-stock meshes
(Aspen, Broadleaf, Conifer, Colonized ×3), which carry their own LOD0/1/2. `generateLODLevel()` (one pool task per
mesh, levels in order) keeps a level only when `m_geometry.size() == LODLevel`: with explicit levels already in
`m_geometry` that is never true. So each of those meshes is decimated three times (~120 k triangles each), uploaded,
and released — the release right after the upload was the source of the 2026-10-04 `vkDestroyBuffer` VUIDs, fixed
by the queue timelines (engine `docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md`).

The same check exists in `MeshResource.cpp` and `MultiLayerMeshResource.cpp`.

## What remains — owner decision

- [ ] **Decision**: a mesh that already carries levels is excluded from the automatic generation (today it is in
      effect: every generated level is dropped, the work is wasted). Recommendation: skip the job when
      `m_geometry.size() > 1` at `onDependenciesLoaded()`, with a log line — no visible change, the decimation and
      upload cost disappear. Alternative: a declared opt-out in the mesh definition.
- [ ] Then measure the citadel load time and the pool's busy time before / after.
