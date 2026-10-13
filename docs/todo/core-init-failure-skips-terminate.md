---
id: core-init-failure-skips-terminate
title: Prove every Core initialize step fails cleanly — fault injection (the missing terminate() is fixed)
status: open
priority: high
scope: src/Core.cpp, every service initialize*()
opened: 2026-10-08
tags: [ave-robustus-ii, raii, defect, fault-injection]
---

# Prove every Core initialize step fails cleanly — fault injection

## Why
The defect in this item's id is fixed. `Core.cpp` used to `return EXIT_FAILURE` when `initializeCoreLevel()` failed,
without `terminate()`: VkInstance, OpenAL and GLFW leaked, and the audio threads could hang at exit. Since c6c237755
("Ave Robustus II P0: every lifetime ends in its owner, failure paths included"; today `src/Core.cpp:509-516`) it
terminates.
Ave Robustus II requires every failure path to be PROVEN clean, and only one step is so far.

## Done (2026-10-08, P0, c6c237755)
- `Core::run()` calls `terminate()` when `initializeCoreLevel()` fails; `~Core()` terminates whatever user / secondary
  service is still registered (safety net, decision D3-a); `Vulkan::Instance`, `Audio::TrackMixer` and
  `Audio::Recorder` release in their destructors when `onTerminate()` did not run.
- Proof of one real failure: no Vulkan driver (`VK_ICD_FILENAMES=/nonexistent.json VK_DRIVER_FILES=…`) — the
  VulkanInstanceService fails, `PlatformManagerService secondary service terminated gracefully!` follows, the
  process exits in 0.29 s with code 1, CEF's subprocess ends cleanly.

## What remains
- **Fault injection** (Ave Robustus II § 1.1, phase P3): a test build that forces EACH `initialize*()` step to fail in
  turn (window, input, renderer, physics, audio, notification, overlay) — each exits cleanly, LSan 0 leak, no hang,
  0 VUID. Only the Vulkan-instance step is proven so far, plus one failure below it: a Vulkan DEVICE whose creation
  fails (the physics-acceleration device) is released and the process exits 0 — proved by fault injection in
  `a3d82895` (the `Device` ↔ `Queue` cycle that kept it alive is broken).

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.1 H1, P0 + P3.
