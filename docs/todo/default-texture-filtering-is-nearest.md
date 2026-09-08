---
id: default-texture-filtering-is-nearest
title: The default texture filtering is "nearest" with 1 mip level and no anisotropy
status: open
priority: unranked
scope: SettingKeys.hpp (Core/Graphics/Texture/*), Graphics/TextureResource
opened: 2026-09-08
tags: [quality, defaults, settings]
---

# The default texture filtering is "nearest"

## Why

Measured 2026-09-08, `src/SettingKeys.hpp`:

| Key | Default |
|---|---|
| `Core/Graphics/Texture/MagFilter` / `MinFilter` / `MipFilter` | **`nearest`** (`DefaultGraphicsTextureFiltering`) |
| `Core/Graphics/Texture/MipMappingLevels` | **1** |
| `Core/Graphics/Texture/AnisotropyLevels` | **0** |
| `Core/Graphics/Texture/POMIterations` | 0 |

So an installation that has never been hand-tuned renders **every texture in the project with
nearest-neighbour magnification, no mip chain and no anisotropic filtering**. On a floor plane at a
grazing angle that is the worst case of all three at once.

That collides with two things:

1. **The stated imperative** — projet-alpha's `AGENTS.md` puts production-grade real-time visual
   quality as non-negotiable. A default that makes every asset look blocky is the first thing a new
   pair of eyes sees.
2. **The settings reset.** `Core` backs the file up and clears the store when the stored
   `WrittenByEngineVersion` or `WrittenByApplicationVersion` is older than the current one
   (`src/AGENTS.md` § reset). Every hand-tuned quality key then falls back to these defaults.
   Observed on the owner's machine: a reset took `linear/linear/linear`, anisotropy 8 and 1024 mip
   levels back to `nearest/nearest/nearest`, 0 and 1 — plus PCF off, TAA off and motion blur off.
   The owner noticed it as "texture filtering went to nearest, is that normal?".

## Owner decision needed — do not guess

`nearest` may be deliberate (it is the honest default for pixel-art content, and
`light-and-shadow-debug`'s Doom-style sprite genuinely wants it). Two candidate answers, and they
are not exclusive:

- [ ] **Move the defaults** to `linear` / a full mip chain / anisotropy at the device maximum, and
      let pixel-art content ask for `nearest` per resource rather than the reverse.
- [ ] **Make the quality keys survive the reset**, by adding them to projet-alpha's
      `RestoredSettingsKeys` (`src/Application.cpp`, currently 2 entries: the Vulkan validation
      pair). That fixes the recurrence without touching anyone's defaults.

⚠️ A third option that looks tempting and is NOT one: raising
`Core/Graphics/ShadowMapping/NormalOffsetScale` as part of a "quality" preset. It ships at 0
**deliberately and on measurement** — at scale 1.0 it removes the sphere's and the dragon's contact
shadows on `reflexion-debug` (`docs/shadow-mapping.md` § Normal-offset). Quality presets must not
touch it.

## References

- [`../shadow-mapping.md`](../shadow-mapping.md) § Normal-offset — why one quality knob stays off.
- `src/AGENTS.md` § what survives a reset (`SettingsKeyRestoration`).
