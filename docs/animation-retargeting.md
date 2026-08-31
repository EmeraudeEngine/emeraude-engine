# Animation Retargeting

Playing an animation clip authored for **one** skeleton on a **different** skeleton.

The engine cannot do this today — `rg "retarget"` over `emeraude-engine/src` and
`emeraude-base/src` returns nothing. This document holds the contract, the mathematics and every
measurement gathered while proving the idea offline (2026-08-30/31), so the C++ implementation
starts from numbers rather than from intent.

The open work is tracked in [`todo/skeletal-animation-retargeting.md`](todo/skeletal-animation-retargeting.md).

## 1. Why the engine needs it

`Animations::SkeletalAnimator` plays an `AnimationClip` whose `AnimationChannel::targetIndex`
addresses **the skeleton the clip was authored against**. Feed it a clip built for another
skeleton and it poses the wrong joints, silently. That single constraint locks the engine out of:

- every motion-capture library that is not already rigged to the target character;
- every AI text-to-motion model (see [`text-to-motion-kimodo.md`](text-to-motion-kimodo.md));
- sharing one animation set across characters with different proportions.

## 2. What a clip actually carries

A `AnimationChannel` holds **local rotations** (a quaternion per joint per key) plus, on the root,
a **translation**. This asymmetry is the whole basis of retargeting:

> A local rotation is expressed in its parent's frame and carries **no length**. It transfers to a
> skeleton with completely different bone lengths and still means the same thing. The **root
> translation is the only quantity carrying a distance**, and it is the only one that must be
> scaled.

## 3. The mathematics

### General case

For a source joint `s` mapped to a target joint `t`:

```
D(s)    = G_rest_src(s)⁻¹ · G_rest_tgt(t)      // bind-pose delta, CONSTANT, precomputed once
G_tgt(t) = G_src(s) · D(s)                      // per frame, in GLOBAL space
L_tgt(t) = G_tgt(parent(t))⁻¹ · G_tgt(t)        // back to local, what the channel stores
```

`G_*` are global (world) joint orientations obtained by forward kinematics; `L_*` are the local
rotations a clip stores. The delta is what makes the source's *rest* pose land on the target's
*rest* pose: at rest `G_src(s) = G_rest_src(s)`, so `G_tgt(t) = G_rest_tgt(t)`. Verify that
identity first — it is the cheapest correctness test of the whole pipeline.

Target joints with no source counterpart keep their **rest local rotation**. They must not be left
uninitialised: fingers, jaw and eyes are usually unmapped and a zeroed quaternion collapses them.

### The special case that made the prototype trivial

When **both** skeletons have identity rest rotations (each joint's rest transform is a pure
translation) **and** are in the **same rest pose**, `D(s)` collapses to the identity and the source
rotations transfer verbatim. That is what happens between Kimodo's SOMA-30 and a Mixamo rig: both
are authored as pure joint offsets, and both are **T-poses**.

⚠️ **Do not generalise from it.** An A-pose source on a T-pose target needs the full delta, and
getting that wrong produces a plausible-looking but permanently twisted character. Verify the rest
pose of any new source before assuming.

### Root translation

```
scale = (target leg length) / (source leg length)
```

Leg length, **not** stature: the stride is produced by the legs. Measured for Kimodo SOMA-30 onto
the Mixamo Paladin: `0.7868 / 0.8553 = 0.9200`. Skipping it makes the feet slide by exactly the
mismatch. See also [`todo/root-motion-mode.md`](todo/root-motion-mode.md), which decides who
consumes that translation once it is correct.

## 4. Worked example — Kimodo SOMA-30 onto the Mixamo Paladin

Everything below is measured, not quoted.

### The SOMA-30 skeleton

Names, parents and parent-local offsets are published in kimodo.cpp's `demo/skeletons_extra.go`,
copied from NVIDIA's Apache-2.0 skeleton definitions.

```
Hips · Spine1 · Spine2 · Chest · Neck1 · Neck2 · Head · Jaw · Left/RightEye
Left/RightShoulder · Arm · ForeArm · Hand · HandThumbEnd · HandMiddleEnd
Left/RightLeg · Shin · Foot · ToeBase
```

Reference body, derived from the offsets:

| quantity | value |
|---|---|
| stature | ≈ 1.687 m |
| hip height | 0.9887 m |
| thigh / shin | 0.4323 / 0.4230 m |
| upper arm / forearm | 0.2874 / 0.2709 m |

