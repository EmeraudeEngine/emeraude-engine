## Async Lambda Capture Safety (CRITICAL)

> [!CRITICAL]
> Lambdas passed to `getOrCreateResource()` execute on the **thread pool**, potentially after
> the calling object is destroyed. All captured data must be **self-contained**.

### Rules

1. **NEVER capture `this`** or local references to stack-allocated objects
2. **Pre-resolve** all `shared_ptr` dependencies before the lambda
3. **Copy scalars** (float, int, Color) by value
4. **Move-capture** containers of `shared_ptr` to avoid atomic refcount overhead

### Pattern: Pre-Resolve + Value Capture

```cpp
// Pre-resolve outside the lambda (on the calling thread)
auto texture = (texIdx < m_textures.size()) ? m_textures[texIdx] : nullptr;
float roughness = pbr.roughnessFactor;
Color< float > color{pbr.baseColorFactor[0], pbr.baseColorFactor[1], ...};

// Lambda captures only values and shared_ptrs — fully self-contained
auto material = container->getOrCreateResource(name, [
    texture = std::move(texture), roughness, color
] (auto & res) {
    if ( texture != nullptr ) res.setAlbedoComponent(texture);
    else res.setAlbedoComponent(color);
    res.setRoughnessComponent(roughness);
    return res.setManualLoadSuccess(true);
});
```

### Why This Matters

`getOrCreateResource()` (Container.hpp:1208) enqueues the lambda to the thread pool and returns immediately. If the caller is stack-allocated (e.g., `GLTFLoader loader{...}`), `this` becomes dangling before the lambda executes → **use-after-free crash**.

**Code references:**
- `Container.hpp:getOrCreateResource()` — Async path (thread pool)
- `Container.hpp:getOrCreateResourceSync()` — Sync path (blocks calling thread)
- `Scenes/GLTFLoader.cpp` — Reference implementation of safe async captures

### Every pool job of the resources has an OWNER (Ave Robustus II, 2026-10-15)

No resource job is fire-and-forget any more, and `Core` neither cancels nor drains the pool for them (the global
`cancelBackgroundWork()` flag and the two `Core::run()` drains are gone). Who owns what:

| Job | Owner (member, declared LAST) | Captures |
|---|---|---|
| a container's loading task, a manual creation (`getOrCreateResource()`) | the `Container` (`Base::TaskGroup m_tasks`) | `this` + the request / the resource |
| a local-data reload (`ResourceTrait::requestLocalData()`), the CDLOD ray-tracing proxy | the provider (`AbstractServiceProvider::submitResourceJob()`, its `TaskGroup`) | the resource (`self`): the job calls the derived class, which a base-class handle could not keep alive |
| an automatic LOD job | the mesh (`Base::TaskHandle m_LODJob`) | `this` only — never the mesh itself |
| the CPU-copy release pass | the `Resources::Manager` (`Base::TaskHandle`) | `this` |

