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

## What remains
- The immediate fix: every failure path terminates what was initialized (and the destructor safety net of decision D3).
- **Fault injection** (Ave Robustus II § 1.1): a test build that forces each `initialize*()` step to fail in turn —
  each exits cleanly, LSan 0 leak, no hang, 0 VUID.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.1 H1, P0 + P3.
