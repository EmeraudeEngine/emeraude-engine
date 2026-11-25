---
id: underwater-caustics-and-depth-light
title: The sun under the sea level — caustics on what it lights, and its light dimming and turning blue with depth
status: in-progress
priority: unranked
scope: Scenes (lights, sea level), Saphir (light generators), Graphics (deferred resolve, traced lanes)
opened: 2026-11-23
tags: [water, ocean, lighting, caustics]
---

# The sun under the sea level — caustics on what it lights, and its light dimming and turning blue with depth

## Why

Owner (2026-11-23, with `underwater-view`): the caustics on the seabed are wanted, after the base. And his decision of
the same day: the LIGHT's own path through the water — the sun dimming and turning blue with depth on every surface it
lights — is handled here, in the lighting, for both views (from above through the surface, and from under it), not as
a screen-space factor in the underwater effect.

Until this is done, a deep seabed seen at close range under water keeps its sunlit sand colour.

## Done (2026-11-25, Linux — graphics doc 45)

The absorption along the light's own path and the caustics of the waves, on every directional light, in the PBR light
pass; the traced lanes get the absorption; three console commands tune it on a running scene. The owner saw it on
screen and kept the first values. Everything that must outlive this item is in
`docs/subsystems/graphics/45-the-sun-under-a-sea.md`.

## What remains

- The peers: macOS, Windows (the compute blits of the curvature's mips and the seven bindless textures are new code
  for MoltenVK and for MSVC). AMD.
- The traced lanes' factor: its effect on the picture measured (a deep seabed in a reflection, the Reflections slot on
  and off).
- The cost of the term a frame, on the RTX 3070 Ti.
- Non-PBR materials under the sea (the term is in the PBR light pass only, like the clouds' shadow): to decide with
  the owner whether any demo needs it.
- The edge of the caustics against the drawn water's edge in a strong swell (one round of x - D(x), the cascades' sum
  instead of the mesh's interpolation): to look at with the owner if he sees it.
