---
id: root-motion-mode
title: Root-motion mode for skeletal animation
status: open
priority: medium
scope: Animations
opened: unknown
tags: [animation, humanoid]
---

# Root-motion mode for skeletal animation

## Why

Industry-standard locomotion, and a requirement for production-grade runtime quality on humanoid
characters: extract the root delta per frame from skeletal clips and feed it back to the actor as
actual displacement (foot planting, no foot sliding, animation-driven speed).

## What remains

- [ ] Companion mode to `LoaderOptions::stripRootMotion` (which kills horizontal root translation
  at load): the new mode KEEPS it and routes it through `MovableTrait`.

## Origin

Inherited from the historical root `TODO.md`.
