---
id: gpu-textures-container-agnostic-decoder
title: A container-agnostic decoder for the GPUTextures store (T7)
status: open
priority: unranked
scope: Graphics/KTX2Decoder, Graphics/CompressedImageResource
opened: 2026-10-22
tags: [textures, ktx2]
---

# A container-agnostic decoder for the GPUTextures store (T7)

## Why

Owner decision T7 (graphics § 40): the store accepts every container — KTX2 with raw BC blocks, KTX2 with Basis
Universal, and later ones (DDS…) as a new decoder, never a new store. Only KTX2 exists today (`KTX2Decoder`, called by
`CompressedImageResource`); the abstraction is worth building when a second container arrives, not before.

## What remains

- When a second container is needed: a decoder interface chosen by the file (magic number), the KTX2 one first,
  `CompressedImageResource` reading through it; the key/value data (`Emeraude.Bake`, `Emeraude.AlphaCoverage`) given an
  equivalent or refused for that container.

## References

- `docs/subsystems/graphics/40-gpu-textures-store.md` (T7).
