---
id: program-failure-destroys-visual
title: One failed shader program destroys the whole visual component, for good
status: open
priority: unranked
scope: Graphics/RenderableInstance, Scenes/Component/Visual, Scenes/AbstractEntity
opened: 2026-09-26
tags: [robustness, shaders, lifecycle]
---

# One failed shader program destroys the whole visual component, for good

## Why

A shader program that fails to generate for ONE render target and ONE render pass marks the WHOLE
renderable instance broken (`Graphics/RenderableInstance/Abstract.cpp`, `setBroken()` after
`generateShaderProgram()` fails). `Visual::shouldBeRemoved()` then answers true, and
`AbstractEntity` removes the component at its next logic cycle
(`[Warning][AbstractEntity] Removing automatically a component from entity '…'`). The entity loses its
visual permanently: nothing ever rebuilds it, even when the target or pass that failed stops being used.

Observed 2026-09-26 on `game-logic`. Before the fix of
`SceneRendering::isAdvancedRendering()` (engine `docs/caution-points.md` § *KeyPad4 broke every lit
shader*), KeyPad4 switched the renderer to the direct path. The ambient-pass programs for the swapchain
target failed to compile: 132 failures, `BuildingA`–`D`, `HealthPackMesh`, `BatteryMesh`, `ID/flyer`.
Every city building lost its `Visual` component. Pressing KeyPad4 again brought the post-processing
back, but not the buildings. A transient or target-specific shader defect therefore becomes a
destructive, irreversible scene edit.

## What remains

- Decide with the owner what a program failure owes the instance (design not chosen):
  - broken **per (target, pass)** — the instance is skipped for that target only, and still drawn
    elsewhere (the shadow maps, the scene target);
  - retried when the target changes, instead of latched for the instance's life;
  - or kept as is, but never escalated to a component REMOVAL: a skipped draw is visible and
    recoverable, a deleted component is neither.
- Whatever is chosen, the error must stay loud: the Shader Debugger page and the trace are how the
  KeyPad4 defect was found in one run.

## ⚠️ Traps

- Reproducing needs a shader that fails for one target only. Before the `isAdvancedRendering()` fix,
  KeyPad4 on any lit scene was exactly that. Since that fix, it needs a deliberately broken variant.
- `BrokenState` is also set for genuine construction failures (no renderable, skinning resources).
  Those may legitimately stay fatal — separate them from a per-target program failure before changing
  anything.

## References

- `Graphics/RenderableInstance/Abstract.cpp` — `setBroken()` call sites, `getReadyForRender()`.
- `Scenes/Component/Visual.hpp` — `shouldBeRemoved()` (returns `isBroken()`); `Visual.cpp` — `processLogics()`.
- `Scenes/AbstractEntity.cpp` — automatic removal (`shouldBeRemoved()` loop).
