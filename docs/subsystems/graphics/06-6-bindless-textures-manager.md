## 6. Bindless Textures Manager

### Overview

The `BindlessTexturesManager` provides a global descriptor set with arrays of textures that can be indexed dynamically in shaders using non-uniform indexing. This eliminates the need to rebind descriptor sets for each material.

> [!CRITICAL]
> **Per-scene ownership — the manager only ever reflects the ACTIVE scene.**
> The bindless table has no scene concept of its own. Each scene owns a
> `Scenes::BindlessTextureSet` (the bindless analogue of `LightSet`) that **describes** the
> textures it uses (RT material textures, light color projections, environment cubemap) and
> **allocates its own dynamic slots** from `FirstDynamicSlot`. Scene code (`SceneMetaData`,
> light emitters, `Scene::enable`) registers into that per-scene set, **never into the manager
> directly**. The manager READS the active scene's set each frame via
> `BindlessTextureManager::syncTextureSet(set, sceneTimeMS)` (driven by the `Renderer` right
> after `Scene::prepareRender`) and writes the descriptor table from it — this also performs
> the per-frame animated-texture frame-view swap and the environment-cubemap write (falling
> back to the engine default cubemap when the scene has none).
>
> Because only one scene is active at a time and the table is mirrored from the active set,
> two scenes may legitimately reuse the same slot indices — **table capacity is the largest
> scene, not the sum of all scenes**.
>
> **On scene disable**, `Scenes::Manager::disableActiveScene()` calls
> `BindlessTextureManager::clearTextureSet(scene.bindlessTextureSet())` (under the exclusive
> lock): a `device->waitIdle()` then each of that scene's freed dynamic slots is overwritten
> with an engine dummy (2D → dummy color-projection 2D; cube + reserved env slot → default
> cubemap). This is REQUIRED — the global table outlives a single scene, so a scene that stops
> being active must leave no descriptor behind, otherwise a later `deleteScene` destroys
> samplers/images still referenced by the descriptor set (`VUID-vkDestroySampler-sampler-01082`).
> The set's CPU data persists with the scene (re-enable re-syncs it). cube-array slots have no
> dummy yet (rare; see `docs/caution-points.md`).
>
> This replaced an earlier design where scenes registered/unregistered directly in the manager,
> which leaked slots and left dangling descriptors on scene switch.

### Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│ BindlessTexturesManager (single global descriptor set)              │
├─────────────────────────────────────────────────────────────────────┤
│ Binding 0: sampler1D[256]   │ 1D texture array                      │
│ Binding 1: sampler2D[4096]  │ 2D texture array                      │
│ Binding 2: sampler3D[256]   │ 3D texture array                      │
│ Binding 3: samplerCube[256] │ Cubemap texture array                 │
└─────────────────────────────────────────────────────────────────────┘
```

### Table Capacities Are Device-Dependent — Never Hardcode Them

> [!CRITICAL]
> The array sizes are **resolved at runtime** in `BindlessTextureManager::computeCapacities()` from
> the device's update-after-bind budget. The `DesiredMaxTextures*` constants (256/4096/256/256/64)
> are a **target, not the effective capacity**.
>
> **Why:** a `COMBINED_IMAGE_SAMPLER` descriptor is charged to BOTH the sampler and the sampled-image
> update-after-bind limits, per set AND per stage. Desktop drivers advertise millions and get the
> desired capacities. MoltenVK advertises **1024 samplers** (Metal's argument-buffer limit) while
> sampled images stay at 1M, so the desired total (4928) blew the budget by ~5x and every pipeline
> layout including the bindless set was rejected. Without the validation layers it did not error — it
> was silently undefined behaviour.
>
> **Reduced profile** on Apple GPUs: **1D[32] 2D[768] 3D[32] Cube[128] CubeArray[32]**, announced by
> a `TraceWarning` at startup. A headroom is withheld from the budget because the update-after-bind
> VUIDs sum **every** set of a pipeline layout, including non-UAB ones such as the SSR/RTGI inputs.
>
> **Rules:**
> - Bound a slot index with `manager.maxTextures2D()` and friends, **never** with
>   `DesiredMaxTextures2D`.
> - The per-scene `Scenes::BindlessTextureSet` receives these capacities from `Scene`'s constructor
>   via `setCapacities()` — a set handing out a slot beyond the table size would have its descriptor
>   write rejected by the manager and the texture would never appear.
> - The generated GLSL declares **unbounded** arrays (`Declaration::Sampler::UnboundedArray`), so
>   capacities never leak into shader code. Keep it that way.
> - Read the startup trace before blaming a missing texture on anything else.

### Reserved Slots

> [!IMPORTANT]
> Each slot is an index into **ONE typed array** — cube slots index `texturesCube[]`
> (binding 3), 2D slots index `textures2D[]` (binding 1). The numbering restarts per array.

| Slot | Array | Constant | Purpose | Written by |
|------|-------|----------|---------|------------|
| 0 | cube | `EnvironmentCubemapSlot` | Scene environment cubemap | `syncTextureSet()` per frame (default cubemap fallback) |
| 1 | cube | `IrradianceCubemapSlot` | IBL diffuse irradiance (32² RGBA16F, stores E/π) | `Scene::updateEnvironmentIBL()` → `BindlessTextureSet` → `syncTextureSet()` (default fallback) |
| 2 | cube | `PrefilteredCubemapSlot` | IBL GGX-prefiltered environment (128² RGBA16F, 6 mips) | same path as slot 1 |
| 3 | 2D | `BRDFLutSlot` | Split-sum BRDF LUT (128² RGBA16F, scale/bias on F0 in RG) | `Renderer::createDefaultResources()` at boot, baked by `Compute::IBLBaker` |
| 4 | 2D | `GrabPassSlot` | Scene color grab pass | Renderer per frame |
| 5 | 2D | `GrabPassDepthSlot` | Scene depth grab pass | Renderer per frame |
| 16+ | all | `FirstDynamicSlot` | Dynamic texture allocation | per-scene `BindlessTextureSet` |
| 0+ | 3D | — | Volumetric cloud SHAPES (`Graphics::CloudShapeResource`), Sep 2026 — the 3D array has NO reserved region, its cursor starts at 0 | `Scenes::Component::CloudVolume` → `BindlessTextureSet::registerTexture3D()` → `syncTextureSet()` |

> ⚠️ The 3D array had no allocator until the clouds (Sep 2026): `updateTexture3D()` existed and
> nobody called it. `BindlessTextureSet::setCapacities()` now takes the 3D capacity too, and
> `clearTextureSet()` does NOT park freed 3D slots on a default (there is no default 3D texture):
> a stale 3D descriptor is harmless under `PARTIALLY_BOUND` as long as no shader indexes it, which a
> cloud that left the set guarantees.

### Usage

**Registering a texture (scene side — into the per-scene set, NOT the manager):**
```cpp
// e.g. from SceneMetaData::rebuild or a light emitter
uint32_t index = scene.bindlessTextureSet().registerTexture2D(texture); // global table index
// Store 'index' in the material SSBO / light UBO for shader access.
```

**Reflecting the active scene into the GPU table (Renderer side, per frame):**
```cpp
bindlessManager.syncTextureSet(scene.bindlessTextureSet(), scene.lifetimeMS());
```

**Updating reserved slots directly (Renderer-owned: grab pass, default env, IBL):**
```cpp
bindlessManager.updateTexture2D(BindlessTextureManager::GrabPassSlot, grabPass);
```

**In GLSL shaders:**
```glsl
layout(set = BINDLESS_SET, binding = 1) uniform sampler2D textures2D[];

