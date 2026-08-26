---
id: photometry-phase-2-relight-demos
title: Photometry phase 2 — relight the demos in real units and delete legacyUnitCompensation
status: open
priority: high
scope: Scenes/Lighting
opened: 2026-07-26
tags: [photometry, calibration]
---

# Photometry phase 2 — relight the demos in real units and delete legacyUnitCompensation

## Why

Phase 1 landed (`Photometry.hpp` + Karis falloff). The shader still carries a TEMPORARY expression:

```
legacyUnitCompensation = 0.8533 · (r²/4 + 1)
```

The move to true inverse-square makes existing content ~22× darker at mid-range (r = 10, d = 5:
0.75 → 0.034); the compensation restores the level at d = r/2 and preserves the RELATIVE balance
between different radii — the one thing auto-exposure cannot recover. **Phase 2 deletes it.**

## What remains

- [ ] Relight the demos in real values (sun 100 klx, a bulb 800 lm…). Measured surface: **3
  `setIntensity` in projet-alpha, 6 in the engine, 4 data files** — the cost is the calibration,
  not the lines.
- [ ] Delete `legacyUnitCompensation`.
- [ ] `peakLuminanceNits` on the cubemaps.

## Owner decisions — do not re-litigate

- **Emissives are settled BY THE SPEC (2026-07-26); invent no convention.** glTF 2.0
  `Specification.adoc` line 2118: the product of emissive texture × factor is in **cd/m² (nits)**;
  `KHR_materials_emissive_strength` is a unitless multiplier that "does not alter the physical
  units". So `emissiveFactor × emissiveTexture × emissiveStrength` **IS** a luminance in nits —
  follow it to the letter, anchor no constant.
- Assets authored "artistically" (emissive in [0,1], no extension) are worth ~1 nit ⇒ black.
  **Owner, 2026-07-26: we follow the spec, NO patch.** If an emissive goes black in phase 2, the
  fix is in the asset, not the importer. (Measured before deciding: 1 asset file uses emissive, 0
  use the extension, 4 `setEmissiveStrength` call sites outside the loaders.)
- ⚠️ **Do not compensate an exporter bug**: Khronos glTF-Blender-IO#1766 — Blender's watt-based
  emission needs ×683/(2π) to come out in conformant nits. Compensating engine-side would
  double-correct properly exported assets.

## ⚠️ Traps

- The falloff SHAPE changed (brighter near, darker far) and `liminal` does not exercise it —
  verify on `light-and-shadow-debug` and `basic-scenery`.
- `PixelFactory` has **no HDR format at all**, which bounds what can be authored.
