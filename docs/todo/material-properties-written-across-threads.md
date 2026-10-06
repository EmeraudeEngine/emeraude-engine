---
id: material-properties-written-across-threads
title: Material dynamic properties — the floats are written and read by two threads
status: open
priority: unranked
scope: Graphics/Material (StandardResource, BeamResource, PathResource)
opened: 2026-10-06
tags: [threading, robustness]
---

# Material dynamic properties — the floats are written and read by two threads

## Why

A dynamic setter (`StandardResource::setEmissiveStrengthValue()`, `setRoughness()`, …) writes the property array on
the calling thread (logic, console), while `updateVideoMemory()` copies the same array to the UBO on the render
thread (`Renderer::flushMaterialVideoMemoryUpdates()`). Since 2026-10-06 the DIRTY FLAG is atomic and lowered before
the copy (no lost update), but the floats themselves are a data race: formally undefined behaviour, in practice a
property one frame old. It became a routine path that day: `AbstractLightEmitter::linkEmissiveMaterial()` writes a
material at every intensity change of a flickering light (55 lamps in `labyrinth`, every logic cycle).

## What remains

Options for the owner: a per-material mutex around the setter writes and the copy (cheap, the copy is 64 floats); a
published copy per render state slot, like the lights (`AbstractLightEmitter::publishStateForRendering()`); or a
frame-partitioned material UBO (the caution-points entry names it as the day a property must be exact per frame).

## References

- `docs/caution-points.md` § Fixed: every "dynamic" material property was DEAD after creation (addendum 2026-10-06).
