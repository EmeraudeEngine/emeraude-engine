---
id: rt-reflections-curved-reflector
title: RTR glossy cone — account for the reflector's curvature
status: open
priority: unranked
scope: Graphics/Effects/Framebuffer/RTR
opened: 2026-08-30
tags: [ray-tracing, reflections, measurement]
---

# RTR glossy cone — account for the reflector's curvature

## Why

The v3 cone width (`tan θ · hitT · f_px / (dCam + hitT)`, see `src/Graphics/AGENTS.md` § "RTR glossy
cone") assumes a FLAT reflector. A convex one compresses its reflected environment into the
silhouette: the same GGX lobe covers far fewer screen texels there, and the flat-plane width
over-blurs it (the v1 comment already noted "a sphere over-blurred by an order of magnitude").

## What remains

1. A white metal SPHERE bench in projet-alpha's `post-processor-effect-debug` (roughness sweep,
   same emissive band) to measure the over-blur.
2. The term: the screen-space derivative of the normal (or the curvature from the normal buffer)
   scales the effective hit distance — derive it, measure it against the sphere.

## ⚠️ Traps

- ⚠️ Measure before deriving: the bench numbers decide, not the intuition.

## References

- `src/Graphics/Effects/Framebuffer/RTR.cpp` — the trace's cone width block.
- projet-alpha `src/Builtin/PostProcessorEffectDebug.AGENTS.md` — the bench and its protocol.
