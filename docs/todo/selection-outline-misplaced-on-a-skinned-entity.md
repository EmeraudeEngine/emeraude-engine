---
id: selection-outline-misplaced-on-a-skinned-entity
title: The selection outline of a skinned entity is drawn small and away from the entity
status: open
priority: high
scope: Scenes/Scene.rendering.cpp (renderSelectionDepth()), Graphics/SelectionOutline, Graphics/SelectionDepthTarget, Graphics/RenderableInstance::Abstract (the depth-only programs of a target, the skinning set), Saphir::Generator::ShadowCasting
opened: 2026-11-16
tags: [defect, rendering, measured]
---

# The selection outline of a skinned entity is drawn small and away from the entity

## Why

The testbed's `animation-debug`, the fox highlighted through the console
(`Core.SceneManagerService.highlightEntity(Fox)`, answered "Entity 'Fox' highlighted."), one capture 12 s after the
start (Linux, RTX 3070 Ti, Release, window-less, the validation layer loaded, 0 VUID, 2026-11-16): the orange outline
has the shape of a fox, but it is about a third of the fox's size on screen and sits near the centre of the picture,
some 230 pixels to the left of the fox it belongs to (1280 x 720). Nothing is outlined around the fox itself.

The same picture with three builds of that day — before and after the changes made to the outline's inputs (the
frame's view, the far plane of the render state) — so it is not theirs. The owner ranked it a defect.

`Scene::renderSelectionDepth()` draws each highlighted entity's instances into the selection depth target with the
DEPTH-ONLY programs of that target (`getReadyForShadowCasting(renderTarget, renderer)`, generated on first use), at
the entity's published world coordinates. A skinned mesh drawn WITHOUT its bone matrices is its bind pose at the mesh
node's scale, not the skeleton's — "a frozen mannequin", as the ray-tracing path already notes for a static structure
of a skinned geometry. That is what the picture looks like; it is a reading of the picture, not a measurement.

## What remains

- Establish the cause: does the depth-only program of the selection depth target skin (the shadow maps' programs
  do), and is the instance's skinning set bound and flushed for that pass (`flushSkinningMatrices()` — the frame's
  section)? A RenderDoc capture of the selection depth pass settles it (the vertex shader of the fox's draw, its
  bound sets).
- Compare with a NON-skinned entity of the same scene (the paladins are skinned too; highlight a static one in
  another scene) to bound the defect to skinning.
- Fix in the engine where the pass or the program lacks the pose; no special case in the outline.
- Proof: the capture again — the outline hugs the fox in two frames of its animation; a static entity unchanged.

## ⚠️ Traps

- `highlightEntity()` takes an entity ADDRESS (`listEntities()`); several entities carry a `Head`.
- The outline is recorded in two steps (`recordDepth()` in the scene part of the frame, `recordComposite()` after the
  tone mapping): a wrong depth source and a wrong composite look alike in the final picture.
- The same capture shows the debug compass (`toggleCompass()`) as a flat yellow bar at the centre of the picture.
  The owner did not rank it; noted here so the next capture of this item looks at it too.

## References

- `docs/caution-points.md` § *Skinned Meshes: Three Traps Paid For On The Same Dragon*.
- Scenes doc 12 (the frame's view, which the outline now receives).
