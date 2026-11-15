---
id: load-finished-runs-scene-handlers-on-a-pool-thread
title: A loading resource reaches the scene's components and entities unsynchronized — its bounding volumes read while it loads, LoadFinished handled on the loader's pool worker
status: open
priority: high
scope: Scenes/Component (Visual, MultipleVisuals, ParticlesEmitter, CloudVolume, SoundEmitter, the light emitters), Scenes/AbstractEntity, Resources/ResourceTrait
opened: 2026-11-15
tags: [tsan, concurrency, defect, measured]
---

# A loading resource reaches the scene's components and entities unsynchronized — its bounding volumes read while it loads, LoadFinished handled on the loader's pool worker

## Why

`Resources::ResourceTrait::finalizeDependencies()` (`src/Resources/ResourceTrait.cpp`) emits `LoadFinished` on the thread
that completed the last dependency: a `Base::ThreadPool` worker for an asynchronous load. The observers' handlers run
there, synchronously (base `ObservableTrait::notify()`), and several scene components answer by WRITING their own state
and by notifying their entity:

| Observer | What its handler does on the pool worker |
|---|---|
| `Component::MultipleVisuals::onNotification()` | `updateRenderBounds()` (writes `m_renderBoundingBox`, `m_renderBoundingSphere`), then `notify(ComponentContentModified)` |
| `Component::Visual::onNotification()` | `refreshScaledBounds()` (writes `m_scaledBoundingBox`, `m_scaledBoundingSphere`), then `notify(ComponentContentModified)` |
| `Component::ParticlesEmitter::onNotification()` | `notify(ComponentContentModified)` |
| `Component::CloudVolume::onNotification()` | `registerShape()` |
| `Component::SoundEmitter::onNotification()` | `playAttachedSound()` / `releaseAttachedSound()` |
| `Component::AbstractLightEmitter::onNotification()` | `registerColorProjectionInBindless()` |

