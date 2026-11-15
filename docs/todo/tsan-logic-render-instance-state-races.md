---
id: tsan-logic-render-instance-state-races
title: TSan — four more states written by the logic thread and read by the rendering thread (instance frame index, instance flags, the 2D view matrices, the light set's ambient)
status: open
priority: high
scope: Graphics/RenderableInstance (Abstract, Multiple), Graphics/ViewMatrices2DUBO, Scenes/SceneMetaData, Scenes/Component (Visual, ParticlesEmitter, Camera)
opened: 2026-11-15
tags: [tsan, concurrency, defect, measured]
---

# TSan — four more states written by the logic thread and read by the rendering thread (instance frame index, instance flags, the 2D view matrices, the light set's ambient)

## Why

Both threads hold the active scene's access SHARED, so nothing orders a plain write on one side against a read on the
other (scenes doc 12 § *Plain state shared by the logic and rendering threads*: six such members got a protocol on
2026-11-15). Scenes the earlier runs did not cover report four more (ThreadSanitizer, 2026-11-15, Debug + TSan on
llvmpipe, the logic thread `Core::logicsTask()` against the rendering thread `Core::renderingTask()`; members 1 to 3
one run each, member 4 found by the proof runs of the ray-tracing refactor):

| # | Scene | Written by the logic thread | Read by the rendering thread | State |
|---|---|---|---|---|
| 1 | `light-and-shadow-debug` | `RenderableInstance::Abstract::updateFrameIndex()` ← `Component::Visual::processLogics()` | `RenderableInstance::Abstract::frameIndexFor()` ← `drawShadowCaster()` ← `castShadows()` ← `Renderer::renderShadowMaps()` | the instance's animation frame index |
| 2 | `game-logic` | `FlagTrait::disableFlag()` ← `RenderableInstance::Multiple::updateLocalData()` ← `Component::ParticlesEmitter::processLogics()` | `FlagTrait::isFlagEnabled()` ← `RenderableInstance::Abstract::isLightingEnabled()` ← `getReadyForRender()` ← `Scene::populateRenderLists()` | the instance's plain flag word (one bit written, another read: the whole word races) |
| 3 | `game-logic` | `ViewMatrices2DUBO::updateViewCoordinates()` ← `Vulkan::SwapChain::updateDeviceFromCoordinates()` ← `Component::Camera::processLogics()` | `Scenes::SceneMetaData::rebuild()` ← `Scene::prepareRender()` ← `Renderer::renderFrameWithInternal()` | the view matrices' LOGIC state, read by the renderer (the render-state slot exists for that: scenes doc 12 § View Matrix State Index) |
| 4 | a switch to `animation-debug` (one run of four) | `LightSet::setAmbientLightColor()` ← `Scene::applyBackgroundLightingNow()` ← `Scene::processLogics()` | `Effects::Lighting::RTR::recordPreDenoisePasses()` (the ambient term of its frame block) ← `PostProcessor::executeIndirectPostProcessEffects()` | the light set's ambient colour (and whatever else the background's deferred lighting writes there) |

## What remains

- Read both sides of each member and choose its protocol with the rule of scenes doc 12 (owner): (1) one word that
  stands alone → an atomic; (2) a flag word shared by bits of different owners → split the bit the logic thread writes
  out of the word, or make the word atomic; (3) values that must agree for a frame → the reader takes the render-state
  slot, never the logic state.
- **Sweep the testbed's scenes under TSan**: `default`, `animation-debug` and `terrain` are clean of this family;
  `light-and-shadow-debug` and `game-logic` were run once each for another purpose and each found members. The other
  scenes were never run. One run per scene on llvmpipe (static scenes start in seconds; a skinned one dies after its
  reports are written).
- Proof: the two scenes, 0 report on these members.

## ⚠️ Traps

- Member 2 is the instance's flag word, not the renderable's (whose "ready" bit became an atomic on 2026-11-15).
- The recipe and the traps of a TSan run: `docs/caution-points.md` § *TSan on a machine whose display GPU runs the
  NVIDIA driver*.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads*.
