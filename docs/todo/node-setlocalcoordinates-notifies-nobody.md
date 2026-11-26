---
id: node-setlocalcoordinates-notifies-nobody
title: setLocalCoordinates() moves an entity without telling its components nor its sub-nodes
status: open
priority: unranked
scope: Scenes (Node, StaticEntity, LocatableInterface), Scenes/Component (SoundEmitter and every component that follows through move())
opened: 2026-11-26
tags: [scenes, components, audio]
---

# setLocalCoordinates() moves an entity without telling its components nor its sub-nodes

## Why

Measured on 2026-11-26 (audio doc 15). `Node::setLocalCoordinates()` and `StaticEntity::setLocalCoordinates()` write
the frame and nothing else, while every other mutator of `LocatableInterface` (`setPosition()`, `move()`,
`rotate()`, `lookAt()`...) ends with `onLocationDataUpdate()`, which calls `move()` on the components and goes down
the sub-nodes. A component that follows its entity through `move()` only stays where the last notified displacement
left it:

- projet-alpha `water-world`, the player's base node repositioned by `Act.setPosition` (which calls
  `setLocalCoordinates()`) from y = 60 to y = 10 then −8: the head's microphone kept the listener at y = 60 (read
  each 120 cycles) and no `onContainerMove()` reached the head node, until the head turned (`Act.lookAt`);
- the head's sound emitter stayed 18 m behind the listener once the microphone was fixed: −25 dB by distance alone.

The microphone now states the listener every cycle (`Microphone::processLogics()`, like the camera): the listener is
right whatever moved the entity. The sound emitter, the lights and every other component still depend on `move()`.

**Not ranked: a defect or the contract?** Nothing says whether `setLocalCoordinates()` is meant as a raw setter (a
frame written while an entity is built, before its components exist) or forgot the notification. The owner decides.
Five call sites in the engine (`NodeAnimation` twice and `Vehicle` once, both EVERY cycle on every node they drive;
the "local coordinates" key of the animation interface in `Node` and `StaticEntity`), ten in projet-alpha (the
player's spawn in two demos, the act's three placements and its two console placements, a car's steering pivot, a
living actor's head, the actor's own forwarder).

## What remains

- The owner's decision: `setLocalCoordinates()` notifies like its siblings (then each call site is read: one that
  runs before the entity is in a scene must stay harmless), or it is documented as raw and the callers that move a
  LIVE entity call the notifying mutators (`Act.setPosition` first).
- Whether `SoundEmitter` should state its position every cycle like the camera and the microphone, whatever the
  answer above.

## References

- `src/Scenes/Node.hpp` (`setLocalCoordinates()`), `src/Scenes/StaticEntity.hpp`, `src/Scenes/Node.cpp`
  (`onLocationDataUpdate()`), `src/Scenes/AbstractEntity.cpp` (`onContainerMove()`).
- `docs/subsystems/audio/15-the-listeners-medium.md` § The microphone states the listener every cycle.
