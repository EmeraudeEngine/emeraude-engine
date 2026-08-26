---
id: csm-rework
title: CSM directional shadows — full rework (2 certain root causes, approved)
status: open
priority: high
scope: Graphics/ShadowMap
opened: 2026-08-25
tags: [shadow-map, measured]
---

# CSM directional shadows — full rework

**Owner decision (2026-08-25): a COMPLETE rework, approved as the chantier following the classic
shadow-mapping fixes.** This supersedes the "PARKED (Jul 2026)" note of the old `TODO.md`: the
investigation that was parked has since produced two certain root causes.

## Why — the two root causes, both certain

- **4a — the CSM UBO is never uploaded after creation.** `DirectionalLight::updateCascades()`
  fills `m_CSMBuffer` every frame and **never calls `requestVideoMemoryUpdate()`**, so the only
  upload is the one from `createOnHardware()`, when the buffer is ALL ZERO (black colour, zero
  intensity). Hence "CSM lights do not light their receivers".
  ⚠️ The engine doc used to attribute this to an early return on `m_shadowMap == nullptr` —
  **FALSE** (when the resolution is > 0 the map exists). Attribution corrected in
  `docs/shadow-mapping.md`.
  **Falsifiable prediction that distinguishes this cause:** a CSM light in CONTINUOUS motion would
  light correctly (`move()` raises the flag every frame).
- **4b — sign and order of the ortho near/far.** `computeCascadeProjection()` passes
  `minZ - margin` / `maxZ + margin` as the ortho near/far, but those are **view-space z
  coordinates** (negative in front of the camera) while `orthographicProjection()` expects
  **positive distances**. Correct: `near = -maxZ - margin`, `far = -minZ + margin` (sign AND order
  inverted). `frustumCenter` is computed and never used.

## What remains

- [ ] Fix 4a and 4b.
- [ ] Texel snapping (otherwise shimmering under camera motion).
- [ ] Inter-cascade fade.
- [ ] Per-cascade bias.
- [ ] `depthClamp` on the projection pass (it is already enabled for the classic pass in
  `ShadowCasting.cpp` — the feature is requested in `Instance.cpp`).
- [ ] **A single source of truth for the light-space matrix** — three sites must agree today.

## ⚠️ Traps

- **Dormant**: `getUniformBlockCSM()` declares the GLSL array with `cascadeCount` while the C++
  offsets assume 4 matrices hard-coded. Every call uses the default `= 4`, so it is harmless
  today; passing 2 or 3 silently shifts the rest of the block.
- `coverageSize` is a **HALF-extent** (60 ⇒ 120×120 m) and the box is centred on the **world
  origin**, never on the camera. Outside it there is no shadow information — that is structural,
  not a bug.
- Do the shadow maps BEFORE the world-space volumetric pass
  (`volumetric-light-single-scattering.md`): it will be their first external consumer, and the
  "outside the map = lit" convention must live in ONE place.
