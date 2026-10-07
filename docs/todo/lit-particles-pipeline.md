---
id: lit-particles-pipeline
title: Graphics — a LIT particle emitter has no valid pipeline
status: open
priority: high
scope: Graphics
opened: 2026-09-28
tags: [graphics, particles, measured]
---

# Graphics — a LIT particle emitter has no valid pipeline

## Why

Measured on projet-alpha's `citadel` (2026-09-28): a `Component::ParticlesEmitter` on the store's
`smoke_001` sprite, its renderable instance switched to lit (`enableLighting()`, or a sprite
manifest declaring `"Lit": true`), fails to build its program:

- `VUID-VkGraphicsPipelineCreateInfo-Input-07904` × 20: the vertex shader declares inputs at
  locations 9 and 10 that the particles' vertex input state does not provide;
- `ShaderGenerator: Unable to finalize the graphics pipeline of the program 'RenderableInstanceAmbientPass'`;
- the emitter draws nothing in the colour passes — but its SHADOW pass pipeline is valid, so it
  still casts a dark streak (seen on the keep's wall).

Smoke under a photometric exposure needs to be lit (a self-lit sprite with no emissive strength
is ~1 nit, black at dusk): no smoke on `citadel` until this is fixed.

## What remains

- [ ] Find which per-instance attributes the lit program expects at 9/10 (the instanced sprite path
  vs the lighting generator) and what the particles VBO carries.
- [ ] Make the lit path valid for particles, or refuse it explicitly.
- [ ] Then the store's `smoke_001` manifest can be declared lit.
