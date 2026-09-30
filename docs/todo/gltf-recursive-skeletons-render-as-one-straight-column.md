---
id: gltf-recursive-skeletons-render-as-one-straight-column
title: glTF RecursiveSkeletons (a mesh reused by several skins, skins sharing a skeleton) renders as one straight column
status: open
priority: unranked
scope: Scenes/Loaders (GLTFLoader skins) / skinned rendering
opened: 2026-09-30
tags: [gltf, conformance, skinning]
---

# glTF RecursiveSkeletons renders as one straight column

## Why

Seen on Linux, macOS and Windows (2026-09-30, while validating the triad's sub-section 6a): the Khronos sample
`RecursiveSkeletons` ("reusing the same mesh multiple times by multiple skins; binding multiple skins to a single
skeleton") opened in the model viewer shows ONE straight white column. The reference
(`glTF-Sample-Assets/Models/RecursiveSkeletons/screenshot/screenshot.jpg`) shows SEVERAL curved, branching white
shapes spread apart.

Either the skins are not applied (the bind pose of the reused mesh), or every instance lands on the same place, or
only one instance of the reused mesh is created.

⚠️ Not proven pre-existing: no earlier capture of this sample exists. The triad's section 3 changes to
`GLTFLoader.cpp` (validate() / strict-forest refusals, an order-independent iterative node walk, the multi-material
descriptor copy, neutral fix-its) do not touch the skin path — building the engine at `bf5f901c^` would settle it.

## What remains

- Build the reference: how many mesh instances, which skin each binds, where each lands (the glTF: meshes reused by
  nodes with DIFFERENT `skin` indices; several skins listing joints of one skeleton).
- Check `GLTFLoader` keys a skinned mesh resource on (mesh, skin), not on the mesh alone — a mesh shared by two skins
  keyed on the mesh index would hand the FIRST skin's binding to every user (the same lesson as `buildResourceKey()`).
- Add the sample to the conformance bench's checked set once fixed.

## References

- Khronos glTF-Sample-Assets `RecursiveSkeletons` README; engine `docs/subsystems/scenes-loaders/03-implemented-loaders/01-gltfloader.md`.