⚠️ The offset stored **on** joint `J` is the vector `parent → J`. The humerus length is therefore
carried by `LeftForeArm`, not by `LeftArm`. Getting this backwards shifts every measurement by one
bone.

**Axis convention** — undocumented upstream, derived from the rest offsets and then confirmed by a
generated motion (the root advanced +6.41 m along +Z on a "walk forward" prompt):

| axis | meaning | evidence |
|---|---|---|
| +Y | up | spine offsets +Y, legs −Y |
| +Z | forward | eyes +0.0759 Z, jaw +0.0309 Z, toes +0.1323 Z |
| +X | the character's left | `LeftArm` +X, `RightArm` −X |

Right-handed (`Y × Z = +X`). This matches the glTF 2.0 authoring convention, and therefore the
engine's, since the Y-up flip made imports the identity.

⚠️ **"Forward" is stated twice in this project and means two different things.** The engine's
`AGENTS.md` says `-Z` forward: that is the **camera** looking direction (the OpenGL convention).
glTF says the **front of an asset faces +Z**: that is an authoring convention. They are consistent
— a model whose front is +Z, seen by a camera looking down −Z, faces that camera. Do not "fix" one
against the other; the mirror incident of Aug 2026 started exactly this way. See
[`coordinate-system.md`](coordinate-system.md).

### The joint mapping

| SOMA-30 | Mixamo | note |
|---|---|---|
| `Hips` | `Hips` | direct |
| `Spine1` `Spine2` `Chest` | `Spine` `Spine1` `Spine2` | **name shift**, 3 ↔ 3 |
| `Neck1` | `Neck` | |
| `Neck2` | — | interpolated midway `Neck` → `Head` |
| `Head` | `Head` | direct |
| `Jaw` | — | derived from `Head` |
| `Left/RightEye` | `Left/RightEye` | direct |
| `Shoulder` `Arm` `ForeArm` `Hand` | identical names | direct |
| `HandThumbEnd` `HandMiddleEnd` | `Thumb4` `Middle4` | coarse hand, 2 bones only |
| `Leg` `Shin` `Foot` `ToeBase` | `UpLeg` `Leg` `Foot` `ToeBase` | ⚠️ **see below** |

### ⚠️⚠️ Trap 1 — `LeftLeg` means the opposite thing in each skeleton

SOMA's `LeftLeg` is a child of `Hips`: it is the **thigh**.
Mixamo's `LeftLeg` is a child of `LeftUpLeg`: it is the **shin**.

The name exists in both and denotes a different bone. A by-name mapping — the obvious first
implementation, and the one `Scenes::Loaders::FBXLoader::loadAnimationClipsOnly()` already uses for
split-animation FBX — puts the thigh rotation on the shin and **breaks the legs without a single
warning**. Any retargeter must resolve joints through an explicit table, never by name equality.

### ⚠️ Trap 2 — prop bones have no counterpart

The Paladin rig carries `mixamorig:Sword_joint` and `mixamorig:Shield_joint`. SOMA has nothing to
map onto them. In the prototype the leftover vertices were bound to their geometrically nearest
bone, which works for the shield (close to the forearm) and **fails for the sword** (its blade is
far from every bone, so it ends up attached to whatever happens to be near and visibly detaches).
Prop bones must be mapped **explicitly** onto the hand that holds them.

### Measured proportions

| bone | SOMA | Paladin | delta |
|---|---|---|---|
| thigh | 0.4323 | 0.4170 | −3 % |
| shin | 0.4230 | 0.3698 | −13 % |
| upper arm | 0.2874 | 0.2557 | −11 % |
| **forearm** | **0.2709** | **0.1866** | **−31 %** |
| hip height | 0.9887 | 0.9558 | −3 % |
| stature | 1.687 | 1.725 | +2 % |

A heroic character is stockier than a mocap reference body. Note that stature is *larger* while
every limb is *shorter* — which is exactly why the root scale must come from the legs.

### Residual after retargeting

Feet penetrate the ground by **2 to 5 cm** depending on the pose, driven by the −13 % shin. A
ground-contact pass is required for production use; the source motion itself is nearly foot-locked
(**1.3 to 1.7 cm** of drift per stance on a 160 cm stride, measured on the SOMA skeleton before
retargeting), so the penetration is introduced by the proportion mismatch, not by the model.

