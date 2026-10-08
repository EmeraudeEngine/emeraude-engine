---
id: jobs-owned-by-their-starter
title: Every asynchronous job owned by the object that starts it; remove the ad-hoc cancellation
status: open
priority: high
scope: src/Graphics/Renderable/MeshResource.cpp, MultiLayerMeshResource.cpp, src/Resources/ResourceTrait.hpp, src/Core.cpp, + the class C sites
opened: 2026-10-08
tags: [ave-robustus-ii, concurrency, lifetime]
---

# Every asynchronous job owned by the object that starts it; remove the ad-hoc cancellation

## Why
51 asynchronous sites are safe only because `Core` drains the pool or because of member order (class C). Claude's own
deviations of 2026-10-06 → 08 belong here: `AbstractServiceProvider::cancelBackgroundWork()` (engine `1ecb9696`), the
second `ThreadPool::wait()` in `Core::run()`, `backgroundWorkCancellation()` handed to the decimator as a raw
`const std::atomic_bool *` (`9f03bfb0`), LOD jobs capturing both `weak_from_this()` and raw `this`.
Measured cost: shutdown with automatic LODs takes Linux 2.2 s, macOS 3-4 s, Windows 21-26 s.
Second case, 2026-10-08 (Linux, RTX 3070 Ti): `terrain` shut down 15 s after launch, still loading, took **20.0 s** to
exit — 32 `CloudShapeResource` growth jobs and the CDLOD clipmap upload ran to completion first (the log shows them
finishing after `Core.shutdown()`). `sponza` with automatic LODs: 6.5 s.

## What remains
- After base `task-handle-and-stop-token`: `MeshResource` / `MultiLayerMeshResource` hold their LOD `TaskHandle`s; then
  `FrameCapture`, `Net::Manager`, `APIClient`, the `Resources::Manager` release pass, `Container` loading tasks.
- **Remove** `cancelBackgroundWork()`, `isBackgroundWorkCancelled()`, `backgroundWorkCancellation()`, the second drain.
- Proof: sponza + automatic LODs, shutdown within bound D7 on the three OS, 0 crash, TSan clean.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.2, § 3.4, P2.
