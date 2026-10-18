---
id: usd-texture-paths-relative-to-authoring-layer
title: USD texture paths are resolved against the stage, not against the layer that authored them
status: open
priority: high
scope: src/Scenes/Loaders/USDLoader.cpp (resolveTexture), src/Scenes/Loaders/USDParser.cpp (describeMaterials)
opened: 2026-10-18
tags: [defect, usd, textures, jungle-ruins]
---

# USD texture paths are resolved against the stage, not against the layer that authored them

## Why

JungleRuins option 1 (`JungleRuins_Karma.usda`) renders its terrain and pyramid untextured: 1 103
`[Warning][USDLoader]`, almost all "Texture '../../../textures/…' not found next to the stage", 196 distinct paths
(129 under `textures/Terrain`, 57 Pyramid, 9 Trees, 1 River). The files exist (`JungleRuins/textures/Terrain/…`).
An identifier like `../../../textures/Terrain/Metallic-Roughness.png` is relative to its AUTHORING layer, deep under
`USD/elements/<element>/`; the loader joins it to the STAGE directory (`JungleRuins/USD/`), which escapes the asset
(`data-stores/textures/…`) and finds nothing. The case-insensitive retry cannot help (0 "matched by IGNORING CASE").

Not a regression of the 2026-10-18 analysis / materialization split: the same 1 103 warnings and 196 paths before it
(engine 0f0e9931 era) and after (b810222b), on Linux and on macOS (M6, found by macOS-PA). The "untextured-looking
ground" noted on 2026-10-04 (JungleRuins rendering investigation) is this.

## What remains

- Resolve a texture asset path against the layer that authored the shader input: Tydra's `asset_identifier` (with
  `load_texture_assets = false`) is the raw authored path here — find what it can give (the resolved path through its
  `asset_resolver`, the layer's working directory, the `AssetPath`'s resolved form) before adding any search of our
  own. The USDZ path (`archiveTexture()`) already resolves against the archive table and is not affected.
- A heuristic search of the stage tree by filename is NOT the fix (the WorldLobby lesson: 60 textures share one
  basename — `USDZArchive::resolve()` refuses an ambiguous suffix for that reason).
- Verify: JungleRuins option 1 with 0 "not found next to the stage" for files that exist, the terrain and pyramid
  textured; WorldLobby (USDZ) and the Anthurium element unchanged.

## References

- `docs/subsystems/scenes-loaders/03-implemented-loaders/02-usdloader.md`, `docs/scene-loaders-usd.md` (texture
  resolution, `load_texture_assets = false`).
- `USDLoader::resolveTexture()`, `USD::describeMaterials()` (`textureIdentifier`).