## 5. How to verify a retargeter

In this order, because each step isolates one failure:

1. **Identity** — retarget a skeleton onto **itself**. The output must be **bit-identical** to the
   input. This catches frame conventions, quaternion order and the delta computation in one shot.
2. **Rest pose** — apply the retarget with a rest-pose clip. The target must land exactly on its
   own rest pose.
3. **Proportions** — retarget onto a skeleton with known different bone lengths and check the
   stride against the leg-length ratio.
4. **Ground contact** — measure per-stance foot drift, not per-frame velocity. A crude
   "foot is low" contact detector reports 5 m/s slides that are pure detector artefacts; window the
   measurement to contiguous stance phases and report the **net displacement** over each.

⚠️ **Never judge "is it animating?" on raw pixels.** The viewer camera runs automatic exposure and
a temporal chain, so two consecutive frames always differ — measured at 10.65 % of pixels changing
with the animation OFF. Compare **heavily downsampled** frames: on a 96×54 reduction the noise
floor was 144/5184 cells against 736/5184 for a playing clip.

## 6. The offline prototype

A working Python implementation of everything above lives at `dependencies/kimodo.cpp/tools/`,
inside the evaluation checkout. ⚠️ **That checkout is unversioned** (covered by this repository's
`dependencies/*` ignore rule), so these scripts are backed up by nothing — treat them as the
reference to READ while writing the C++, not as something to rely on being there:

| script | role |
|---|---|
| `dump_rig_joints.py` | Blender: dumps a target rig's joint positions and its axis convention |
| `build_retargeted_skeleton.py` | **the retargeting proper** — the SOMA↔Mixamo table, the frame conversion, the root scale |
| `blender_bind_mesh_to_skeleton.py` | Blender: heat weights + UV/texture extraction |
| `make_paladin_glb.py` | assembles the retargeted skeleton, weights, textures and clips into a glTF |
| `make_mannequin_glb.py` | a box-per-bone proxy mannequin, for looking at a source skeleton directly |

⚠️ Their relative-path defaults (`--soma demo/skeletons_extra.go`) assume the kimodo checkout as
the working directory. Run them from there, or pass the paths explicitly.

⚠️ **Blender pitfall, and it does not present as a scale error.** Detaching a mesh from its
armature with `object.parent = None` **loses the inherited transform**: a Mixamo rig arrives in
centimetres and the mesh comes out ~70× too large. The symptom observed was *"2310 vertices could
not be weighted"*, which sends you looking at the weighting. Use
`bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')` followed by `transform_apply`, and
**assert the resulting bounding box is human-sized** before going further.

⚠️ UVs are carried by the **loop**, not by the vertex: a vertex on a seam holds several. Exporting
per-vertex assigns one UV to all of them and shifts the texture. Deduplicate on `(vertex, uv)`.

## 7. What the engine already provides

The pieces a retargeter needs are in place, which is why this is a contained piece of work:

| piece | where |
|---|---|
| `Joint` — name, parentIndex, local T/R/S, inverse bind matrix | `Base/Animation/Joint.hpp` |
| `Skeleton` — ordered joints, name lookup, hierarchy validation | `Base/Animation/Skeleton.hpp` |
| `AnimationChannel` — keyframes + sampling | `Base/Animation/AnimationChannel.hpp` |
| **In-memory clip injection** | `Animations::AnimationClipResource::load(AnimationClip<float>)` |
| Clips resolved against an external skeleton **by name** | `Scenes::Loaders::FBXLoader::loadAnimationClipsOnly()` |
| `composeTRS()` / `decomposeTRS()`, `Quaternion::slerp()` | `Base/Math/TransformUtils.hpp`, `Quaternion.hpp` |

Missing, besides the retargeter itself: **no clip serializer** — `AnimationClipResource` loads from
a path, a JSON value or memory, but nothing writes one back, so a retargeted clip does not survive
the session.

## Link Index

- [`todo/skeletal-animation-retargeting.md`](todo/skeletal-animation-retargeting.md) — the open work
- [`todo/root-motion-mode.md`](todo/root-motion-mode.md) — who consumes the root translation
- [`text-to-motion-kimodo.md`](text-to-motion-kimodo.md) — the AI motion source this was proven against
- [`coordinate-system.md`](coordinate-system.md) — the engine frame
- [`../src/Animations/AGENTS.md`](../src/Animations/AGENTS.md) — the animation subsystem
