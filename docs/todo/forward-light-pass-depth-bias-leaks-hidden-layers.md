---
id: forward-light-pass-depth-bias-leaks-hidden-layers
title: Forward light passes add the lighting of hidden layers within their depth bias (foliage reads too bright)
status: open
priority: high
scope: Vulkan/GraphicsPipeline, Scenes/Scene.rendering
opened: 2026-10-04
tags: [lighting, forward, depth, foliage]
---

# Forward light passes add the lighting of hidden layers within their depth bias

## Why

Every forward light pass tests depth `LEQUAL` with depth write off and, by default, a slope-scaled depth bias of
−1 (`Vulkan/GraphicsPipeline.cpp`, "Default behavior to remove artefact when adding light over the ambient pass").
The lit list is drawn batch-major (ambient, then the lights, per batch), so when a light pass runs the depth buffer
holds the batch's own front layer — and every fragment of a HIDDEN layer within the bias of it passes the test and
ADDS its lighting. Dense near-coplanar layers in one batch (foliage cards: Sponza's `IvySim_Leaves`, its cypress)
are counted several times.

Measured 2026-10-04 (Sponza at night, the 22 lamps alone, deferred resolve as reference): with the bias, the ivy
reads brighter in forward on 20.8 % of a test window (forward brighter on 99.8 % of the differing pixels); with the
slope factor at 0 (a local experiment, reverted) forward and deferred agree to 0.2 % of the pixels over 2 levels and
0.03 % of the lamp energy (engine `docs/subsystems/graphics/38-deferred-punctual-lights.md` § Measured).

## What remains

- Decide the intended mechanism (owner): drop the bias and make the light passes' position bit-identical to the
  ambient pass's (`invariant gl_Position`, same vertex code) then test `EQUAL`; or keep LEQUAL and prove which
  artefact the bias was added for (none was visible on Sponza with it at 0, on this GPU).
- Every light still drawn forward is concerned: shadowed / projected lights, line lights, the sun, every material the
  deferred resolve does not take, every target without a G-buffer.

## ⚠️ Traps

- Measure at NIGHT (`sponza --demo-options 1,60`): in daylight the lamps are invisible and so is the leak.
- A slope bias of 0 without a bit-identical position can z-fight (light missing in speckles): check both directions.
