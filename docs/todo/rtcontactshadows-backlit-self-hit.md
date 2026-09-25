---
id: rtcontactshadows-backlit-self-hit
title: RTContactShadows crushes a back-lit leaf — its ray hits the pixel's own card
status: open
priority: unranked
scope: Graphics/Effects/Lighting/RTContactShadows
opened: 2026-09-25
tags: [ray-tracing, contact-shadows, vegetation]
---

# RTContactShadows crushes a back-lit leaf — its ray hits the pixel's own card

## Why

By reading (2026-09-25, leaf-translucency design): for a pixel whose light is BEHIND its surface (a back-lit leaf),
the contact-shadow ray starts on the viewer's side (origin offset along the view-facing normal) and crosses the
pixel's own card at `t = adaptiveBias / |N·L|` (`RTContactShadows.cpp` ~172, ~195-205, ~546-553). The shadow is
`smoothstep(0, MaxDistance, t)` ≈ 0: the transmitted glow is darkened to ~20 % near the camera in the traced lane
only. SSContactShadows has a facing gate for exactly this (`SSContactShadows.cpp` ~148-152).

## What remains

- A facing gate like SSContactShadows' (no contact shadow when the light is behind the surface), or rejecting the
  pixel's OWN instance in the candidate judgement.
- ⚠️ A light-side ray origin does NOT fix it under wind: the BLAS holds rest-pose triangles (owner decision
  2026-09-21) while the raster leaf is displaced, so the ray still hits the rest-pose copy of its own card at
  `t ≈ displacement`. Freeze the wind at ZERO displacement for any traced measurement.
