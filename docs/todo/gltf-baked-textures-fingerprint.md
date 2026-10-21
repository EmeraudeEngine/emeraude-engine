---
id: gltf-baked-textures-fingerprint
title: A baked glTF's textures checked against their sources at load
status: open
priority: unranked
scope: tools/gltf-bake-textures.py, Scenes/Loaders/GLTFLoader, tools/texture-bake
opened: 2026-10-22
tags: [textures, gltf, bake]
---

# A baked glTF's textures checked against their sources at load

## Why

A `GPUTextures` entry carries the bake's fingerprint (`Emeraude.Bake`) and goes stale visibly: the engine compares it
with the sources it has and falls back on them (graphics § 40, L4). A glTF baked by `tools/gltf-bake-textures.py` (a
GLB, or a `.gltf` and its sidecar KTX2 files, L6) carries none: a repainted source leaves the baked asset behind, and
nothing says so at load. Owner decision 2026-10-22: not part of L6 — the tool's cache is keyed on the CONTENT of the
sources, so the next bake catches the change; the load-time check is this item.

## What remains

- The single bake mode of `EmeraudeTextureBake` writes a fingerprint (its sources, channels and flip, the pipeline
  version), the glTF tool passes it.
- The glTF loader reads it (`KTX2Decoder::readKeyValue(bytes, key)` exists, fuzzed by `fuzz_ktx2`), resolves the sources
  relative to the asset, and reports a stale image (falling back means decoding the sources: an owner decision).

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` § Implementation (L4, L6).
- `docs/subsystems/graphics/39-material-texture-packing.md` § Baked assets.
