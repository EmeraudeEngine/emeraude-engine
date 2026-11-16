---
id: tsan-logic-render-instance-state-races
title: A configuration written on a LIVE object from another thread (an instance's tangent-space toggle, an entity's flag word) — what is left once the six states ThreadSanitizer reported have a protocol
status: in-progress
priority: high
scope: Graphics/RenderableInstance::Abstract (the configuration setters, the flag word), Scenes/AbstractEntity (its flag word: isRenderable()), Scenes/Manager.console.cpp (the tangent-space toggle)
opened: 2026-11-15
tags: [tsan, concurrency, defect]
---

# A configuration written on a LIVE object from another thread (an instance's tangent-space toggle, an entity's flag word) — what is left once the six states ThreadSanitizer reported have a protocol

## Why

Both threads hold the active scene's access SHARED, so nothing orders a plain write on one side against a read on the
other (scenes doc 12 § *Plain state shared by the logic and rendering threads*).

**Fixed on 2026-11-16** (owner decisions; the protocols, the proofs and the numbers are in scenes doc 12, the last
seven rows of its table): an instance's animation time, its flag word's three state bits, the view read on the
rendering side — its position, its matrices, its near and far planes, `RenderTarget::Abstract::viewDistance()` removed
—, the scene's ambient as the traced effects add it, and the extent a frame culls an entity on. The sweep of the
testbed's scenes under ThreadSanitizer, twice: NO report is left between the logic and the rendering threads.

**What is left was never reported — found by reading**:

- An instance's flag word now holds configuration only, set when its component is created. ONE caller writes it on a
  live instance from another thread: the console command that toggles the tangent-space display
  (`Scenes/Manager.console.cpp` → `RenderableInstance::Abstract::enableDisplayTBNSpace()`), on the main thread, while
  the rendering thread reads the word (`isLightingEnabled()`, `isShadowCastingDisabled()`…). No run sent that command.
- An ENTITY has a flag word too: `AbstractEntity::isRenderable()` is read by the rendering thread (the two list
  builders) and written by `updateEntityProperties()` on the logic thread, each time its components change.

## What remains

- **The rule for a configuration change on a live object — an owner decision**: e.g. the word becomes atomic (one
  mechanism for every bit, a locked operation per write), or a configuration change goes through the logic cycle and
  is published like a state, or the bits that change while the scene runs leave the word as the three state bits did.
- Apply it to the instance's word and to the entity's, then send the console command in a ThreadSanitizer run.
- **The peers' round** on the fixed members (macOS, Windows), after the push.

## ⚠️ Traps

- The recipe and the traps of a TSan run on the software device: `docs/caution-points.md` § *The software device
  gives a ray-query hit the identity of another instance*.
- ThreadSanitizer stops repeating a report, and a family hidden behind another shows only once the first is fixed (the
  far plane appeared after the culling was fixed): run the sweep again after each fix.
- The reports that remain in the sweep are LOADING threads, other items:
  `sky-material-atmosphere-read-while-its-creation-sets-it`, `renderable-flags-read-while-its-loader-writes-them`, and
  the testbed's `city-reads-an-image-average-colour-while-it-loads`.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the table, the rule for choosing a
  protocol, the proofs, the two sweeps, the cost of the published extent).
