---
id: fbx-pbr-specular-branch-unexercised
title: FBX — the PBR specular branch (pbr.specular_factor / specular_color) has never run
status: open
priority: unranked
scope: Scenes/Loaders/FBXLoader
opened: 2026-09-15
tags: [fbx, material, test-coverage]
---

# FBX — the PBR specular branch has never run

## Why

`FBXLoader` reads `pbr.specular_factor` / `specular_color` behind a gate on `ufbx_material::shader_type`. The only FBX
assets here are the Paladin's, which are legacy Phong, so the gate is proven CLOSED where it must be and untested
where it opens. This is a test-coverage gap, not a known defect: the priority waits for the owner.

## What remains

- Find or author an FBX with a PBR material (e.g. a 3ds Max Physical / Autodesk Standard Surface export), load it, and
  check the specular reaches the material with the right values.

## References

- `docs/subsystems/scenes-loaders/03-implemented-loaders/03-fbxloader.md`; split out of the former item
  `gltf-conformance-loader-gaps` (2026-10-11).
