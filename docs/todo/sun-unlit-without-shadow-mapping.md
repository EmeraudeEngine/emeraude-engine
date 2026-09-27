---
id: sun-unlit-without-shadow-mapping
title: Disabling shadow mapping appears to switch the directional sun's light off entirely
status: open
priority: unranked
scope: Saphir (LightGenerator light-pass selection), Scenes (LightSet), Graphics/Renderer
opened: 2026-09-28
tags: [lighting, shadows, directional-light, settings, observed]
---

# Disabling shadow mapping appears to switch the directional sun's light off entirely

## Why

Observed while bisecting the `terrain` ground blobs (2026-09-28, docs/caution-points.md § Two-sided normals): with
`Core/Graphics/ShadowMapping/Enabled = false` in a settings copy, `terrain --demo-options 100000,25,0,0,1` renders a
FLAT ground — no modelling of the relief at all, the sky blown out — and the near-ground crop mean falls from 98 to
41/255 (auto exposure). A light without a shadow map should still light: the frame looks as if the sun contributed
nothing. Not investigated: this item only records the observation.

## What remains

1. Confirm it at a PINNED exposure (the auto exposure moves between the two runs): same pose, shadows on/off, compare
   a sunlit slope against a slope facing away.
2. If confirmed, find which light-pass variant a shadow-less directional light takes
   (`RenderPassType::DirectionalLightPass*`, the program set built for it) and why it adds nothing. Check point and
   spot lights too, and a demo without `SunCourse` / `SkyFollowsSun`, to tell an engine defect from a demo one.

## ⚠️ Traps

- A shadow A/B is meaningless until this is settled: switching the shadows off removed the blobs only because every
  pixel lost the sun.

## References

- Captures of 2026-09-28: `1790546767` (shadows on, effects bypassed) against `1790546911` (shadows off, same).
