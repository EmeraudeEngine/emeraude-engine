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
JPEG normal maps (129 MB each on disk, 256 MB in VRAM as RGBA8), 3.5 GB of textures, on an 8 GB
card. This is what Unreal covers with Virtual Texturing.

⚠️ Do not conclude from a bad frame rate on that scene that the loader is at fault — this hole and
`geometry-lod-storage-architecture.md` are the two known structural gaps it exposes.

## What remains

- [ ] Design and build residency management for texture data (mip streaming at minimum).
