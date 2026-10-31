---
id: resize-or-non-native-resolution-crash
title: A live resize or a launch at a non-native resolution crashed with a demo loaded (to verify)
status: open
priority: high
scope: Graphics/PostProcessing, Window (swap-chain recreation)
opened: 2026-07-03
tags: [resize, post-processing, device-lost, validation]
---

# A live resize or a launch at a non-native resolution crashed with a demo loaded (to verify)

## Why

Seen on 2026-07-03 with projet-alpha's `liminal` demo loaded (recorded only in projet-alpha
`src/Builtin/Liminal.AGENTS.md` § "Engine bugs found"):
- a live `Core.WindowService.resize(1920,1080)` segfaulted during the post-process stack recreation (the last
  indirect-render-target log line was `BloomUp3`);
- launching with the window settings at 1920x1080 produced storms of the validation message "descriptor set #0 not
  bound", then `VK_ERROR_DEVICE_LOST`.
The demo was only stable at the native framebuffer size (3840x1990). A defect, hence `high` by default. Nobody has
re-checked it since: the post-process stack, the swap-chain recreation and the device-loss handling have changed a
lot (lighting lanes, deferred destruction contract).

## What remains

- FIRST verify on the current tree: with a demo loaded (liminal, then one RT-heavy demo), run a live
  `WindowService.resize` to a smaller and a larger size, and launch with a non-native window size; validation layers
  ON, count VUIDs, note crash or device loss.
- Not reproduced → delete this item and the bullet in projet-alpha `Liminal.AGENTS.md`.
- Reproduced → find the post-process resource that is recreated without its descriptor set being rebound.

## References

- projet-alpha `src/Builtin/Liminal.AGENTS.md` § "Engine bugs found (NOT fixed)"
- Related: `intel-hang-render-thread-frees-memory-during-resize`
