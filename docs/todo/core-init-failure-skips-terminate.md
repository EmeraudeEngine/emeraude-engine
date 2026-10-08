---
id: core-init-failure-skips-terminate
title: Core exits without terminate() when initializeCoreLevel() fails
status: open
priority: high
scope: src/Core.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, raii, defect]
---

# Core exits without terminate() when initializeCoreLevel() fails

## Why
`Core.cpp:455-458`: `if ( !this->initializeCoreLevel() ) { return EXIT_FAILURE; }` — no `terminate()`, while the veto
path calls `return this->terminate();`. The secondary services already started are never terminated (`~Core` handles
the primary ones only): VkInstance, OpenAL, GLFW leak, and the audio threads may hang at exit (see
`audio-thread-member-order`).

## Done (2026-10-08, P0)
- `Core::run()` calls `terminate()` when `initializeCoreLevel()` fails; `~Core()` terminates whatever user / secondary
  service is still registered (safety net, decision D3-a); `Vulkan::Instance`, `Audio::TrackMixer` and
  `Audio::Recorder` release in their destructors when `onTerminate()` did not run.
- Proof of one real failure: no Vulkan driver (`VK_ICD_FILENAMES=/nonexistent.json VK_DRIVER_FILES=…`) — the
  VulkanInstanceService fails, `PlatformManagerService secondary service terminated gracefully!` follows, the
  process exits in 0.29 s with code 1, CEF's subprocess ends cleanly.

## What remains
- **Fault injection** (Ave Robustus II § 1.1, phase P3): a test build that forces EACH `initialize*()` step to fail in
  turn (window, input, renderer, physics, audio, notification, overlay) — each exits cleanly, LSan 0 leak, no hang,
  0 VUID. Only the Vulkan-instance step is proven so far.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.1 H1, P0 + P3.
