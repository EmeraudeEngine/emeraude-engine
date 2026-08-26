---
id: post-device-loss-robustness
title: Post-device-loss robustness — recover or fail-stop cleanly
status: open
priority: medium
scope: Vulkan/Device
opened: unknown
tags: [vulkan, robustness]
---

# Post-device-loss robustness — recover or fail-stop cleanly

## Why

The engine does not yet recover, nor fail-stop cleanly, after `VK_ERROR_DEVICE_LOST`. The primary
rule stands — a device loss must not be reachable through scene content in the first place — but
the secondary behaviour is undefined today.

Recorded as an open item in `docs/caution-points.md` (§ device loss); it had no entry of its own in
the historical `TODO.md`.

## What remains

- [ ] Decide between recovery (recreate device + resources) and clean fail-stop, then implement
  the chosen one. ⚠️ Both are real work; "it aborts" is neither.
