---
id: light-cull-sphere-ignores-local-centre-and-scale
title: The forward light cull centres the bounding sphere on the node origin and ignores the renderable's scale
status: open
priority: high
scope: Scenes/Scene.rendering
opened: 2026-10-04
tags: [lighting, forward, culling]
---

# The forward light cull centres the bounding sphere on the node origin and ignores the renderable's scale

## Why

`Scene::renderLightedSelection()` tests each point / spot / line light against the batch's world sphere built as
`Sphere{localSphere.radius() * max(frame scale), batchCoordinates->position()}`: the renderable's LOCAL sphere centre
(`boundingSphere().position()`) is dropped and so are the instance's local transformation and the renderable's
`uniformScale()` (which the file's own `worldRadius()` helper applies). A mesh whose bounds sit away from its node
origin, or a scaled renderable, can lose a light that does reach it — the forward pass is then simply missing.

Found reading the code during the deferred resolve's A/B (2026-10-04). A fix (model matrix +
`applyLocalTransformation()` for the centre, `worldRadius()` for the radius) changed NOTHING measurable on Sponza —
its spheres are wide enough — so it was not kept: no fix without proof.

## What remains

- Find or build a case that shows it (an off-origin mesh at the edge of a lamp's radius), measure, then fix as above.
- The deferred resolve is not concerned (no per-object cull).
