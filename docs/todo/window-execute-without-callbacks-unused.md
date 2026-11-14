---
id: window-execute-without-callbacks-unused
title: Window::executeWithoutCallbacks() has no caller left and runs its function under the native-surface lock
status: open
priority: unranked
scope: Window (executeWithoutCallbacks)
opened: 2026-11-14
tags: [window, clean-up, wayland, lock]
---

# Window::executeWithoutCallbacks() has no caller left and runs its function under the native-surface lock

## Why

`Window::executeWithoutCallbacks()` (`Window.hpp:827-908`) disables every GLFW callback, polls the events, runs the
caller's function, then restores the callbacks. It was written for Windows, where `vkCreateSwapchainKHR()` could
deadlock when a GLFW callback fired during a swap-chain recreation. On 2026-11-14 a search of the engine, of
projet-alpha and of a downstream application found no caller.

It also contradicts the rule set the same day (`docs/caution-points.md` § *Fixed: the close button hung the
application on Wayland*): it takes the native-surface lock and runs the caller's function UNDER it. A function that
waits for the rendering thread there would hang exactly as the close button did. Nothing reaches it today.

## What remains

- Decide (owner): remove it (a public method of `Window`: an API removal, to announce in `CHANGELOG.md`), or keep it
  and release the lock before the function runs (the pump only needs it).
- If it is removed: check whether the Windows deadlock it guarded against is covered elsewhere today (the swap-chain
  recreation path) — ask the Windows peer for the recreation under a resize storm.

## References

- `docs/caution-points.md` § *Fixed: the close button hung the application on Wayland — a handler served under the
  native-surface lock (2026-11-14)*, the rules.
