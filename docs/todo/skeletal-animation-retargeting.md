---
id: skeletal-animation-retargeting
title: Retarget an animation clip from one skeleton to another
status: open
priority: unranked
scope: Animations
opened: 2026-08-31
tags: [animation, skeleton, mocap]
---

# Retarget an animation clip from one skeleton to another

## Why

`Animations::SkeletalAnimator` can only play a clip authored against the very skeleton it holds.
Everything else poses the wrong joints, silently. That locks the engine out of every motion-capture
library, every AI motion source, and any reuse of one animation set across characters of different
proportions.

Nothing in the cascade does this — `rg "retarget"` over `emeraude-engine/src` and
`emeraude-base/src` returns zero. It is not even in the `Animations/AGENTS.md` "Remaining Work"
list, which mentions blending, a state machine and a timeline.

**Owner decision (2026-08-30): a chantier of its own, generic and measured first**, not a
by-product of any particular motion source. It pays for itself independently of what feeds it.

## What remains

A prototype in Python proved the whole thing end to end and produced every number the C++ needs.
See [`../animation-retargeting.md`](../animation-retargeting.md) — mathematics, the SOMA-30
reference skeleton, the joint mapping, the traps, and the verification protocol.

- [ ] `Animations::Retargeter` — a joint correspondence table plus the bind-pose delta, producing
      an `AnimationClip` targeting the destination skeleton's joint indices.
- [ ] Root translation scaled by the **leg-length ratio** (not the stature). Coordinate with
      [`root-motion-mode.md`](root-motion-mode.md), which decides who consumes it.
- [ ] Unmapped target joints keep their **rest** local rotation — never a zeroed quaternion.
- [ ] Explicit mapping for **prop bones** (`Sword_joint`, `Shield_joint`): they have no source
      counterpart and geometric proximity is not a substitute.
- [ ] A ground-contact pass. Measured residual after retargeting Kimodo onto the Paladin:
      **2 to 5 cm** of foot penetration, caused by the proportion mismatch, not by the source.
- [ ] A **clip serializer**. `AnimationClipResource` loads from a path, a JSON value or memory, but
      nothing writes one back, so a retargeted clip does not survive the session.

## ⚠️ Traps

- **`LeftLeg` denotes a different bone in each skeleton** — the thigh in SOMA, the shin in Mixamo.
  A by-name mapping (which `FBXLoader::loadAnimationClipsOnly()` already does for split-animation
  FBX) breaks the legs with no warning. Resolve through an explicit table.
- **The bind-pose delta vanishes only when both rest poses are the same pose with identity rest
  rotations.** True for SOMA↔Mixamo (both T-poses), false in general. Implementing the trivial case
  and calling it done produces a permanently twisted character on the first A-pose source.
- **`play()` takes the CLIP's own name, never the resource key**, and returns a bool that must be
  read — the documented cause of `asset-loader` cycling nothing.
- Verification order that isolates failures: **identity first** (retarget a skeleton onto itself,
  demand a bit-identical result), then rest pose, then proportions, then contact.

## References

- [`../animation-retargeting.md`](../animation-retargeting.md) — the full reference
- [`../text-to-motion-kimodo.md`](../text-to-motion-kimodo.md) — the source it was proven against
- [`root-motion-mode.md`](root-motion-mode.md) — the root translation's consumer
- Working Python prototype: `dependencies/kimodo.cpp/tools/`, with its own `README.md`. Replaying
  the chain reproduces its glTF output **byte for byte** — the regression test if it is ever
  touched. ⚠️ The checkout is **unversioned**: the prototype exists on this workstation only.
