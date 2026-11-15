## Multi-Scene Lifecycle (Active / Dormant / Deleted)

> [!CRITICAL]
> Several scenes can be loaded at once; **exactly one is ACTIVE**. Read
> [`docs/multi-scene-resource-ownership.md`](../../multi-scene-resource-ownership.md) — it is
> the code-generation doctrine for any code touching GPU resources. The four Jun 2026 fixes
> (view-matrices, bindless, sampler cache, AS builder) all came from wiring a shared/global
> resource per-scene.

**Three states (the application designer decides transitions — the engine only offers them):**

- **Active** — the **only** scene that is rendered (`Renderer::renderFrame`) AND ticked
  (`processLogics`). The global services (bindless table, env cubemap, AS builder) mirror it.
- **Loaded-Dormant** — loaded but not active: **not rendered, not ticked**. It therefore does
  **NOT** contaminate the active scene's rendering (a common misconception — the renderer only
  reads the *active* scene's LightSet / post-process / render targets). Its pooled resources
  (audio sources) are released via Suspend/Wakeup. It still holds its per-scene GPU resources, so
  keep the dormant set small (memory; the RTX 3070 Ti 8 GB constraint is deliberate). Re-activating
  re-syncs the bindless table in ~1 frame.
- **Deleted** — destroyed; per-scene state dies with it, global services keep running referencing
  nothing of it.

> [!IMPORTANT]
> **`BindlessTextureSet` slot capacities are device-dependent.** `Scene`'s constructor pushes the GPU
> table capacities into its set (`setCapacities(maxTextures2D, maxTexturesCube, maxTexturesCubeArray)`,
> read from `Renderer::bindlessTextureManager()`). They are resolved at renderer initialization from
> the device's update-after-bind budget and are **lower than the `DesiredMaxTextures*` constants on
> MoltenVK** (2D[768]). Never bound a slot with the desired constants: a set handing out a slot beyond
> the table would have its descriptor write rejected by the manager and the texture would simply never
> appear. See [`Graphics/AGENTS.md`](../../../src/Graphics/AGENTS.md) → "Table Capacities Are Device-Dependent".

**Disable contract** (`Manager::disableActiveScene`, under the exclusive `m_activeSceneSharedAccess` lock):
editor deactivate → `BindlessTextureManager::clearTextureSet` (overwrites this scene's dynamic
slots with dummies — **hitch-free, NO waitIdle**) → `Scene::disable` (suspend entities, node
controller). The scene stays loaded (dormant) unless also deleted.

**Delete contract** (`Manager::deleteScene`): if active, `disableActiveScene` first; then
**`device->waitIdle()` (the drain lives HERE, before `m_scenes.erase`)** so in-flight frames that
referenced the scene's textures/buffers complete before destruction. The drain is on delete, not
disable, precisely so scene **switching stays seamless**.

**Writer preference** (fixed 2026-09-24, owner decision): every exclusive section on
`m_activeSceneSharedAccess` — `disableActiveScene`, `enableScene`, `deleteScene`,
`withExclusiveActiveScene` — first builds an `ExclusiveAccessAnnouncement`, BEFORE its
`std::unique_lock`, destroyed AFTER it; `withSharedActiveScene()` and `hasActiveScene()` wait
(`waitForAnnouncedExclusiveAccesses()`, a condition variable, lock-free when nothing is announced)
while one is. Measured on Windows: forest shutdown 121 s → 1.5-1.8 s (NVIDIA and AMD, validation ON).
⚠️⚠️ Without it the Windows shutdown hung 24 s to over 3 min, validation ON or OFF: MSVC's `std::shared_mutex`
is an SRWLOCK (neither fair nor FIFO), the render loop holds the shared access for a whole frame and
takes it back microseconds later, and it stole the lock from the woken writer frame after frame
(diagnosed by the Windows session with symbols, `docs/caution-points.md` § Platform-Specific).
⚠️ A re-entrant shared acquisition on the same thread is now a deadlock on EVERY OS as soon as a
writer is announced (it was already undefined behaviour): code running inside the frame reads the
scene through `Core::m_frameScene`, never through the manager again.

**Changing application state between two logic cycles** (`Manager::betweenLogicCycles()`, owner decision 2026-11-15):
the application's logic hook (`Core::onCoreProcessLogics()`) runs on the logic thread inside the cycle lock
(`m_logicCycleAccess`, held by `withLogicCycle()` for one whole cycle) and OUTSIDE the scene's lock, so nothing ordered
it against another thread changing what it reads: a downstream testbed swapped its active game object (a shared
pointer) and switched its player's input flags from the main thread while the hook read them — five ThreadSanitizer
reports — and could destroy that object while the hook was still running on it (`disableActiveScene()` waits for the
scene's accesses, never for the hook). `betweenLogicCycles(change)` takes the cycle lock: the change lands between two
cycles, and once it returned the hook no longer runs on what the change removed. The rules:
- any thread but the logic thread; the caller waits for the cycle in flight (one cycle at most; nothing while the logic
  thread is paused or stopped); shared accesses are allowed inside;
- ⚠️ never from inside an access of the active scene, shared or exclusive — a rendering frame holds the shared one for
  its whole duration: `enableScene()` / `disableActiveScene()` / `deleteScene()` announce their write lock WITHOUT the
  cycle lock, so the cycle in flight may be waiting for that announced writer, which waits for the frame's shared
  access, which would wait for the cycle. Three threads, no way out;
- not re-entrant, and an exclusive access (`withExclusiveActiveScene()`) asked from inside the change is the same
  contract fault as one asked from inside the cycle: traced, aborted (`m_logicCycleOwner` names the holder of the lock,
  the cycle or the change);
- keep `enableScene()` / `disableActiveScene()` OUTSIDE the change: their notification handlers would run under the
  cycle lock.
What it does NOT order: the rendering thread, which never takes the cycle lock — a state the renderer reads keeps its
own protocol (a render state slot, an atomic; item `tsan-logic-render-shared-state-races`).
Proof (the testbed's `default`, Linux, Debug + ThreadSanitizer with `detect_deadlocks=1`, llvmpipe, the active object
unloaded / reloaded / destroyed / rebuilt through the console, 2026-11-15): before, the five races in one run, the logic
thread holding the cycle lock in each; after, 0 report naming the swap in 3 runs, 0 lock-order inversion, a clean
shutdown each time. Release with the validation layer (RTX 3070 Ti): 25 input switches in a row from the main thread
while the scene runs, 0 hang, 0 VUID, exit 0.
Peers' round 8 (2026-11-15, engine 13d88cb5 + testbed e5f46b58): macOS (Apple M6, MoltenVK 1.4.2, Apple clang with `-Wdocumentation -Werror`) and Windows (RTX 3500 Ada, MSVC 19.44) build Release and Debug with 0 warning and run the testbed's two scenarios as Linux does: three rounds of unloading (kept, then destroyed) and
reloading the active object — 7 control switches each way, no "contract violation", the slowest command the rebuild of
a destroyed act (0.10 s Linux, 0.14 s Windows, 0.75 s macOS) — and 25 input switches through the web interface (26 each
way, the slowest command 0.03 s / 0.011 s / 0.012 s); 0 VUID, 0 `[Error]`, exit 0.

**Jun 2026 audit (from the Scene Manager outward):** no remaining multi-scene-hazardous global
statics; the cached-shared-resource-destruction class was fully swept (all `TextureResource` types
+ `Overlay::Surface` now release the cache-owned sampler instead of destroying it). Dormant scenes
do not contaminate rendering; suspend/wakeup already releases their pooled (audio) resources.