**The shutdown order.** The `Resources::Manager` is a primary service: it is terminated AFTER the renderer and the
Vulkan device. So `Core::terminate()` first calls `Resources::Manager::stopBackgroundWork()` — the manager stops its
OWN jobs (release pass, every container, the provider's jobs) and waits for them, once the logic and rendering threads
are joined, before any service they touch goes. Idempotent; `Manager::onTerminate()` calls it again.
**Request every stop, then wait (owner decision 2026-11-01).** The manager first calls every container's
`requestBackgroundWorkStop()` (new loads refused, queued ones discarded, running ones see their token), THEN waits for
each (`stopBackgroundWork()`), then the provider's jobs. It used to stop and wait one container at a time, in the
`unordered_map` order (implementation-defined): on Windows the 'Image manager' came first and waited 13.6 s for tasks
queued behind the ocean's frames, which run under the 'Movie manager''s token — not yet requested; the cold
`water-world` shutdown took 18.4 s there (3.75 s on macOS, 4.7 s on AMD, whose order put the movies first). TSan stress
`EmeraudeResourceJobsStress` (projet-alpha `docs/unit-testing.md`, loads + LODs + reloads in flight at the stop), Linux,
lavapipe, 3 runs: 0 report, 0 assertion, exit 0, every resource job stopped in 5-86 ms.

**A job that never runs.** A job stopped before it starts never runs (base `TaskGroup` / `TaskHandle` contract,
2026-10-15). A loading task or a manual creation discarded that way puts its resource in the Failed state through
`ServiceAccess::DiscardedTaskGuard` (so its parents stop waiting, owner ruling 2026-10-01); a discarded reload ends the
reload through `ResourceTrait::LocalDataReloadJob` (else `acquireLocalData()` would wait forever). The stop removes
the queued tasks from the pool's queue and these guards run on the STOPPING thread (a task already dequeued: on its
worker); they touch ONLY what they hold — never the container. `stopBackgroundWork()` uses `TaskGroup::waitForAll()`:
when it returns, every discarded task's guard has run, and it never waits behind other owners' queued work.
`stopBackgroundWork()` traces every group that took more than 10 ms to stop, and its total. A load the stop cancelled
ends Failed through `ResourceTrait::abandonLoading()` — WITHOUT an error line — and each container traces ONE line, "N
resource load(s) cancelled by the stop." (owner decision 2026-10-15: macOS measured 374-389 `[Error]` lines for a quit
during the sponza load before it; real load failures stay errors). Its parents stay silent too
(`ResourceTrait::loadAbandoned()`): one going on without it no longer warns "goes on without its failed dependency"
(macOS measured 24-28 such warnings per quit after the summary line), one failing for it no longer logs "fails: its
dependency … failed" and is marked abandoned in turn, so the whole chain above a cancelled load is quiet. A parent
still goes on or fails exactly as before; only the lines are gone.
⚠️ Linux does not reproduce it (2026-10-15): sponza is loaded before `Core.shutdown()` is served (~3.6 s), and the act
removal waits for every pending load of the act first (a test-only build with a 400 ms sleep per image file: "Removing
the act 'sponza'" took 84 s, then nothing was left to cancel). The cancelled loads come from a slower machine (macOS M6).
Verified on macOS (M6, 2026-10-15, sponza quit at 2 s, still loading, ~390 loads cancelled per run, 3 runs): 0 "goes on
without", 0 "fails: its dependency" (were 24-28 and 0-1), 0 `[Error]` and the same single AL warning as a normal quit
after the command, quit in 0.55-0.91 s, 0 VUID, exit 0.

**Measured (engine d38bf755 and after, 2026-10-15; former item thread-pool-jobs-owned-through-a-task-handle).**
- Before (2026-10-08), `Core` cancelled through a flag and drained the pool: the shutdown with automatic LODs took 2.2 s
  on Linux, 3-4 s on macOS, 21-26 s on Windows; `sponza` 6.5 s; `terrain` quit while loading took 20.0 s, because its
  cloud growth jobs and the clipmap upload ran to completion first.
- After, Linux (RTX 3070 Ti, validation layer):
  - `sponza` with automatic LODs quits in 0.60-0.72 s at 8 / 12 / 16 / 25 s (at 8 s, the jobs in flight stop); 0 VUID.
  - `terrain` quit during its load exits 1.27 s after the command executes. The command itself waits for the main
    thread while the act loads: item `act-load-blocks-main-thread`.
- TSan stress `EmeraudeResourceJobsStress` (projet-alpha `docs/unit-testing.md` § ThreadSanitizer): 0 report.
- Peers: Windows, base suites green (2531 tests); macOS, `sponza` quit at 2 s in 0.48-0.71 s.

### The stop reaches the completion chain (owner decision 2026-11-01, the explicit token in its narrow form)

A load task's stop token used to be dropped by the container (`[...] (const std::stop_token &) { loadingTask(request); }`),
and the work a completion starts runs at the END of the dependency chain, on the thread of the dependency that completed
last: a movie loads → its animated texture's `onDependenciesLoaded()` → `createTexture()` compresses 124 frames. Nothing
there could see the stop: `water-world` on a cold texture cache made a shutdown wait for the whole compression (item
`movie-load-ignores-the-stop`: 46 s on macOS; 106 s on Linux limited to 4 cores).

- **`ResourceTrait::LoadStopScope`** (RAII): the container's load task and its manual-creation task hold one around the
  load; it hands the task's token to the resource (`m_loadStopToken`, under `m_dependenciesAccess`) and takes it back at
  the end. A completion after the load (another thread) or a load outside a container task gets a token that never
  stops.
- From there the token is an EXPLICIT parameter: `setLoadSuccess()` → `checkDependencies(token)` → each parent's
  `dependencyLoaded(dependency, token)` (and `dependencyFailed()` / `releaseLinksAfterFailure()` /
  `goOnAfterDependencyFailure()` on the failure side) → `onDependenciesLoaded(const std::stop_token &)` (every resource
  class) → `Material::Interface::create(renderer, token)` → `Component::Interface::create(renderer, binding, token)` →
  `TextureResource::Abstract::createTexture(renderer, token)`. The 73 `load()` overloads and the 395 `setLoadSuccess()`
  calls are unchanged (owner: the narrow form, not the literal one).
- **A completion the stop cuts short is ABANDONED**: `onDependenciesLoaded()` returns false with the stop requested →
  `m_loadAbandoned`, no error line (here, in the texture and material creations, nor in the parents) — the 2026-10-15
  rule of `abandonLoading()`.
- Who checks it: `AnimatedTexture2D` before each frame (its frames are now encoded IN PARALLEL on the pool,
  `ThreadPool::parallelFor()`, the calling worker taking its share — no deadlock from a pool worker; a stop waits for one
  frame's encoding at most) and before each frame's uncompressed chain; `Texture2D` before its one encoding; and
  `TextureCompressor::compress()` itself before each ROW of blocks (owner, 2026-11-01 — one 2048² BC5 held the Windows
  shutdown 7.2 s): a stopped compression returns NOTHING, silently, never a partial chain the cache would store
  (`TextureCache::getOrCompress()` stores nothing), and `Texture2D` does not fall back to its uncompressed upload.
  Linux, `water-world` cold, 4 cores, shutdown 2 s after load: exit 1.5-1.9 s (7.4-7.9 s with the frame checks alone; the
  'Image manager' under 10 ms); a cold load without a stop still encodes all 122 frames, 0 error.
- The token is requested at `Resources::Manager::stopBackgroundWork()`, after the logic and render threads are joined
  (~2.9 s of encodings on macOS before it), not at the shutdown request: owner, 2026-11-01, keep that order (an act's
  loads stop earlier through the item `act-removal-stops-its-pending-loads`).
- Measured (Linux, RTX 3070 Ti, `water-world --demo-options 0`, texture cache OFF in a settings copy, the process limited
  to 4 cores with `taskset` so the compression outlasts the load, `Core.shutdown()` 2 s after the act is loaded,
  validation layer): before (projet-alpha 1a13702f, engine 6b14c46a) shutdown → exit 111.4 s, "the 'Movie manager' tasks
  took 106305 ms to stop", 122 BC5 encodes; after 7.6 s, 2799 ms, 2 encodes; 0 VUID, 0 `[Error]`, exit 0 both. On all
  32 threads, `city`'s 124 frames are encoded before its scene finishes loading (sea material at 9.2 s, scene 10.3 s):
  the flat sea of a cold cache is gone there. ⚠️ The parallel encoding takes the whole pool: other loads queue behind it
  for those seconds (on 2 cores `city`'s whole load took ~95 s, the sea material still before the scene).

⚠️ Since 2026-10-15 `ResourceTrait.hpp` includes `ThreadPool.hpp` (the provider's `TaskGroup` member): every resource
header pays for it (compile time, the lowest priority).
