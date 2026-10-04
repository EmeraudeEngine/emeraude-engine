---
id: automatic-lod-buffers-destroyed-in-use
title: Automatic LOD — vkDestroyBuffer on buffers still in use, and decimation of meshes that carry their own LODs
status: open
priority: unranked
scope: Graphics/Renderable/MeshResource, MultiLayerMeshResource (generateLODLevel), Geometry upload
opened: 2026-10-04
tags: [lod, vulkan, validation]
---

# Automatic LOD — vkDestroyBuffer on buffers still in use, and decimation of meshes that carry their own LODs

## Why

`Core/Graphics/LOD/EnableAutomaticGeneration` is OFF by default. Turned ON in a test settings copy (2026-10-04,
Linux, citadel, validation layers on, while testing the CPU-copy release), 65 LOD jobs ran and the run carried
**40 VUIDs**, identical with the CPU-copy release on or off:
- 20 × `VUID-vkDestroyBuffer-buffer-00922` ("can't be called on VkBuffer … currently in use by VkCommandBuffer"),
  during the loading, right after an automatic level finishes (`'Aspen0_LOD1' (IndexedVertexResource) is
  successfully loaded`);
- 20 × `VUID-vkDestroyDevice-device-05137` at shutdown (objects still alive when the device is destroyed).

The automatic LOD also decimated meshes that already have their own levels: projet-alpha's tree stock (Aspen,
Broadleaf, Conifer, Colonized — built with LOD0/1/2 per species) and the CarConcept parts.

## What remains

1. Find which buffer is destroyed in use (a staging buffer, or a level's buffers replaced while a frame still
   draws them) and why 20 objects outlive the device.
2. Owner decision: should a mesh carrying explicit LODs (the tree stock) be excluded from the automatic
   decimation, and how is that declared?

## References

- `geometry-lod-storage-architecture` (where the generated levels live, a separate question).
- `MeshResource::onDependenciesLoaded()` / `generateLODLevel()` (the `_LOD<n>` geometries), the same in
  `MultiLayerMeshResource`; `docs/todo/cpu-copies-retained-after-upload.md` (the lease taken by the LOD job).
