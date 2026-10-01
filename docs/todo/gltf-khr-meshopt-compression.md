---
id: gltf-khr-meshopt-compression
title: glTF files that require KHR_meshopt_compression are refused (fastgltf 0.9.0)
status: open
priority: unranked
scope: Scenes/Loaders/GLTFLoader, the vendored fastgltf
opened: 2026-10-01
tags: [gltf, compression, meshopt, dependency]
---

# glTF files that require KHR_meshopt_compression are refused (fastgltf 0.9.0)

## Why

Seen 2026-10-01: `glTF-Sample-Assets/Models/BrainStem/glTF-Meshopt/BrainStem.gltf` declares
`extensionsRequired: ["KHR_mesh_quantization", "KHR_meshopt_compression"]`. The loader refuses it ("An extension
required by the glTF is not supported by fastgltf"), and the model viewer stays empty.

The `glTF-Meshopt-EXT` variant (`EXT_meshopt_compression`) loads and animates. `KHR_meshopt_compression` is the
ratified Khronos successor of the EXT extension; the sample corpus now ships both.

## What remains

- [ ] Check whether a newer fastgltf release parses `KHR_meshopt_compression`, and whether its decode path matches
  the EXT one: the engine decodes meshopt itself (`MeshoptBufferAdapter`, `meshoptimizer`).
- [ ] Either upgrade fastgltf, or map the KHR extension onto the existing EXT decode path. The extension's spec says
  whether its filters / modes differ from EXT's.
- [ ] Re-test: every `glTF-Meshopt` sample opens, renders like its EXT twin, and animates (BrainStem). Rerun the
  conformance bench.

## References

- Khronos `KHR_meshopt_compression` and `EXT_meshopt_compression` specifications.
- `src/Scenes/Loaders/GLTFLoader.cpp` (`MeshoptBufferAdapter`, the parser extension mask, `reportMissingExtensions()`).