// Access with non-uniform index
vec4 color = texture(textures2D[nonuniformEXT(textureIndex)], uv);
```

### Color Projection via Bindless

Light color projection textures are registered into the **scene's `BindlessTextureSet`** during `createOnHardware()` or asynchronously via `ObserverTrait` notification when resource loading completes. Each light UBO carries a `ColorProjectionIndex` field (`uint` encoded as `bit_cast<float>`) that indexes into the bindless 2D or Cube array. The light stores a `Scenes::BindlessTextureSet *` (set in each light's setup from `scene.bindlessTextureSet()`), not a manager pointer.

- **2D lights** (directional, spot): `set.registerTexture2D()` → `sampler2D` array at binding 1
- **Point lights**: `set.registerTextureCube()` → `samplerCube` array at binding 3
- **Sentinel value**: `UINT32_MAX` means no color projection texture assigned
- **Unregistration**: by texture instance (`set.unregisterTexture2D(texture.get())`), done in the light's `destroyFromHardware()`

The bindless set is bound during lighting passes when `renderPassUsesColorProjection(renderPassType)` returns true, alongside the standard environment cubemap usage.

**Code references:**
- `Scenes/Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` - Registration
- `Scenes/Component/AbstractLightEmitter.cpp:unregisterColorProjectionFromBindless()` - Cleanup
- `RenderableInstance/Abstract.cpp:render()` - Bindless set binding condition
- `Saphir/Generator/SceneRendering.hpp` - Pipeline layout enablement

### Lifecycle Constraints

> [!CRITICAL]
> **VMA Allocation Order**
>
> The BindlessTexturesManager holds references to Vulkan resources. During shutdown:
> 1. `Renderer::clearDefaultResources()` releases texture references
> 2. `ResourceManager::unloadUnusedResources()` frees VMA allocations
> 3. Only then can `Device::destroy()` safely destroy VMA allocator
>
> The `Core::terminate()` loop calls `unloadUnusedResources()` after each service
> to ensure proper cleanup order.

**Code references:**
- `BindlessTexturesManager.hpp/cpp` - Manager implementation
- `Renderer::createDefaultResources()` - Default cubemap initialization
- `Renderer::clearDefaultResources()` - Cleanup before shutdown

### GrabPass — NEED-DRIVEN arming (Aug 2026)

The grab blit (scene capture for TranslucentGB refraction/transmission) records whenever the
frame contains TranslucentGB objects (`Scene::hasTranslucentGBObjects()`); `Renderer::enableGrabPass(true)`
remains a manual force-on.
> [!WARNING]
> The flag alone used to gate the blit and NOTHING in the engine ever set it: the machinery
> was pre-allocated but dead, and every grab-pass material sampled an unfilled bindless slot
> (measured on CarConcept: uniform sky-blue glass, no interior, and NO "GrabPass" zone in
> `getGPUTimings()`). **The GPU timings are the one-command diagnostic**: a grab-pass material
> on screen without a GrabPass zone in the frame = the blit is not armed.
