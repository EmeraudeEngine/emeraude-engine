---
id: tsan-logic-render-shared-state-races
title: TSan — the logic and rendering threads share plain scene state under the SHARED scene access (five races)
status: open
priority: high
scope: Scenes/Scene, Graphics/PostProcessStack, Graphics/Renderable, Scenes/LightSet, Graphics/RenderableInstance
opened: 2026-11-13
tags: [tsan, concurrency, defect, measured]
---

# TSan — the logic and rendering threads share plain scene state under the SHARED scene access (five races)

## Why

A ThreadSanitizer run of projet-alpha `default` (2026-11-13, Ubuntu 26.04, Debug + TSan, `--disable-cef`, the AMD RADV
device forced through a settings copy, 138 s to the start, a clean `Core.shutdown()`) reported 132 data races: 127 inside
uninstrumented third-party code (libglycin, glib, gio, pango, fontconfig, pulse) and FIVE in the engine, all between the
logic thread (`Core::logicsTask()`, `withLogicCycle` → `withSharedActiveScene`) and the rendering thread
(`Core::renderingTask()`, `withSharedActiveScene`): both hold the scene access SHARED, so nothing orders a write on one
side against a read on the other. The act's build thread (projet-alpha `Stage::buildPumpingWindow()`) is party to none
of them (it only allocated the objects). "As if synchronized via sleep" on some reports: the ordering exists by timing
only.

| # | Writer (logic thread unless said) | Reader (rendering thread) | Member |
|---|---|---|---|
| 1 | `Scene::processLogics()` `Scene.cpp:418-419` | `Scene::updateVideoMemory()` `Scene.rendering.cpp:644` (`windTime`) and `Scene::lifetimeMS()` `Scene.hpp:611` from `Renderer::renderFrameWithInternal()` | `m_lifetimeUS`, `m_lifetimeMS` |
| 2 | **rendering thread** `PostProcessStack::rebuildOrderedEffects()` `:993`, `:1002` from `syncCameraEffects()` (`Renderer::renderFrame()`) | **logic thread** `PostProcessStack::hasEnabledIndirectDiffuseProvider()` `:1027-1028` from `Scene::updateIBLDiffuseOwnership()` (`Scene::processLogics()`) | the ordered effect list |
| 3 | **a pool worker** `Renderable::Abstract::setReadyForInstantiation()` (`SkyBoxResource::onDependenciesLoaded()`) | `Renderable::Abstract::isReadyForInstantiation()` from `RenderableInstance::Abstract::isReadyToRender()` (`Scene::populateRenderLists()`) | the `FlagTrait` word |
| 4 | `LightSet::enable()` `LightSet.hpp:222` from `Scene::applyBackgroundLightingNow()` (`Scene::processLogics()`) | `LightSet::isEnabled()` `LightSet.hpp:233` from `Saphir::Generator::SceneRendering::isLightingRequested()` (shader generation in `getReadyForRender()`) | `m_enabled` |

| 5 | **rendering thread** `RenderableInstance::Abstract::createSkinningResources()` `Abstract.cpp:365`, `:411`, `:434` from `prepareSkinningResources()` ← `getReadyForShadowCasting()` (`Scene::castShadows()`) | **logic thread** `hasSkinningResources()` `Abstract.hpp:878` and `updateSkinningMatrices()` `Abstract.cpp:459` from `Component::Visual::processLogics()` `Visual.cpp:63-65` (`Scene::processLogics()`) | a skinned instance's skinning buffer pointer and its descriptor-set vector |

Full report: the 2026-11-13 session's `tsan-default-report-radv.txt` (scratchpad, not kept); reproduce with the recipe
below. Member 5 was added on 2026-11-15 (a skinned scene, `animation-debug`, llvmpipe, four reports in each of two
runs): the renderer creates the skinning buffer and its descriptor sets on its first shadow pass while the logic thread
tests and writes them — beyond a stale read, the logic thread can write the joint matrices through a half-built object.

## What remains

- Decide the publication contract for each member (owner): an atomic (3, 4 and the two lifetimes are single words), a
  per-render-state-slot copy (the engine's model for what the logic publishes), or the effect list rebuilt by the logic
  thread rather than by the renderer (2). Fix, then re-run TSan: 0 engine report on `default` and `terrain`.
- The third-party reports are not ours: glycin and glib are uninstrumented, TSan cannot see their atomics. A suppression
  file for them (`TSAN_OPTIONS=suppressions=`) would make the engine's reports stand out; it is a tool configuration,
  not a silenced finding, and it goes under `tools/` with this item's reference.

## ⚠️ Traps

- **TSan and the NVIDIA Vulkan driver (595.99.02)**: the instance died at device creation (`ThreadSanitizer: DEADLYSIGNAL`,
  a SEGV in a driver thread, "nested bug in the same thread, aborting"), before the engine started. Run TSan on a Mesa
  device: `Core/Video/VulkanDevice/ForceGPU` in a settings COPY (`--settings-filepath`), `"AMD Radeon Graphics (RADV …)"`
  here, `"llvmpipe (…)"` on any machine.
- **Not the Intel iGPU on a machine whose compositor runs on the NVIDIA GPU** (tried 2026-11-14 on the Linux
  workstation): the compositor refuses its buffers at the first present and the instance loses its display
  (`igpu-presentation-under-nvidia-wayland`). `ForceGPU = "llvmpipe (LLVM …)"` works there: `default` started in 6 s
  with the window reduced to 960 x 540 in the same settings copy (software rendering under TSan is slow, the main
  thread still answers), and the suppression file the build copies beside the binary already covers it
  (`TSAN_OPTIONS=… suppressions=tsan_suppressions.txt`, `race:libLLVM*`, `race:libgallium*`).
- That llvmpipe run reported three more races outside this item's family: the renderer's sampler cache and the
  local-data release pass (both fixed the same day, `docs/caution-points.md`), and downstream projet-alpha's
  `act-activation-races-the-logic-thread` (closed 2026-11-15: `Scenes::Manager::betweenLogicCycles()`, scenes doc 08).
- **CEF does not start under TSan** (2026-11-15: `free(): invalid size`, SIGABRT at the first browser subprocess): every
  TSan run takes `--disable-cef`, and what only a web screen triggers is out of its reach.
- **A skinned scene does not survive llvmpipe** (`animation-debug`: the driver's rasterizer thread dies within a second,
  TSan or not — item `skinned-scene-crashes-the-software-vulkan-device`): on llvmpipe, keep to static scenes.
- TSan exits with code 66 when it reported anything (`halt_on_error=0` keeps the run going): the engine's own exit code
  is in the journal ("User exit code").
- The recipe: `cmake -S . -B .claude-build-tsan -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TSAN=On -G Ninja`, then
  `TSAN_OPTIONS="halt_on_error=0 second_deadlock_stack=1 log_path=<scratch>/tsan-report" ./projet-alpha --load-demo
  default --disable-cef --disable-settings-autosave --enable-log=… --enable-remote-console=7780 --set-vk-layers=
  --settings-filepath=<copy>`; 8 min of build on 8 cores, 138 s to the start.

## References

- Engine `docs/caution-points.md` § *TSan on a machine whose display GPU runs the NVIDIA driver*.
- projet-alpha `docs/caution-points.md` § Core Application (the act built on a worker, which this run validated).
