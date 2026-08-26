---
id: remove-dead-camera-velocity-view-ubo
title: Remove the dead camera velocity vector from the view UBOs
status: open
priority: low
scope: Graphics/ViewMatrices
opened: unknown
tags: [cleanup, ubo]
---

# Remove the dead camera velocity vector from the view UBOs

## Why

`ViewMatrices*UBO::updateViewCoordinates()` uploads a velocity vector (`VelocityVectorOffset`)
every frame and the generated GLSL view blocks declare it, but **no shader reads it**.

## What remains

- [ ] Remove it from the UBO layout and from the generated view blocks.

## ⚠️ Traps

- The velocity **parameter** itself must stay: it feeds the OpenAL listener/doppler on the audio
  side of the AVConsole contract.
- Motion vectors do NOT use it (they need the previous view-projection matrix, not a linear
  velocity) — do not "reuse" it there.

## Origin

Inherited from the historical root `TODO.md`.
