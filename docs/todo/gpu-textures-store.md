---
id: gpu-textures-store
title: A GPUTextures store of GPU-ready material textures, looked up before Images
status: open
priority: unranked
scope: Resources/Manager (stores, containers), Graphics/TextureResource, Graphics/Material/StandardResource, tools/texture-bake
opened: 2026-10-21
tags: [textures, resources, release, texture-packing]
---

# A GPUTextures store of GPU-ready material textures, looked up before Images

## Why

`Images` mixes authoring sources and runtime artefacts, and a baked KTX2 is reachable from a glTF only: a JSON or
C++ material always loads its textures through the pixel path; a code-built material is not repacked. A finished product must be able to ship GPU-ready
textures with no texture compute at load, while a development build keeps working from the sources and the cache.

## What remains

- The owner's open question of the design (graphics § 40 § Open questions): the tokens beyond ambientCG's set.
- Then the lots: the store and its container-agnostic decoder (T7); the name parser and its guardrails (unit-tested,
  every refusal); the resolution per component (`TextureSet` / material name, `GPUTextures` then `Images`; `.gltf`
  materials too, T9); the fingerprint and the stale-entry fallback (T11); the sources' naming check (T13); `Images` refusing GPU containers (T8); the hidden repack of code-built materials (T10); the tool
  that bakes a material definition into `GPUTextures`; a store material as the runtime proof on the three OS.

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` (the design, decisions T1–T13).
- `docs/subsystems/graphics/39-material-texture-packing.md` § Baked assets, § The mip filter.
