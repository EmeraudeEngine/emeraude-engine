---
id: texture-vram-accounting-per-texture
title: Account the VRAM of every texture (and per material) — the reference before texture packing
status: in-progress
priority: unranked
scope: Graphics/TextureResource, Vulkan (VMA)
opened: 2026-10-20
tags: [memory, textures, measurement, texture-packing]
---

# Account the VRAM of every texture (and per material)

## Why

The texture-packing work (graphics § 39) must prove its gain, and today nothing measures a texture's footprint:
`Texture2D::memoryOccupied()` answers `sizeof(*this)`; only the global VMA budgets exist (graphics § 37). Owner
order (2026-10-20): measure first.

## What remains

- DONE on Linux (2026-10-20, uncommitted): `Vulkan::Image::deviceMemoryBytes()`, `StandardResource::textureUsages()`,
  `Core.RendererService.getTextureMemory(top)`; reference recorded in graphics § 39 § Reference measurement
  (sponza, water-world); console conformance 4 935 / 0, MCP conformance 1 902 / 0, 0 `VUID-`.
- Validation by the peers (macOS, Windows) after the push: the command on `sponza` and `water-world`, the numbers
  compared with Linux (the BC7 sizes must match; MoltenVK may align differently).
- `jungle-ruins` is not a reference until its USD textures load (`usd-texture-paths-relative-to-authoring-layer`):
  measure it then and add the row.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `37-gpu-memory-budget-and-report.md`.
