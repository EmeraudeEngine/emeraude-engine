---
id: tsan-logic-render-instance-state-races
title: TSan — the rendering thread's culling reads an entity's LOGIC state (coordinates, render box, collision model); four other states of this family are fixed
status: in-progress
priority: high
scope: Scenes (AbstractEntity, Node, StaticEntity, Scene.rendering — populateRenderLists(), populateShadowCastingRenderList()), Physics::Particle (isVisibleTo)
opened: 2026-11-15
tags: [tsan, concurrency, defect, measured]
---

# TSan — the rendering thread's culling reads an entity's LOGIC state (coordinates, render box, collision model); four other states of this family are fixed

## Why

Both threads hold the active scene's access SHARED, so nothing orders a plain write on one side against a read on the
other (scenes doc 12 § *Plain state shared by the logic and rendering threads*).

**Fixed on 2026-11-16** (owner decisions; the protocols, the proof and the numbers are in scenes doc 12, the last six
rows of its table): an instance's animation time (a render state slot), its flag word (three bits left it, each to its
owner's protocol), the view's logic state read on the rendering side (the frame's view, five readers), the scene's
ambient as the traced effects add it (the frame's view). ThreadSanitizer: `light-and-shadow-debug` 1 report → 0,
`game-logic` 4 → 0.

**What the sweep found the same day** (the testbed's 40 scenes under ThreadSanitizer, the final code, one run each:
33 clean). On `node-relation`, `collision-debug`, `beams`, `paths`, `city` and `citadel`, the rendering thread reads an
entity's logic state while the logic thread writes it:

| Read by the rendering thread | Written by the logic thread |
|---|---|
| `Node::isVisibleTo()` / `StaticEntity::isVisibleTo()` → `AbstractEntity::getWorldRenderBoundingBox()` → `getWorldCoordinates()` (a node walks its parents' LOGIC frames) — from `Scene::populateRenderLists()` (`rasterFrom`) and `Scene::populateShadowCastingRenderList()` | `Node::move()` / `rotate()` ← `moveFromPhysics()` / `rotateFromPhysics()` ← `Scene::resolveCollisions()`; `Node::setLocalCoordinates()` ← `Component::Vehicle::processLogics()`; `StaticEntity::yaw()` ← `playAnimation()` ← `AnimatableInterface::updateAnimations()` |
| `Node::isVisibleTo()` → `getWorldCoordinates()` then `collisionModel()->getAABB()` (the fallback without a render box) | the same writers; the collision model itself is refreshed by `refreshCollisionBoundaries()` at every pose of a skinned visual |
| `AbstractEntity::getWorldRenderBoundingBox()` in `missesCascadeReceivers` (`populateShadowCastingRenderList()`) | the same writers |

The caller already holds the entity's PUBLISHED coordinates (`getWorldCoordinatesStateForRendering(readStateIndex)`),
and tests the visibility on the live ones. `m_renderBoundingBox` (the model-space box the test starts from) is written
by the logic thread too (`refreshScaledBounds()`, an animated visual's box). ⚠️ `AbstractEntity.hpp`
(`refreshCollisionBoundaries()`) calls this "the same benign one-frame race as the live getWorldCoordinates() read":
a comment that tolerates a race is a defect (Ave Robustus II).

## What remains

- **The culling's protocol — an owner decision.** Candidates: (A) the entity PUBLISHES its world extent for rendering
  per render state slot (what `Component::ParticlesEmitter` does with `m_renderWorldBounds`), computed on the logic
  thread at the publication, and the two list builders test the frustum against the slot's extent; (B) the visibility
  test takes the slot's coordinates and computes the box on the rendering thread — which leaves the model-space box and
  the collision model shared.
- `Physics::Particle::isVisibleTo()` implements the same interface: read it when the protocol is chosen.
- **One configuration bit is still written while the scene runs**: the console command that toggles an instance's
  tangent-space display (`Scenes/Manager.console.cpp`, `enableDisplayTBNSpace()`) writes the instance's flag word from
  the main thread while the rendering thread reads it. Not reported (the command was not sent in any run). The rule for
  a configuration change on a live instance is to decide.
- **The peers' round** on the four fixed members (macOS, Windows), after the push.
- Proof when done: the six scenes above, 0 report; the sweep again.

## ⚠️ Traps

- The recipe and the traps of a TSan run on the software device — the settings keys without which a ray-traced scene
  kills it, the third key for `sponza`: `docs/caution-points.md` § *The software device gives a ray-query hit the
  identity of another instance*.
- The sweep's two other families are NOT this item: `sky-material-atmosphere-read-while-its-creation-sets-it` (two
  loading threads), and the testbed's `city-reads-an-image-average-colour-while-it-loads`.
- A scene that moves nothing reports nothing: `default`, `terrain`, `sponza`'s static part are clean of this member
  only because no entity moves there.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the table, the rule for choosing a
  protocol, the proof of the four fixed members and the sweep).
