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

⚠️ Since 2026-10-15 `ResourceTrait.hpp` includes `ThreadPool.hpp` (the provider's `TaskGroup` member): every resource
header pays for it (compile time, the lowest priority).
