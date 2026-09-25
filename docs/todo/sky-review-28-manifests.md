---
id: sky-review-28-manifests
title: Unfreeze the review of the 28 sky manifests
status: open
priority: medium
scope: Scenes/Sky
opened: 2026-07-29
tags: [ibl, review, owner-gate]
---

# Unfreeze the review of the 28 sky manifests

## Why

The IBL chantier is complete (lots 1-4 pushed), so the sky review can finally be judged with
directional irradiance and live reflections instead of a flat ambient. The protocol and the
verdicts already acquired are in the sky photometric contract work.

## Done so far

- **2026-09-25: every body is where its picture paints it.** The 13 Y-down directions were MEASURED
  (`tools/sky-manifest.py --locate`), `IndustrialSunsetPureSky` got its missing sun (920 lx, anchored on
  its sky) and `JeGray` its measured photometry (owner decisions): `docs/caution-points.md` § *The sky
  manifests put their bodies where the pictures do not*. The review now judges photometry and taste only
  — a sky can no longer look wrong because its sun lights the scene from below.

## What remains

- [ ] Run the review of the 28 sky manifests with the owner.
- [ ] Points to show explicitly:
  - the `geometry-loader` boat is a **WHITE** material (its "red" came from the 3000 K sun alone);
    under a 17 klx dome it reads very light — the dome/star balance of the manifests is what the
    owner has to judge;
  - the prefiltered cubemap's mip 4-5 checkerboard, if any shimmer shows.
