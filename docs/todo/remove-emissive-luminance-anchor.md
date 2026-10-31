---
id: remove-emissive-luminance-anchor
title: Remove the glTF loader's EmissiveLuminanceAnchor (2000 nits per unit) to honour the emissive-unit decision
status: open
priority: unranked
scope: Scenes/Loaders/GLTFLoader
opened: 2026-10-31
tags: [gltf, photometry, emissive]
---

# Remove the glTF loader's EmissiveLuminanceAnchor (2000 nits per unit) to honour the emissive-unit decision

## Why

Owner decision (2026-07-26): a glTF material's emission is read in the specification's unit (`emissiveFactor` ×
`KHR_materials_emissive_strength`, in nits), with no import multiplier, and the engine never compensates the
Blender exporter defect (glTF-Blender-IO#1766). The loader still multiplies every glTF emission by
`EmissiveLuminanceAnchor = 2000` (`src/Scenes/Loaders/GLTFLoader.cpp:1206`), an August 2026 experiment whose own
comment says it awaited an owner arbitration. The arbitration is the 2026-07-26 decision (re-confirmed 2026-10-31).

## What remains

- Remove the anchor; the emission becomes `emissiveFactor × emissiveStrength` nits.
- Re-measure the assets the anchor was masking or causing: DamagedHelmet's HUD, CarConcept's logo map
  (`nonEmissiveMaterialNames`, `docs/subsystems/scenes-loaders/02-architecture.md:73`), the glTF conformance bench.
- Update the docs that describe the anchor (`docs/caution-points.md` § "a glow switched off read black",
  scenes-loaders `02-architecture.md`, graphics 13-12 doc 23).

## ⚠️ Traps

- An asset WITHOUT `KHR_materials_emissive_strength` then emits about 1 nit: invisible against a daylight-metered
  scene. That is the decided behaviour, not a regression to "fix" with a new multiplier; present the measurements to
  the owner before closing.
- The WAD materializer has its own anchor (8000 since 2026-10-15, compensating sRGB decoding): it is a separate
  loader and is not covered by this decision unless the owner says so.

## References

- `src/Scenes/Loaders/GLTFLoader.cpp:1195-1210`
- graphics 13-12 doc 23 § glTF emissive — owner decision
