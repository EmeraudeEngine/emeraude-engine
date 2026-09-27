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

**Jun 2026 audit (from the Scene Manager outward):** no remaining multi-scene-hazardous global
statics; the cached-shared-resource-destruction class was fully swept (all `TextureResource` types
+ `Overlay::Surface` now release the cache-owned sampler instead of destroying it). Dormant scenes
do not contaminate rendering; suspend/wakeup already releases their pooled (audio) resources.
