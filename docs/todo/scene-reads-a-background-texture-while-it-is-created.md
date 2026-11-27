---
id: scene-reads-a-background-texture-while-it-is-created
title: Scene::setBackground() reads the background's environment cubemap while a loading thread creates it
status: open
priority: high
scope: Scenes (Scene::setBackground()), Graphics/TextureResource (TextureCubemap::isCreated()), Graphics/Renderable (SkyBoxResource::environmentCubemap())
opened: 2026-11-27
tags: [defect, concurrency, sky, threadsanitizer]
---

# Scene::setBackground() reads the background's environment cubemap while a loading thread creates it

## Why

ThreadSanitizer, 2026-11-27, the testbed's `default` demo on the software device, the process held on TWO cores
(`taskset -c 0-1`), ten `setBackground(name, true)` sent in a burst one second after the console answers: 2 data races
in one run, both in `Scene::setBackground()` called by the console command (main thread, the scene's exclusive access
held):

- **The reader** — `Scene::setBackground()` → `TextureCubemap::isCreated()`: it reads the texture's `m_image`
  (a `std::shared_ptr`, 8 bytes) then `Vulkan::AbstractObject::isCreated()` (1 byte) to decide whether the scene
  adopts the cubemap now.
- **The writer, a thread of the pool** — the cubemap is still LOADING: `Resources::Container< CubemapResource >::
  loadingTask()` → `CubemapResource::load()` → `loadEquirectangularHDR()` → `ResourceTrait::setLoadSuccess()`, which
  ends the dependency chain and creates the texture on the hardware (`m_image` assigned, its created flag set).

The same family as the background's stars (scenes doc 12 § *Plain state shared…*, the two rows of 2026-11-27): a
resource's members read by a scene before the resource reports itself loaded. Not seen at sixteen cores or more: the
pool finishes the load before the command reaches the scene. Not read: `SkyBoxResource::environmentCubemap()` returns
a member the sky box's own load assigns — the same question one level up.

## What remains

- The protocol is the owner's choice: the scene tests `isLoaded()` (the resource's atomic status) before it reads the
  texture — the late adoption on the rendering side already covers a cubemap that was not ready — or the texture
  publishes "created" by a flag of its own stored last.
- Read every member `Scene::setBackground()` and the late adoption (`getRenderableInstanceReadyForRendering`) read on
  a background or a cubemap still loading.
- Proof: the same run (below), 0 report naming `setBackground` or `TextureCubemap::isCreated`.

## ⚠️ Traps

- At full core count the run reports nothing: hold the process on two cores and send the switches in a burst.
- Until 2026-11-27 the music playlist decoded its 32 tracks on the pool at startup, which delayed every other load
  and made this family of races appear in a plain `default` run; since the playlist is a list of names the pool is
  free and they hide.

## References

- `src/Scenes/Scene.entities.cpp` (`Scene::setBackground()`), `src/Graphics/TextureResource/TextureCubemap.cpp`
  (`isCreated()`), `src/Graphics/CubemapResource.cpp` (`loadEquirectangularHDR()`),
  `src/Scenes/Manager.console.cpp` (`setBackground`).
- `docs/subsystems/scenes/12-frame-synchronization-double-buffering-contract.md` § *Plain state shared by the logic
  and rendering threads*.
