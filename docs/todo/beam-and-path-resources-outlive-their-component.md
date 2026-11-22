---
id: beam-and-path-resources-outlive-their-component
title: A beam's and a path's own resources stay in their stores until the scene closes
status: open
priority: high
scope: src/Scenes/Component (Beam, Path), src/Resources (Container)
opened: 2026-11-22
tags: [resources, memory, beam, path, lifetime]
---

# A beam's and a path's own resources stay in their stores until the scene closes

## Why

`Scenes::Component::Beam` and `Scenes::Component::Path` create THREE resources of their own in their constructor,
under a name unique to the component (`Beam#<n>/<component>`, `Path#<n>/<component>`): a material
(`Material::BeamResource` / `PathResource`), a `Geometry::PulledVertexResource` and a `Renderable::MeshResource`,
through `Resources::Container::getOrCreateResourceSync()`. The containers keep them: when the component is destroyed
(its entity removed) nothing withdraws them, and they are unloaded only when the stores are — at the end of the run.

Measured on 2026-11-22 (projet-alpha `city`, a weapon that created one beam per shot): 161 shots → 161 materials,
161 geometries and 161 renderables alive at the shutdown (`161 resource(s) unloaded !` for each store), their
entities long gone. A scene that spawns transient beams or paths — a lightning strike per event, a trail per
projectile — grows without bound: the material's uniform block, the renderable, and, through the renderable's name
in the program cache key, possibly a program set each (graphics doc 33 § Traps, not measured).

projet-alpha no longer creates a beam per shot (one beam, moved and switched): the defect stays for every other use.

## What remains

- Decide the contract (owner): a component-private resource is not a STORE resource — it has one user and its
  lifetime. Either the component owns it outside the containers (no name, no store entry), or the container offers a
  scoped registration the component's destructor withdraws (through the deferred destruction: a frame in flight may
  still draw it).
- Measure what one beam costs once its component is gone: the uniform block, the program set, the pipeline.
- Check the other components that name a resource after themselves (`/usr/bin/grep -rn 'getOrCreateResourceSync'
  src/Scenes/Component`).

## ⚠️ Traps

- The resources are retired through the deferred destructor, never destroyed while a frame in flight may use them.
- The renderable's name enters the program cache key: a fix that keeps unique names keeps one program set per beam.

## References

- `src/Scenes/Component/Beam.cpp`, `Path.cpp` (the constructors), `docs/subsystems/graphics/33-beams-lasers-and-electric-arcs.md`.
