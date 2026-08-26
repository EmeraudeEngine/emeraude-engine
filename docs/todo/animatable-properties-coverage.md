---
id: animatable-properties-coverage
title: Make every scene object animatable — a pass over all of them
status: open
priority: unranked
scope: Animations
opened: unknown
tags: [animation, scene]
---

# Make every scene object animatable — a pass over all of them

## Why

**Owner intent (recorded 2026-08-26):** a pass over **all scene objects** so that each exposes
what can be animated on it. Today the coverage is partial and nothing says which object exposes
what.

This is the prerequisite of the timeline sequencer (`scene-animation-sequencer.md`): a sequencer
can only address channels that exist.

## What remains

- [ ] Go through every animatable object, list what it exposes, and add what is missing.
- [ ] Make the exposed set discoverable (the sequencer must be able to ask an entity for its
  animatable properties rather than carry a hard-coded table).