`ComponentContentModified` reaches `AbstractEntity::onNotification()` → `updateEntityProperties()` (the entity's mass,
drag, inertia, bounding primitives, collision flag, `updateVisualDebug()`) → `onContentModified()` →
`notify(EntityContentModified)` → `Scene::onEntityContentModified()` (the octrees). Only that LAST link expects another
thread (it takes the octree mutexes; its comment says "a notification from any other thread waits for the lock as
before"). Everything before it is plain state, written by the pool worker while the thread that owns the entity — the
logic thread once the scene runs, the application's build thread before — reads or writes it.

**Measured** (ThreadSanitizer, 2026-11-15, Debug + TSan on llvmpipe, the testbed):

| Run | Reports | Pool worker's side | The other thread |
|---|---|---|---|
| `terrain` (one run) | 3 | `MultipleVisuals::onNotification()` → `updateRenderBounds()` → `AACuboid::merge()` on `m_renderBoundingBox` | the application's scene-building thread, INSIDE the component's constructor: `updateRenderBounds()` → `AACuboid::reset()` (from `Scenes::Toolkit::bakeTreeImposter()`) |
| a switch to `animation-debug`, before the logic ↔ rendering fixes of that day (two runs) | 6 and 12 | `Visual::onNotification()` → `notify(ComponentContentModified)` → `AbstractEntity::updateEntityProperties()`: `setRenderingAbilityState()` (the entity's flags), the bounding box `reset()`, the collision model read and `BoxCollisionModel::resetShapeParameters()` | the scene-building thread on the same entity: `AbstractEntity::setCollisionModel()`, `Scene::checkEntityLocationInOctrees()` (`isRenderable()`, `getWorldRenderBoundingBox()`), the construction of the `BoxCollisionModel` |
| the same switch, after them (three runs) | 3, 4, 4 | the same `updateEntityProperties()` from `Visual::onNotification()`: the entity's flags, its bounding box `reset()` | **the logic thread**: `Scene::processLogics()` → `checkEntityLocationInOctrees()` → `AbstractEntity::isRenderable()` / `getWorldRenderBoundingBox()` |
| those three runs, the READ side | 3, 3, 1 | `Geometry::IndexedVertexResource::cacheBoundingVolumes()` (from `load()`, and again from `createOnHardware()` in `onDependenciesLoaded()`) WRITES the geometry's box | the scene-building thread READS it: `AbstractEntity::linkComponent()` → `updateEntityProperties()` → `component->renderBoundingBox()` → `AACuboid::merge()` (a `Visual` added while its geometry loads; here by `enableVisualDebug()`) |

In the first three rows the pool worker's stack is `Geometry::…Resource::load()` → `ResourceTrait::setLoadSuccess()` →
`checkDependencies()` → `finalizeDependencies()` → `notify()`.

For the `terrain` case: the constructor subscribes (`this->observe(renderable.get())`) BEFORE it computes the bounds, so
the notification can land on an object whose constructor has not returned. `Component::Visual` has the same shape
(`observe()`, then `refreshScaledBounds()`).

What it can do, by reading the code (the effects themselves were not observed):

- `MultipleVisuals`: each pass is "reset, then merge every corner", so when both threads see the loaded source the BOX
  ends on the right values whatever the interleaving (the thread that resets last merges everything after it) — but the
  SPHERE is derived from the box as it is at that instant: a pass that reads the box between the other pass's reset and
  its last merge writes a too-small sphere, and keeps it if it writes last. No later notification repairs it (the load
  is finished): the component is culled too early for the rest of its life.
- The entity: its bounding box is reset and rebuilt by the pool worker while the logic thread files the entity in the
  octrees from that same box (`isValid()` read between the reset and the rebuild: the entity filed as a point, or not
  filed), and its collision model's shape parameters are rewritten while it is being installed.
- The read side: an entity built from a geometry whose box is half written gets a wrong extent; the completion handler
  recomputes it afterwards — on the pool worker, which is the first half of this item.

Undefined behaviour in every case (unsynchronized writers and readers).

## What remains

- **Decide the contract (owner)** — where a load completion is HANDLED for a scene object:
  1. the handler only records it (an atomic flag on the component); the component consumes it on the logic thread
     (`processLogics()`, or a scene pass for the entities the logic does not tick: confirm which entities have none);
  2. the scene queues the completions and dispatches them at a fixed point of the logic cycle (the model of
     `Scene::m_physicsDeferredContent`), one mechanism for every component;
  3. a lock per component around the state the handler writes (the readers are the culling and the renderer's visit:
     a per-frame cost, and the entity link stays unordered).
- **The read side**: a component reads its renderable's bounding volumes only once they are published. Since
  2026-11-15 `Renderable::Abstract::isReadyForInstantiation()` is an atomic stored LAST by the loading worker (scenes
  doc 12): gating `renderBoundingBox()` / `boundingBox()` on it orders the read after `cacheBoundingVolumes()` — check
  that every renderable type stores it after its geometry's last write, and what an entity's extent is meanwhile
  (empty: not filed in the rendering octree until the completion).
- Whatever the contract: a constructor computes first and subscribes LAST, then covers "finished in between" with the
  check-then-act of `CloudVolume` (`src/Scenes/Component/CloudVolume.cpp`, `isLoaded()` → `observe()` → `isLoaded()`).
- The audio and bindless handlers (`SoundEmitter`, `AbstractLightEmitter`, `CloudVolume`) call into services from the
  pool worker: read which of those calls are documented for any thread before deciding they are in scope.
- Proof: TSan on `terrain` and on a switch to `animation-debug` (the recipe: `docs/caution-points.md` § *TSan on a
  machine whose display GPU runs the NVIDIA driver*), 0 report on these members.

## ⚠️ Traps

- `terrain` under TSan on llvmpipe takes about four minutes to start; its three reports appear during the build of the
  act (the forest's imposters are baked while the pool still loads their geometry), not during the run.
- The `animation-debug` reports appear within two seconds of the load (the scene is active while its last resources
  finish); the software device then dies on that scene (`skinned-scene-crashes-the-software-vulkan-device`), which does not matter here: the reports are written first.
- A `default` run never shows it (six runs that day, 0 report of this family): its two objects are loaded before
  anything observes them.
- Moving the handler to the logic thread changes WHEN an entity becomes collidable / visible after its load (one logic
  cycle later at most): the testbed's scenes that wait for a resource before placing a player must be re-run.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the lot whose proof runs found it; the
  publication flag the read side can use).
- `docs/subsystems/scenes/` (the entity / component notification chain), base `ObservableTrait::notify()`.
