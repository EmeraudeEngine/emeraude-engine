---
id: lod-job-outlives-its-mesh
title: An automatic LOD job runs after its mesh resource is destroyed — segfault at shutdown
status: open
priority: high
scope: Graphics / Renderable (MeshResource, MultiLayerMeshResource), Base::ThreadPool
opened: 2026-10-07
tags: [crash, threading, lod, shutdown, lifetime]
---

# An automatic LOD job runs after its mesh resource is destroyed — segfault at shutdown

## Why

Found 2026-10-07 (Linux) while validating the lock-free `geometry()`: `sponza` with
`Core/Graphics/LOD/EnableAutomaticGeneration = true`, `Core.shutdown()` after 45 s → **SIGSEGV, exit 139, 3 runs out
of 3** — the build WITHOUT that change crashed the same way (2/2), so it predates it. gdb, the crashing thread:

```
#0 __memcpy_avx_unaligned_erms
#1 std::basic_streambuf<char>::xsputn
#2 std::__ostream_insert<char>
#3 EmEn::Graphics::Renderable::MeshResource::generateLODLevel(...)
#4 EmEn::Base::ThreadPool::Task::invokeSmall<MeshResource::onDependenciesLoaded()::{lambda()#1}>
#5 EmEn::Base::ThreadPool::worker()
```

`MeshResource::onDependenciesLoaded()` enqueues `[this, sourceGeometry, sourceLease, ...]` on the primary thread
pool; at shutdown the resource containers unload ("MeshResource 123 resource(s) unloaded") while a worker still runs
the job, which then reads `this` (its `name()` for a trace) — a use after free. `MultiLayerMeshResource` enqueues the
same way. `citadel` (94 levels published) exited cleanly: the window is the time left in a queued or running job.

## What remains (an owner decision first — the lifetime model)

- [ ] Choose: (a) the job keeps its resource alive — capture a `weak_ptr` (or `shared_from_this()`), `lock()` at the
      start, give up when expired; ⚠️ a job still holding it past the renderer's teardown would upload a level on a
      dead device. (b) The resource cancels and waits for its pending jobs in its destructor. (c) The shutdown drains
      the thread pool before the resource containers unload. Recommendation: (a) + (c).
- [ ] A reproduction that does not need the GPU timing: a test seam delaying the job, or a scene unloaded right after
      the jobs are queued.
- [ ] Fix both meshes; prove 0 crash over N shutdowns of `sponza` with automatic LODs.

## References

- `src/Graphics/Renderable/MeshResource.cpp` (`onDependenciesLoaded()`, `generateLODLevel()`), the same in
  `MultiLayerMeshResource.cpp`; `Base::ThreadPool`.
