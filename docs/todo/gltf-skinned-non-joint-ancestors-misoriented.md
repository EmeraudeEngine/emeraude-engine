---
id: gltf-skinned-non-joint-ancestors-misoriented
title: Skinned glTFs with transformed non-joint ancestors are mis-oriented when animated (BrainStem, citadel's dragon)
status: open
priority: unranked
scope: Scenes/Loaders/GLTFLoader (skins and node animations), Animations/SkeletalAnimator, Scenes/Component/Visual
opened: 2026-10-01
tags: [gltf, skinning, animation, measured]
---

# Skinned glTFs with transformed non-joint ancestors are mis-oriented when animated (BrainStem, citadel's dragon)

## Why

Found by the macOS peer during the triad 12 validation (2026-10-01) and reproduced on Linux. Open
`glTF-Sample-Assets/Models/BrainStem/glTF/BrainStem.gltf` in the `+ModelViewer` (`Core.openFiles()`):

- at rest (bind pose) the robot stands upright and is framed correctly;
- as soon as `Core.cycleAnimation()` starts its clip (`clip_0`), the whole robot lies horizontally on the ground and
  tumbles. It is a rigid ~90° rotation of the whole figure.

This predates triad 12: Linux with the pre-12 `GLTFLoader` / `SkeletalAnimator` (engine `a777ddf7`) shows the same
pose, and the rest poses before and after 12 are identical (mean per-pixel difference 0.007). CesiumMan, Fox,
RiggedFigure and SimpleSkin animate upright.

The asset's hierarchy, read by the macOS peer:

- scene root node 0 carries a matrix of +90° about X;
- node 0 → node 21 (no transform) → node 2, whose rotation is about −90° about X;
- node 2 is the skin's `skeleton` and the NON-JOINT parent of the root joint (node 3);
- the 18 joints are already parents first (triad 12's reordering is the identity here);
- the animation has 57 channels, and one of them targets node 2 itself: the only non-joint target.

**The same family, reported by the owner (2026-10-01): citadel's dragon** (`data-stores/glTF/Dragon.glb`, actor
`projet-alpha/src/Actor/Dragon.cpp`) looks DOWN while perched, i.e. while its `Qishilong_stand` clip loops. Its
hierarchy (read 2026-10-01):

- the root joint's ancestors are `Object_4` → `RootNode` → `Object_2` (no transform) →
  `M_B_44_Qishilong_skin_Skeleton.FBX` (rotation +90° about X, scale 0.0254) → `Sketchfab_model` (rotation −90°
  about X): two transformed NON-joint ancestors that cancel each other's rotation;
- 1 skin of 156 joints, already parents first; no `skeleton` property;
- 27 clips of 468 channels each, ALL on joints: no non-joint channel, unlike BrainStem.

A clue from the Windows peer: of CesiumMan, Fox, BrainStem, RiggedFigure and SimpleSkin, only BrainStem logs
"[SceneDataConsumer] 1 node animation clip(s) attached, driving 1 node(s)" (the node-2 channel). Its 5 screenshots
over 8 s without `cycleAnimation()` are pixel-identical (bind pose).

Both assets have transformed non-joint ancestors above the skeleton root. The likely cause, not proven: once animated, the transform of the non-joint ancestor node 2 (its rest rotation or its
animated one) no longer reaches the joint matrices, while node 0's +90° still applies, so the figure tips over by a
net 90°. Triad 12's `loadAnimations()` sorts a non-joint channel into a separate node clip.

## What remains

- [ ] Find which transform is lost. Compare the world matrix of the root joint at rest and at frame 0 of the clip, and
  check where the node-2 channel (the node clip) is applied relative to the skinned evaluation.
- [ ] Fix it in the engine so that the skinned joints are evaluated under the animated non-joint ancestors.
- [ ] Re-test BrainStem upright while animating (plain, Draco and Meshopt variants), and CesiumMan, Fox, RiggedFigure
  and SimpleSkin unchanged. Re-test citadel's dragon perched (`Qishilong_stand`), looking straight ahead. Add BrainStem animated to the glTF conformance bench.

## References

- `src/Scenes/Loaders/GLTFLoader.cpp`: `loadSkins()`, `loadAnimations()` (joint channels versus node channels).
- `src/Animations/SkeletalAnimator.cpp`: `computeWorldMatrices()`.
- `docs/todo/triad-engine-pass.md` § 12 (the peer results).
