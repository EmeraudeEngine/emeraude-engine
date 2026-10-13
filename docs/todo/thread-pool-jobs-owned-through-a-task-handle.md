---
id: thread-pool-jobs-owned-through-a-task-handle
title: Thread-pool jobs owned through a TaskHandle by the object that starts them (LOD jobs first); remove the ad-hoc cancellation
status: open
priority: high
scope: src/Graphics/Renderable/MeshResource.cpp, MultiLayerMeshResource.cpp, src/Resources/ResourceTrait.hpp, src/Core.cpp, src/PrimaryServices.cpp, src/Graphics/FrameCapture.cpp, src/Net/Manager.cpp, src/Net/APIClient.cpp, src/Resources/Manager.cpp, src/Resources/Container.hpp
opened: 2026-10-13
tags: [ave-robustus-ii, concurrency, lifetime, thread-pool]
---

# Thread-pool jobs owned through a TaskHandle by the object that starts them (LOD jobs first); remove the ad-hoc cancellation

## Why

Phase P2 of Ave Robustus II, its thread-pool half. The owner ordered it started NOW (2026-10-13): everything it needs
is already in emeraude-base — `Base::TaskHandle` + `ThreadPool::submit()` (`src/ThreadPool.hpp:68`, `:843`; base
`73a319a`; a task waiting for its own handle aborts, D2 `26128bc`) and `ShapeDecimator::setStopToken()`
(`src/VertexFactory/ShapeDecimator.hpp:110`; stop latency 16 ms worst on Linux, 2.5 ms on the M2, since base `fd9dc5c`). Split out of
`jobs-owned-by-their-starter` (which keeps the `Base::Thread` owners, still blocked on base).

Today the LOD jobs are protected by Claude's own deviations of 2026-10-06 → 08 (plan § 3.4), not by their owner:
- `MeshResource.cpp:447` / `MultiLayerMeshResource.cpp:733` enqueue the job with `[this, weakSelf = weak_from_this(), …]`:
  the job LOCKS the mesh alive for its whole run instead of the mesh owning the job;
- they poll `serviceProvider().isBackgroundWorkCancelled()` between levels (`MeshResource.cpp:460`,
  `MultiLayerMeshResource.cpp:746`) and hand the decimator the provider's flag as a raw `const std::atomic_bool *`
  (`setCancellationFlag()`, `MeshResource.cpp:484`, `MultiLayerMeshResource.cpp:770`);
- `Core::run()` raises `m_resourceManager.cancelBackgroundWork()` (`Core.cpp:666-668`; the flag and its accessors in
  `ResourceTrait.hpp:282-305`, member `:383`) and drains the pool TWICE (`Core.cpp:670-676` and `:691-698`, the second
  one added for these jobs on 2026-10-07: graphics doc `20-15b-level-of-detail.md`), plus `PrimaryServices::Impl::terminate()`
  (`PrimaryServices.cpp:359`).

Measured cost of the shutdown with automatic LODs: Linux 2.2 s, macOS 3-4 s, Windows 21-26 s (2026-10-08, before D7);
`sponza` 6.5 s (2026-10-08, Linux). Second case, 2026-10-08 (Linux, RTX 3070 Ti): `terrain` shut down 15 s after
launch, still loading, took **20.0 s** to exit — 32 `CloudShapeResource` growth jobs and the CDLOD clipmap upload ran
to completion first (the log shows them finishing after `Core.shutdown()`): resource loads on the pool, which no
owner can stop.

The other thread-pool sites of P2 are protected the same way — by `Core` / `PrimaryServices` draining the pool, or a
hand-made wait — and capture a raw `this` (verified 2026-10-13):
- `Graphics/FrameCapture.cpp:432` — `enqueue([this] { writeFiles(); })`; `~FrameCapture() = default` (`:101`).
- `Net/Manager.cpp:698` — `enqueue([this, ticket] { performDownload(ticket); })`; `onTerminate()` relies on
  `PrimaryServices` having drained the pool (comment `:193-201`).
- `Net/APIClient.cpp:378` — `enqueue([this, ticket] { performRequest(ticket); })`; same reliance (`:182-190`).
- `Resources/Manager.cpp:251` — the local-data release pass, `enqueue([this, now] …)`; `onTerminate()` spin-waits on
  `m_localDataReleasePassRunning` (`:816`), the destructor does not wait.
- `Resources/Container.hpp:1566` and `:1847` — the loading tasks, `enqueueTask(…, [this, request] { loadingTask(request); })`
  through `ServiceAccess::enqueueTask()` (`Resources/Container.cpp:69`).
Not in scope (already self-contained or value-only): `PeerStore` / `SharingServer` (their destructors wait for their
jobs), `ResourceTrait.cpp:895` and `CDLODTerrainResource.cpp:1658` (`[self]`), `Container.hpp:1416` (values).

## What remains

1. **LOD jobs first**: `MeshResource` / `MultiLayerMeshResource` keep their LOD `TaskHandle`(s) from
   `ThreadPool::submit()` as members; the job receives the `std::stop_token`, checks it between levels and hands it to
   `ShapeDecimator::setStopToken()`. The mesh's destructor (implicit, the handle's) stops and waits for its job.
   Remove the `isBackgroundWorkCancelled()` polls and both `setCancellationFlag()` calls.
2. **The other sites**, each with its own handle(s): `FrameCapture`, `Net::Manager`, `APIClient`, the
   `Resources::Manager` release pass (its spin-wait goes), the `Container` loading tasks.
3. **Remove** `cancelBackgroundWork()`, `isBackgroundWorkCancelled()`, `backgroundWorkCancellation()` and
   `m_backgroundWorkCancelled` (`ResourceTrait.hpp`), their call in `Core.cpp`, and the second drain once no
   thread-pool job depends on it; then base `ShapeDecimator::setCancellationFlag()` (TRANSITIONAL, `ShapeDecimator.hpp:122`)
   and its test — base item `task-handle-and-stop-token` points here for it.
4. **Proof**: `sponza` + automatic LODs and `terrain` shut down during the load, within bound D7 on the three OS, 0 crash,
   TSan clean; the base tests green.

## ⚠️ Traps

- **The job must not keep its owner alive.** Today the job holds a strong `shared_ptr` to the mesh (`weakSelf.lock()`)
  for its whole run. With the mesh owning the handle, a job that drops the LAST reference runs the mesh's destructor
  ON THE POOL THREAD, which then waits for its own task: a contract fault, `std::abort()` in every build (base D2,
  death test `ThreadPoolTaskHandleDeathTest.TaskWaitingForItsOwnHandleAborts`). Capture values and the data it needs
  (`sourceGeometry`, `sourceLease`), and `this` only because the owner's handle guarantees its life.
- **A destructor now waits** for its job's stop latency, on whichever thread releases the owner (logic, render, main):
  16 ms worst on Linux, 2.5 ms on the M2 since base `fd9dc5c`; to be re-measured on the new Windows machine (base item).
- The LOD publication into the mesh (the job writes the generated levels) must stay synchronized with the render
  thread reading them; moving the ownership does not change that contract.
- `enqueue()` returns `false` when the pool is stopping; `submit()`'s refusal must be handled the same way (the work
  done inline or the resource failed), never ignored.

## References

- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.2 (class C), § 3.4 (the deviations), § 4.1 (the target form), § 5 P2.
- emeraude-base `docs/caution-points.md` § the decimator's stop (stop latency, `setCancellationFlag()` transitional).
- Items: `jobs-owned-by-their-starter` (the `Base::Thread` half, blocked), base `task-handle-and-stop-token`.
