---
id: terrain-forest-stalls-without-validation
title: The terrain forest stalls the presentation for 60 s at load, then exits, when the validation layers are OFF
status: open
priority: unranked
scope: Graphics
opened: 2026-09-23
tags: [terrain, vegetation, synchronization, wayland]
---

# The terrain forest stalls the presentation for 60 s at load, then exits, when the validation layers are OFF

## Why

Measured on Linux (RTX 3070 Ti, Wayland), `projet-alpha --load-demo terrain --demo-options 100000,25` with its
210 000-tree forest:

- validation layers **ON** (`Core/Video/VulkanInstance/EnableDebug`): starts in ~12 s, runs at ~31 ms, 0 VUID —
  every such run, six of them;
- validation layers **OFF** (with or without the GPU profiler, ray-traced lane or screen-space): the log shows
  `The acquisition of the next image was canceled by the 60000000000 ns timeout!` right after `The scene 'terrain'
  successfully loaded`, then `Cleanup the stage` immediately after `The application successfully started` and a
  clean exit (code 0) — every such run, four of them. Before the imposters, the same configuration gave 184 s frames.
- `forest` (792 trees) and the BARE terrain (`--demo-options 100000,25,0,0,1`) start normally without validation.

A race that the validation's slowdown hides is the first suspect (the stall happens while the main thread builds and
registers ~18 000 instanced visuals); under Wayland a main thread that does not pump the display events for 60 s
also blocks `vkAcquireNextImageKHR`. Not attributed. The exit after the stall is not explained either.

⚠️ **Not to be confused with the one-second FREEZE (FIXED 2026-09-23)**: `terrain` frozen for good about a second
after its start was a logic/render DEADLOCK over the render-target list lock (`docs/caution-points.md` § *Holding a
render-target list lock across its callback*). This item is the other pattern: two 60 s acquisition timeouts BEFORE
`successfully started`, then a clean exit. Some of those runs overlapped another projet-alpha instance on the same
machine (`Failed to bind to 127.0.0.1:7777` in one log): re-measure with a single instance before anything else.

## What remains

- Reproduce without validation, with thread stacks during the stall (`gdb -p`, `thread apply all bt`).
- Tell a GPU wait from a starved event loop: the acquire timeout says nothing of which one.

## References

- `projet-alpha` `src/Builtin/AGENTS.md` § 6d (the forest), `src/Graphics/AGENTS.md` § 15e (the imposters).
