---
id: scene-animation-sequencer
title: Scene animation sequencer — entities on a timeline, values on channels
status: open
priority: unranked
scope: Animations
opened: unknown
blocked-by: [animatable-properties-coverage]
tags: [animation, scene, tooling]
---

# Scene animation sequencer — entities on a timeline, values on channels

## Why

**Owner intent (recorded 2026-08-26):** general, scripted, scene-wide animation — pick entities
on a **timeline**, set values on their animatable elements, and drive the whole scene from those
**channels**. A cutscene mechanism owned by the runtime, not hand-written code per demo.

## What remains

- [ ] The channel model (an entity + one of its animatable properties + keyed values over time),
  its interpolation and its evaluation per frame.
- [ ] The timeline that owns the channels, plays, seeks and loops.
- [ ] How a sequence is authored and stored (a scene JSON is the natural home — see
  `json-resource-describes-subresources.md`).

## Blocked by

`animatable-properties-coverage.md`: a channel can only address a property an entity actually
exposes.
