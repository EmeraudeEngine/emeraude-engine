---
id: vegetation-baked-occlusion-channel-is-unread
title: The vegetation A channel (baked occlusion) is filled and never read
status: open
priority: unranked
scope: Saphir/LightGenerator, Saphir/Generator/SceneRendering
tags: [vegetation, shaders, lighting]
opened: 2026-09-21
---

# The vegetation A channel (baked occlusion) is filled and never read

## Why

The tree skinner writes four vertex colour channels; the wind shader of 2026-09-21 consumes three
of them (R trunk bend, G branch bend, B flutter phase). The fourth, **A = baked occlusion**, is
written on every vertex of every level of detail and read by nobody. A canopy therefore lights its
inside exactly like its rim, which is the one thing that makes procedural foliage look like a
cloud of floating cards.

## What remains

1. Route the vertex colour to the FRAGMENT stage whenever the vegetation wind is enabled. It
   travels today only when the material asks for vertex colours, and the tree materials are
   texture-based, so the channel does not currently reach the fragment shader at all.
2. Multiply the ambient / indirect diffuse term by `svPrimaryVertexColor.a`. It must NOT touch the
   direct lighting: a leaf in the sun is lit whatever its neighbours do — only what reaches it
   from the sky is occluded.
3. Decide whether it also attenuates the screen-space or traced indirect lane, or only the raster
   ambient. Both lanes already own the indirect diffuse (`IndirectDiffuse`), so applying it twice
   would double-darken the inside of the canopy.

## Traps

- ⚠️ The A channel is a **density estimate, not ray-traced occlusion**: the skinner counts leaves
  and branch nodes in the cell around a point and normalizes on the densest cell. It is a shading
  hint, not a physical quantity — do not feed it to anything that claims to be photometric. See
  emeraude-base `src/VertexFactory/AGENTS.md` § *Vegetation*.
- ⚠️ Measure the result at a PINNED exposure, or the auto-exposure absorbs the change and the
  before/after look identical.

## References

- Producer: emeraude-base `TreeSkinner::ambientOcclusion()`.
- Sibling: `vegetation-wind-does-not-reach-the-shadow-pass`.
