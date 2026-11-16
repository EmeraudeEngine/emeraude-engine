---
id: renderable-flags-read-while-its-loader-writes-them
title: A renderable's flag word is written by its loader while a component built on it reads isSprite()
status: open
priority: high
scope: Graphics/Renderable (Abstract: the flag word, isSprite(); SpriteResource::load()), Graphics/RenderableInstance::Multiple (updateLocalData()), Scenes/Component/MultipleVisuals (its constructor)
opened: 2026-11-16
tags: [tsan, concurrency, defect, measured]
---

# A renderable's flag word is written by its loader while a component built on it reads isSprite()

## Why

ThreadSanitizer, 2026-11-16, the second sweep of the testbed's scenes (Debug, the software device): `sprite` reports
one data race on a renderable's flag word. It did not show in the first sweep: the two threads have to meet.

| Written by | Read by |
|---|---|
| the thread loading the sprite: `FlagTrait::enableFlag()` ← `Renderable::SpriteResource::load()` ← `Resources::Container< SpriteResource >::loadingTask()` | the thread building the scene: `Renderable::Abstract::isSprite()` ← `RenderableInstance::Multiple::updateLocalData()` ← `Component::MultipleVisuals::applyWorldFrame()` ← the `MultipleVisuals` constructor |

The scene asks for the sprite (an asynchronous load) and builds a `MultipleVisuals` on it at once; the component's
constructor lays its instances out and asks the renderable whether it is a sprite — the layout of the instance buffer
depends on the answer — while the loader is still writing the word that holds it. The "ready" bit of that word became
an atomic on 2026-11-15 on the grounds that the other bits are written BEFORE the renderable is published (scenes doc
12): they are written during the load, and a component may be built before its end. Besides the race, an answer read
too early lays the instances out for the wrong kind of renderable.

## What remains

- Decide where the kind of a renderable is known (owner): a property of the TYPE (a sprite resource is a sprite before
  it loads: a virtual or a constructor-time constant, never a flag its loader sets), or the component waits for the
  renderable before laying its instances out, or the word becomes atomic. The first removes the question for every
  consumer.
- Audit the other bits of `RenderableFlagBits` written by a loader and read before the load ends.
- Proof: `sprite` under ThreadSanitizer, several runs (the race shows in one run of two at best).

## ⚠️ Traps

- Not a logic / rendering race: a loading thread against the thread building the scene. Same family as
  `sky-material-atmosphere-read-while-its-creation-sets-it` and scenes doc 19 § *onMeshLoaded hands out resources
  still being created*: an object used while its load or its creation runs.
- The recipe of a TSan run on the software device: `docs/caution-points.md` § *The software device gives a ray-query
  hit the identity of another instance*.

## References

- Scenes doc 12, the second sweep of 2026-11-16, and the row "A renderable is ready for instantiation".
