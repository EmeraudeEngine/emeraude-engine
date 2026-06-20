# Multi-Scene Resource Ownership

> [!CRITICAL]
> **This is a code-generation doctrine.** Most rendering features in this engine were first built
> and tested with a *single* scene. Loading several scenes, switching between them, and deleting
> them at runtime exposes a whole class of resource-lifetime bugs (five were found and fixed in
> Jun 2026 — see [`caution-points.md`](caution-points.md)). Every one shared the same root: a
> **shared or global resource whose ownership/lifecycle was wired per-scene** (or via a global
> static), **or a renderer-global service that kept operating after its active scene was gone**.
> Read this before writing any code that creates, references, or destroys a GPU resource.

## The runtime model you must assume

- `Scenes::Manager` can hold **several scenes loaded at once**; **exactly one is ACTIVE**. Scenes
  are **enabled, switched, and deleted at runtime**. Your code must stay correct across
  `enable → render(N frames) → disable → delete`, **while other scenes still exist**.
- The rendering thread and the logic thread run concurrently under a **shared** scene lock
  (`Scenes::Manager::m_activeSceneSharedAccess`); scene enable/disable take the **exclusive**
  lock. GPU work for a scene may still be **in flight** after its frame's lock is released.

## Ownership tiers — the decision rule

For any resource, ask: **"is it shared across scenes / does it outlive a single scene?"**

1. **Engine-global — owned ONCE by the Renderer/device, for the renderer's whole lifetime.**
   Samplers (`Renderer::getSampler` cache), pipeline / program / descriptor-set-layout caches,
   the main `DescriptorPool`, `BindlessTextureManager` (the descriptor table),
   `AccelerationStructureBuilder`, `PostProcessor` (the composite/blit pass), default/dummy
   textures, `GrabPass`.
   - Create once at renderer init; reach it **through the Renderer**.
   - **NEVER** own it per-scene. **NEVER** publish it through a global `static`. Consumers
     **release their reference**, they do **not** `destroyFromHardware()` it — the owner destroys
     it once, at shutdown.
   - A global service that *executes work* (not just holds resources) must **only run for the
     ACTIVE scene**. If there is no active scene, it must **not** record its pass — its inputs
     (descriptors, sampled images) belong to a scene that may be gone. See anti-pattern #5.

2. **Shared resources — `Resources::Manager`, reference-counted.**
   Geometries, textures, materials, skeletons… outlive any single scene. Their GPU sub-objects may
   be cache-shared (e.g. a texture's sampler). Drop the reference; the manager unloads at
   refcount 0. Never destroy a cache-owned sub-object from a resource's teardown.

3. **Per-scene state — DESCRIBES, never owns shared GPU objects.**
   `LightSet`, `BindlessTextureSet`, `SceneMetaData`. The scene describes what it uses; a global
   service mirrors/reads the **active** scene. Genuinely per-scene GPU objects are fine (e.g. the
   scene's TLAS), but anything shared is **borrowed** (e.g. `SceneMetaData` borrows the
   renderer-owned `AccelerationStructureBuilder`).

4. **Per-render-target GPU resources — tied to the render target's create/destroy.**
   View matrices (UBO + descriptor set) live in `RenderTarget::Abstract::createRenderTarget` /
   `destroyRenderTarget`, **not** in camera connect/disconnect (a transient AVConsole event).

## Lifecycle hooks — what to do, where

- **Per frame / on activate:** the global service mirrors the active scene's description
  (e.g. `BindlessTextureManager::syncTextureSet(scene.bindlessTextureSet(), …)`, driven by the
  Renderer right after `Scene::prepareRender`).
- **On disable** (`Scenes::Manager::disableActiveScene`, under the exclusive lock): release this
  scene's references held by global services (e.g. `BindlessTextureManager::clearTextureSet`), and
  `device->waitIdle()` (or use a deferred-retire queue) before any GPU object the descriptor set /
  an in-flight frame may reference is destroyed. **Never destroy a shared/global object here.**
- **On delete:** per-scene state dies with the scene; global services keep running and must hold
  **zero** references to the deleted scene.

> The `PostProcessStack` itself is **per-scene state (tier 3)**: the demo builds it and moves it
> into the `Scene`. The renderer-global `PostProcessor` *executes* the active scene's stack. So
> the stack is owned by the scene; the executing service is owned by the renderer and must mirror
> the active scene exactly like the bindless table.

## Anti-patterns — the five canonical bugs

1. **Lifecycle tied to a transient event** instead of the owner's lifecycle → view-matrices were
   created/destroyed on camera connect/disconnect (unsynchronized with the render thread).
2. **A global `static` pointer to a per-scene-owned object** → `Geometry::Interface`'s static
   `AccelerationStructureBuilder` was nulled when any scene was deleted.
3. **A per-scene instance of a service that serves shared objects** → the bindless table and the
   AS builder were per-scene yet serve shared geometries/textures.
4. **A consumer destroying a cache-owned shared object** → `Texture2D` cleanup called
   `m_sampler->destroyFromHardware()` on the shared cached sampler.
5. **A renderer-global service running its pass with no active scene** → the `PostProcessor` is
   enabled by the demo (`postProcessor().enable(true)`) and stays enabled after the scene is
   deleted. Both render paths then ran the final composite: in `Renderer::renderFrameDirect` the
   post-process block was gated only on `m_postProcessor.isEnabled()`, so with no scene it blit +
   composited against a **destroyed/null** primary-sampler image (the deleted scene's effect
   output) with an incompatible render-pass layout → device lost. Fix: gate the composite on an
   **active scene** (`scenePtr != nullptr`), and take the internal-target PP path in `renderFrame`
   only when `scene != nullptr`. With no scene, fall through to a plain cleared frame.

## Checklist before generating resource code

- [ ] Shared across scenes / outlives a scene? → own it **once** at the Renderer level; reach it via the renderer.
- [ ] Tempted to add a `static`/singleton for engine state? → **don't**; reach the owner through the renderer.
- [ ] Does a scene reference a shared GPU object? → **describe/borrow**; release on disable; never destroy.
- [ ] About to destroy a GPU object a descriptor set or in-flight frame may reference? → clear the descriptor **and** `waitIdle` (or deferred-retire) first.
- [ ] New per-scene state? → model it on `LightSet` / `BindlessTextureSet` (passive description read by the global service).

## References

- [`caution-points.md`](caution-points.md) — the four Jun 2026 fixes, with symptoms and root causes.
- [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) §6 — Bindless Textures Manager (per-scene set + manager).
- [`render-targets.md`](render-targets.md) — View Matrices Lifecycle.
- [`src/Scenes/AVConsole/AGENTS.md`](../src/Scenes/AVConsole/AGENTS.md) — device connect/disconnect must not manage GPU lifecycle.
