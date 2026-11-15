---
id: mesh-loaded-hook-configures-materials-still-being-created
title: The loaders' onMeshLoaded hook hands out materials whose asynchronous creation still runs — configuring them there races the creation
status: open
priority: high
scope: Scenes/Loaders (FBXLoader, GLTFLoader, LoaderOptions), Graphics/Material/StandardResource, Resources/Container
opened: 2026-11-15
tags: [tsan, concurrency, defect, measured, api]
---

# The loaders' onMeshLoaded hook hands out materials whose asynchronous creation still runs — configuring them there races the creation

## Why

`Scenes::Loaders::LoaderOptions::onMeshLoaded` is documented as the place to "patch the descriptor in place (e.g.
enable IBL reflection on PBR materials…)". The FBX loader builds each material with
`Resources::Container< Material::StandardResource >::getOrCreateResource(name, configure)`: the container returns the
new resource AT ONCE and runs `configure` (`setAlbedoComponent()`, the roughness, the normal map…) as a pool task. The
hook is then called on the loading thread with that material in the descriptor — while `configure` may still be
running, or not yet started.

**Measured** (ThreadSanitizer, 2026-11-15, the testbed's `animation-debug` on llvmpipe, one report in each of three
runs): a pool worker in `StandardResource::setAlbedoComponent()` → `FlagTrait::enableFlag()` (the creation function,
`FBXLoader.cpp`) against the loading thread in `StandardResource::setReflectionComponentFromEnvironmentCubemap()` →
`Material::Interface::isCreated()` → `FlagTrait::isFlagEnabled()` (the testbed's hook, called from
`FBXLoader::load()`), on the material's plain flag word.

What it can do, by reading the code (not observed):

- the hook's setter tests `isCreated()` and REFUSES once the material is created ("The resource … is created ! Unable
  to create or change the reflection component."): whether a model gets its reflection component depends on which
  thread is ahead;
- both threads set bits of the same flag word and fill the same component table without a lock: a lost bit, a
  component written while the creation reads the table.

## What remains

- **Decide the contract (owner)**:
  1. a material hook that runs INSIDE the creation function, before the material is created (a
     `LoaderOptions::onMaterialConfigure(StandardResource &)` beside `onMeshLoaded`, called by `configure` on the pool
     task): one writer, the documented use case keeps working; `onMeshLoaded` is then documented "the materials are
     still being created: read their identity, never configure them";
  2. the loader waits for the materials' creation before calling the hook (the load of a model becomes as long as its
     slowest material: the textures);
  3. the option the use case really wants: a loader option "reflection from the environment cubemap" applied by
     `configure` itself, and no material mutation from a hook at all.
- The glTF loader calls the same hook (`GLTFLoader.cpp`): read how it creates its materials before deciding it is safe.
- The testbed's caller (`Actor/Paladin.cpp`, `customizeMesh`) moves to the chosen contract.
- Proof: TSan on a switch to `animation-debug`, 0 report on a material's flags; the paladins keep their reflections
  (a pixel A/B at a pinned exposure).

## ⚠️ Traps

- One report per run in the three runs: at the first load of the model.
- The software device dies on that scene a second later (`skinned-scene-crashes-the-software-vulkan-device`); the
  report is written before.

## References

- `load-finished-runs-scene-handlers-on-a-pool-thread` (the same runs; the other "a resource used while it loads").
- `docs/caution-points.md` § *TSan on a machine whose display GPU runs the NVIDIA driver* (the recipe).
