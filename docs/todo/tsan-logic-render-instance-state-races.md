---
id: tsan-logic-render-instance-state-races
title: TSan — the view's near and far planes are read in their LOGIC state on the rendering side; five other states of this family are fixed
status: in-progress
priority: high
scope: Graphics/ViewMatricesInterface and its three implementations (nearPlane(), farPlane()), Graphics/RenderTarget (viewDistance(), six implementations), Graphics/FrameView (farPlane()), Graphics/SelectionOutline, Scenes/Scene.rendering.cpp (the two list builders)
opened: 2026-11-15
tags: [tsan, concurrency, defect, measured]
---

# TSan — the view's near and far planes are read in their LOGIC state on the rendering side; five other states of this family are fixed

## Why

Both threads hold the active scene's access SHARED, so nothing orders a plain write on one side against a read on the
other (scenes doc 12 § *Plain state shared by the logic and rendering threads*).

**Fixed on 2026-11-16** (owner decisions; the protocols, the proofs and the numbers are in scenes doc 12, the last
seven rows of its table): an instance's animation time, its flag word, the view's position and matrices read on the
rendering side, the scene's ambient as the traced effects add it, and the extent a frame culls an entity on.
ThreadSanitizer: `light-and-shadow-debug` 1 report → 0, `game-logic` 4 → 0, `node-relation` 6 → 0, `collision-debug`
7 → 0, `beams` and `paths` 1 → 0, `citadel` 3 → 0.

**What is left, reported on `city` on 2026-11-16** (one report, once the culling's were gone):

| Read by the rendering thread | Written by the logic thread |
|---|---|
| `RenderTarget::ShadowMap< ViewMatricesCascadedUBO >::viewDistance()` → `ViewMatricesCascadedUBO::farPlane()` (the LOGIC state's buffer) ← `Scene::populateShadowCastingRenderList()` | `ViewMatricesCascadedUBO::updateFromMainCameraFrustum()` ← `DirectionalLight::updateCascades()` ← `Scene::updateCSMCascades()` ← `Scene::processLogics()`, at every cycle |

The same read exists, unreported because the value seldom changes there: `viewDistance()` of the five other render
targets (`Texture`, `ImposterBake`, `SelectionDepthTarget`, `SceneRenderTarget`, `Vulkan::SwapChain`), called by
`Scene::populateRenderLists()` too; `FrameView::farPlane()` — the frame's view itself answers from the logic state —
read by `RTGI` for its sky distance; `SelectionOutline.cpp` (`mainViewMatrices.nearPlane()` / `farPlane()`).
`ViewMatricesInterface` has no `nearPlane(readStateIndex)` / `farPlane(readStateIndex)`.

## What remains

- **The planes of a render state**: `nearPlane(readStateIndex)` / `farPlane(readStateIndex)` on the three view
  matrices (the published buffer holds them), `FrameView::nearPlane()` / `farPlane()` from the slot, a render target's
  view distance for a frame (`viewDistance(readStateIndex)`, or the list builders reading the view matrices' slot),
  the selection outline through the frame's view. The protocol is the one already decided for this family (the
  frame's view); the shape of `RenderTarget::Abstract::viewDistance()` is the choice to make.
- **One configuration bit is still written while the scene runs**: the console command that toggles an instance's
  tangent-space display (`Scenes/Manager.console.cpp`, `enableDisplayTBNSpace()`) writes the instance's flag word from
  the main thread while the rendering thread reads it. Not reported (the command was not sent in any run). The rule
  for a configuration change on a live instance is to decide. The ENTITY's own flag word is the same question
  (`AbstractEntity::isRenderable()` is read by the rendering thread, written by `updateEntityProperties()`).
- **The peers' round** on the fixed members (macOS, Windows), after the push.
- Proof when done: `city`, 0 report from the engine's logic / rendering pair; the sweep again.

## ⚠️ Traps

- The recipe and the traps of a TSan run on the software device — the settings keys without which a ray-traced scene
  kills it, the third key for `sponza`: `docs/caution-points.md` § *The software device gives a ray-query hit the
  identity of another instance*.
- `city` also reports two things that are NOT this item: `sky-material-atmosphere-read-while-its-creation-sets-it`
  (two loading threads), and the testbed's `city-reads-an-image-average-colour-while-it-loads`.
- ThreadSanitizer stops repeating a report: a family hidden behind another shows only once the first is fixed (the
  far plane appeared after the culling was). Run the sweep again after each fix.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the table, the rule for choosing a
  protocol, the proofs, the sweep, the cost of the published extent).
