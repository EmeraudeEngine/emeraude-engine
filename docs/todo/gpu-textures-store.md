---
id: gpu-textures-store
title: A GPUTextures store of GPU-ready material textures, looked up before Images
status: in-progress
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

- L1 DONE 2026-10-21 (graphics § 40 § Implementation): the base `TextureName` parser and its guardrails, the
  `GPUTextures` store, `Images` refusing GPU containers (T8).
- L2 DONE 2026-10-21: the resolution for JSON materials (`TextureSet` / material name, `GPUTextures` then `Images`).
  Follow-ups: D10's single fetch for a GPU data texture; an index by set name if the store grows large.
- L3 DONE 2026-10-21: `EmeraudeTextureBake --material / --materials` bakes a material definition into `GPUTextures`
  (the engine's grouping, `TexturePacker::plan()`). Follow-up: parallel baking for large stores (sequential now).
- L4 DONE 2026-10-21: the bake's fingerprint (`Emeraude.Bake`) and the stale-entry fallback (T11);
  `Core/Graphics/Texture/CheckGPUTextures`.
- L5 DONE 2026-10-21: code-built materials declared by their source (`Material::TextureSource`), resolved by the
  JSON code (T10 hidden repack, T4 lookup).
- L6 DONE 2026-10-22 (T9 revised: a glTF never looks in `GPUTextures`): `tools/gltf-bake-textures.py` bakes a `.gltf` in
  place (sidecar KTX2 files); every baked container records its binary alpha coverage (`Emeraude.AlphaCoverage`), read
  by `CompressedImageResource`; `fuzz_ktx2`. Follow-up item: `gltf-baked-textures-fingerprint`.
- The container-agnostic decoder (T7) when a second container arrives; the tokens beyond ambientCG's set (open
  question); the sources' recognition in `Images` (T13).
- A store material as the runtime proof on the three OS (L1–L6 sent to the peers once pushed).

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` (the design, decisions T1–T13).
- `docs/subsystems/graphics/39-material-texture-packing.md` § Baked assets, § The mip filter.
