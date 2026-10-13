---
id: texture-streaming
title: Texture streaming
status: open
priority: medium
scope: Graphics/Texture
opened: 2026-08-09
tags: [streaming, memory]
---

# Texture streaming

## Why

The engine has **no texture streaming**. `JungleRuins` (the gold-goal scene) names it: 8192×8192
JPEG normal maps (129 MB each on disk), 3.5 GB of textures, on an 8 GB card. Each such map is ~85 MB
in VRAM with its mips: a 2D texture is encoded to BC7 by default wherever the device has
`textureCompressionBC` (`src/Graphics/TextureResource/Texture2D.cpp:261`, the BC7 chain cached on disk
under `Core/Graphics/Texture/EnableTextureCache`, default true; `docs/subsystems/graphics/30-compressed-image-path.md`,
`docs/scene-loaders-usd.md` § 3.4 Textures) — 256 MB (341 MB with mips) only as the RGBA8
fallback. The scene still does not fit without residency management. This is what Unreal covers with
Virtual Texturing.

⚠️ Do not conclude from a bad frame rate on that scene that the loader is at fault — this hole and
`geometry-lod-storage-architecture.md` are the two known structural gaps it exposes.

## What remains

- [ ] Design and build residency management for texture data (mip streaming at minimum).
