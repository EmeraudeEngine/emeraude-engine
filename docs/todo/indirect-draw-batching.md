---
id: indirect-draw-batching
title: Indirect draw / draw-call batching
status: open
priority: medium
scope: Graphics/Renderer
opened: unknown
tags: [gpu-driven, performance]
---

# Indirect draw / draw-call batching

## Why

Part of the GPU-driven roadmap: use `vkCmdDrawIndexedIndirect` to batch draws by pipeline/material
and cut per-draw CPU overhead.

## What remains

- [ ] Batch by pipeline/material through indirect draws.

## ⚠️ Traps

- MDI already exists and is **OFF by default** with one known defect — see
  `mdi-wrong-texture-after-first-frame.md`. Do not build a second indirect path without settling
  that one first.
- Dynamic rendering (`vulkan-sync2-dynamic-rendering.md`) is the lever that makes ordering by
  pipeline layout worthwhile.

## Origin

Inherited from the historical root `TODO.md`.
