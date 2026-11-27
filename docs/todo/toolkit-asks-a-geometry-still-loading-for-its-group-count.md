---
id: toolkit-asks-a-geometry-still-loading-for-its-group-count
title: Toolkit::generateRenderableInstance() asks a geometry still loading for its sub-geometry count
status: open
priority: high
scope: Scenes/Toolkit (generateRenderableInstance(), the Mesh / SimpleMesh dispatch), Graphics/Geometry (IndexedVertexResource::subGeometryCount(), Interface::buildSubGeometries())
opened: 2026-11-27
tags: [defect, concurrency, threadsanitizer, wrong-result]
---

# Toolkit::generateRenderableInstance() asks a geometry still loading for its sub-geometry count

## Why

ThreadSanitizer, 2026-11-27, macOS (Apple M6, MoltenVK), the testbed's `city` window-less: 2 data races in one run,
the same pair twice (through the tile builder's instantiation, then through the street lamps).

- **The reader, the thread that builds the scene** — `Toolkit::generateRenderableInstance()` chooses between
  `MultiLayerMeshResource` and `SimpleMeshResource` on `geometryResource->subGeometryCount() > 1`
  (`IndexedVertexResource::subGeometryCount()` reads `m_subGeometries`).
- **The writer, a thread of the pool** — the geometry the building thread has just asked for
  (`ResourceGenerator::shape()` → `Container::getOrCreateResource()`, the creation function runs later on a worker):
  `IndexedVertexResource::load(Shape)` → `setLoadSuccess()` → `Interface::onDependenciesLoaded()` →
  `IndexedVertexResource::createOnHardware()` → `Interface::buildSubGeometries()`, which clears and fills
  `m_subGeometries`.

Besides the race, the answer is WRONG when the read comes first: a geometry with several groups answers 1 and gets a
`SimpleMeshResource`, one material for all its groups. It is the trap of `docs/caution-points.md` § *Inside `load()`,
a dependency is NOT loaded — never ask one about its content*, at another site; it depends on how busy the pool is,
so it looks like a random defect.

Not reproduced on Linux that day (three ThreadSanitizer runs of `city`, 0 report): a rate, not an absence.

## What remains

- The owner's choice of where the count comes from: what the caller already knows (the shape's group count, read
  before the geometry is asked for and passed along), a wait (`ResourceTrait::waitUntilLoaded()` — refused from a
  pool worker, so every caller of the Toolkit must be a building thread), or a renderable that decides in its own
  `onDependenciesLoaded()` (the geometry is loaded there).
- Every other site that asks a resource just obtained from `getOrCreateResource()` / `getResource()` about its
  content (`/usr/bin/grep -rn "subGeometryCount()" src`).
- Proof: `city` under ThreadSanitizer on macOS, 0 report naming `generateRenderableInstance` or
  `buildSubGeometries`; a geometry with two groups always drawn with its two materials.

## References

- `src/Scenes/Toolkit.hpp` (`generateRenderableInstance()`), `src/Graphics/Geometry/IndexedVertexResource.hpp`
  (`subGeometryCount()`), `src/Graphics/Geometry/IndexedVertexResource.cpp` (`createOnHardware()`),
  `src/Graphics/Geometry/Interface.cpp` (`buildSubGeometries()`, `onDependenciesLoaded()`).
- `docs/caution-points.md` § *Inside `load()`, a dependency is NOT loaded*; `docs/subsystems/resources/07-critical-attention-points.md`
  (a synchronous request does not wait).
