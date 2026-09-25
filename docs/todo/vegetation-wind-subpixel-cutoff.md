---
id: vegetation-wind-subpixel-cutoff
title: Stop computing the vegetation wind where its motion is smaller than a pixel
status: open
priority: unranked
scope: Saphir/AbstractVertexStage (vegetation wind), Scenes/SceneInstanceTransforms
opened: 2026-09-25
tags: [vegetation, wind, performance]
---

# Stop computing the vegetation wind where its motion is smaller than a pixel

## Why

Owner, 2026-09-25 (`terrain`, 750 000 trees): "eliminate the wind on the trees too small on screen, it is useless
— I said 40 px at random, find a metric so that only the trees whose leaves visibly move are animated".

The metric: the wind's displacement PROJECTED on screen. A motion under ~1 px is invisible (and the TAA blends it
away). `pixels = amplitude × viewportHeight / (2 · tan(fovY / 2) · depth)`: at 2880 × 1620 under 60°, the 0.4 m
tip bend of `terrain` (`setVegetationWind(…, 0.4, 0.6)`) falls under a pixel at ~560 m, the much smaller leaf
flutter at about a hundred metres.

⚠️ Today it would change little: the wind is scene-wide (ONE header in the instance-transforms SSBO) and
`terrain`'s trees become octahedral IMPOSTERS at 250 m, which never sway (`AbstractVertexStage`: "an imposter
billboard cannot be … blown by the wind"). What sways at 100-250 m is the mesh LODs, whose trunk and branch
motion is still 2-5 px there. The saving is the FLUTTER term of the far mesh LODs.

## What remains

- Per vertex, per term (trunk, branch, flutter): weight = smoothstep over the projected amplitude (0.5 → 1 px),
  and skip the term's evaluation below it (the view projection and the viewport height are in the view UBO).
- Measure the vertex cost before/after on `terrain` (GPU profiler, ScenePass + shadow cascades).
- ⚠️ What the owner sees "moving" far away may be the octahedral imposters switching view cells while the camera
  moves, or foliage aliasing — not the wind: check that first (freeze the wind: `setVegetationWind(…, 0, …)`).
