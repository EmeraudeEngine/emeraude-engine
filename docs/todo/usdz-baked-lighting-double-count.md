---
id: usdz-baked-lighting-double-count
title: USDZ WorldLobby — suspected double counting of baked lighting
status: open
priority: medium
scope: Scenes/Loaders/USDLoader
opened: 2026-08-10
tags: [usd, photometry, measured]
---

# USDZ WorldLobby — suspected double counting of baked lighting

## Why

`--load-demo world-lobby --demo-options 1`: **49.8 % of the frame is ≥ 0.9 sRGB, 18.8 % clips,
WHILE THE FLOOR IS CORRECT** (194 cd/m²). Right wall **≥ 570**, hall background **≥ 555** =
~3× the floor. Downward-facing spots cannot do that to a VERTICAL surface: a wall under a downlight
receives grazing light and must be DARKER than the floor.

Hypothesis (untested): an Omniverse Kit export bakes its lighting into the base colour, and
re-lighting counts it twice. The rule is **EMISSIVE, never lit**.

⚠️ **This is NOT an exposure problem** — the floor is correct, so do not touch the exposure
triplet; what would collapse is the clipping figure.

## What remains

⚠️ **2026-10-04: the measurements above were taken WITHOUT the baked textures** — the 85
`Materials/Bake/baked_textures_*` were "not in the archive" (the materials wore flat colours) and a third of the
layers (furniture, plants, lamps) were not composed (`docs/scene-loaders-usd.md` § 11.8, fixed). The
suspicion is now testable for real. Confirmed, it is a defect (a wrong image): high by default.

- [ ] **Re-measure WITH the baked textures**, at a PINNED exposure: floor / right wall / hall luminances (cd/m²),
  the share of the frame ≥ 0.9 sRGB and the clipping share; compare with the August figures above.
- [ ] **Verification without launching anything**: sample the base colour of those materials and
  look for shading gradients baked into the texels.

## References

- `docs/scene-loaders-usd.md` § 11.5 "Still open on this asset", item 1 (the August measurement), § 11.8 (the
  half-loaded scene, fixed 2026-10-04).
