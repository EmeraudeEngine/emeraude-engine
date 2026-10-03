---
id: rtr-metallic-paint-blotches
title: The RT lane's reflections blotch a curved metallic paint (CarConcept in citadel)
status: open
priority: unranked
scope: Graphics/Effects/Lighting (RTR, its denoiser), the metallic / clear-coat path
opened: 2026-10-03
tags: [ray-tracing, reflections, denoise, metal, clearcoat]
---

# The RT lane's reflections blotch a curved metallic paint (CarConcept in citadel)

## Why

Seen by the Windows peer and on Linux (2026-10-03), citadel's CarConcept on the RayTracing lane. Its
paint (`Paint 1 Carmine`: metallic 1 by default, `KHR_materials_clearcoat`) shows dark grainy patches with
ragged edges on the bonnet and wings, where Khronos' reference is smooth.

Isolated by A/B, same pose:
- The patches stay with `PostProcess.disable(AmbientOcclusion)`, `disable(IndirectDiffuse)` and
  `disable(ContactShadows)`.
- With `disable(Reflections)` the car goes nearly black: this metallic paint takes almost all its light
  from reflection, so that concept carries the patches.
- The ScreenSpace lane (SSR) renders the paint smooth.

It was first attributed to the multi-UV gap (the occlusion baked on `TEXCOORD_1`). That was WRONG: a
metal has no ambient diffuse for an occlusion to darken (`docs/caution-points.md`, the metalness weight).

## What remains

- Find which part of RTR makes them. Candidates: too few rays per pixel or a denoiser that does not
  converge on a curved, low-roughness surface; the clear coat's second lobe; a self-intersection bias on
  the curved shell (patches with ragged edges are also the signature of acne).
- Measure per pixel at a pinned exposure: the paint's high-frequency energy, RT lane against SS lane,
  same pose.

## References

- Graphics: `docs/reflection-pipeline.md`; memory note "RT reflection traps".
