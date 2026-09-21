---
id: vegetation-wind-shader
title: Vegetation — the wind shader that consumes the generator's vertex channels
status: open
priority: unranked
scope: Saphir, Graphics
tags: [vegetation, shaders, saphir]
opened: 2026-09-21
---

# Vegetation — the wind shader that consumes the generator's vertex channels

## Why

The tree generator reserves its vertex colour channels for wind from the first delivery (owner
decision, 2026-09-21): **R** = trunk bending weight, **G** = branch bending weight, **B** = leaf
flutter phase, **A** = baked AO. Nothing reads them yet. Without the shader the channels are
inert data and the trees are rigid.

## What remains

1. A Saphir vertex-stage contribution: hierarchical bending — a slow, large trunk sway weighted by
   R, a faster branch sway weighted by G, a high-frequency per-leaf flutter phased by B. The
   classic two-scale decomposition, not one global sine.
2. A global wind state (direction, strength, gust envelope, time) as a uniform, updated once per
   frame — not per renderable.
3. The A channel folded into the ambient/indirect term as baked AO.
4. **Motion vectors must follow.** A vertex displaced in the vertex stage and not reported to the
   motion-vector pass smears under TAA and motion blur. This is the part that is easy to forget
   and expensive to debug.

## Traps

- ⚠️⚠️ Anything that moves geometry in the vertex stage changes what the **ray-tracing** lane
  sees: the BLAS holds the *undisplaced* triangles unless it is refit. Decide explicitly whether
  wind is visible to the traced lane, and write the answer down — an inconsistency here shows up
  as reflections and shadows that do not sway with the tree.
- ⚠️ Measure the shimmer, do not eyeball it: `docs/temporal-stability-measurement.md` in
  projet-alpha. Moving foliage under TAA is the textbook shimmer source.

## References

- Producer side: DONE. The skinner fills R/G/B/A on every vertex of every level. See
  emeraude-base `src/VertexFactory/AGENTS.md` § *Vegetation* for what each channel holds, and
  the warning that A is a density estimate rather than ray-traced occlusion.
- Sibling: `vegetation-renderable-and-lod-chain`.
