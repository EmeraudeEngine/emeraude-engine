---
id: underwater-caustics-and-depth-light
title: The sun under the sea level — caustics on what it lights, and its light dimming and turning blue with depth
status: open
priority: unranked
scope: Scenes (lights, sea level), Saphir (light generators), Graphics (deferred resolve, traced lanes)
opened: 2026-11-23
blocked-by: [underwater-view]
tags: [water, ocean, lighting, caustics]
---

# The sun under the sea level — caustics on what it lights, and its light dimming and turning blue with depth

## Why

Owner (2026-11-23, with `underwater-view`): the caustics on the seabed are wanted, after the base. And his decision of
the same day: the LIGHT's own path through the water — the sun dimming and turning blue with depth on every surface it
lights — is handled here, in the lighting, for both views (from above through the surface, and from under it), not as
a screen-space factor in the underwater effect.

Until this is done, a deep seabed seen at close range under water keeps its sunlit sand colour.

## What remains

Everything. To decide with the owner before any code:
- where the factor enters: every direct-light path shades a submerged point (the forward light generators, the
  deferred resolve, the traced lanes that shade their hits) — one shared helper, read from the sea's `WaterMedium`;
- the caustics' pattern: derived from the sea's own waves (the FFT slopes) or an animated texture; its fade with depth;
- the existing light projections (the gobo of a light) as a carrier, and their open items
  (`traced-lanes-miss-directional-and-animated-cubemap-projections`, the `deferred-resolve-projection-*` items).

## ⚠️ Traps

- The lagoon seen from above was approved by the owner (2026-09-28) with an absorption along the VIEW path only:
  adding the light's path darkens and blues the deep sand. Judge it on screen with him, with a before / after pair.
- The sea no longer casts shadows on its own seabed (`Scene.rendering.cpp`): a caustic is not a shadow map of the sea.
