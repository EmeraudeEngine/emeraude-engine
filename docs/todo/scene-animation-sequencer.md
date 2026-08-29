---
id: scene-animation-sequencer
title: Scene animation sequencer — entities on a timeline, values on channels
status: open
priority: unranked
scope: Animations
opened: unknown
blocked-by: [animatable-properties-coverage]
tags: [animation, scene, tooling]
---

# Scene animation sequencer — entities on a timeline, values on channels

## Why

**Owner intent (recorded 2026-08-26):** general, scripted, scene-wide animation — pick entities
on a **timeline**, set values on their animatable elements, and drive the whole scene from those
**channels**. A cutscene mechanism owned by the runtime, not hand-written code per demo.

## What remains

- [ ] The channel model (an entity + one of its animatable properties + keyed values over time),
  its interpolation and its evaluation per frame.
- [ ] The timeline that owns the channels, plays, seeks and loops.
- [ ] How a sequence is authored and stored (a scene JSON is the natural home — see
  `json-resource-describes-subresources.md`).

## Blocked by

`animatable-properties-coverage.md`: a channel can only address a property an entity actually
exposes.

## ⚠️ Traps

**`Scenes::Component::NodeAnimation` (Aug 2026) does NOT solve this, and is not a starting point
for it.** It plays the TRS clips an IMPORTED ASSET ships with — a glTF rotating a bezel — over the
node hierarchy the loader built, addressing its targets by the clip's own `targetIndex`. It is
asset playback, not scene authoring: no timeline, no seek, no arbitrary property, no cross-entity
channels. Treat it as a second consumer of `Base::Animation::AnimationClip`, alongside
`SkeletalAnimator`, and nothing more.

⚠️ It deliberately bypasses `Animations::AnimatableInterface` — that map is keyed by animation ID
(one animation per object, no clip selection) while one imported clip drives many nodes. **The
sequencer will need `AnimatableInterface`'s successor to carry a clip/channel identity**, which is
precisely what `animatable-properties-coverage.md` has to establish first. Do not conclude from
`NodeAnimation` that the interface is unnecessary.
