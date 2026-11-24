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

## 8. A mesh with NO skeleton — rig it on the source's own topology (2026-11-21)

A generated character (a text-to-3D or image-to-3D mesh) arrives with no rig at all. There is then
nothing to retarget **onto**, and the cheapest correct answer is to stop retargeting: give the mesh a
skeleton with the **source's** topology — SOMA-30 — laid at the mesh's own joints. Kimodo's local
rotations then play **verbatim**, with no table, no delta per clip and no `Animations::Retargeter`.
That is what the prototype of § 6 did to the Paladin all along (it re-rigged it, it never
retargeted it); the only new problems are the ones a rig used to solve for free.

Proven end to end on `RevenantAlt.glb` (a projet-alpha test asset): 13 Kimodo clips baked in, loaded
by `GLTFLoader` and cycled through `SkeletalAnimator` in the `+ModelViewer` — rest pose, clip 4 and
clip 13 captured — with **0 `VUID-`, 0 `[Error]`**, the validation layer loaded (headless run, RTX
3070 Ti). **Status**: the owner looked at that version on screen the same day — "it looks good, except
that part of the chin moves with the thorax" — and the chin was fixed (§ 8.4). The joint positions
are still the first estimate, never corrected by hand; the eight GAME clips of § 8.6 were verified by
measurement and by a scripted run, not yet judged on screen.

The chain is five scripts beside the prototype (`dependencies/kimodo.cpp/tools/`, README § 5; same
warning as § 6: that checkout is unversioned).

### 8.1 Preparing the mesh

| | measured on `RevenantAlt.glb` |
|---|---|
| source | 1 028 441 vertices, **1 947 720 triangles**, ONE connected shell, 4 border and 2 non-manifold edges out of 2.92 M |
| frame | unit height (0.98), centred on the origin, **facing +X** (so does `Hugo.glb`, from the same generator) |
| prepared | 150 000 triangles, 116 341 vertices, turned to face +Z, feet on Y = 0 |
| fidelity | source vertex → prepared surface: mean 0.03 %, max **0.14 %** of the height — the sampling floor |

50 to 100 times the vertex count of every skinned mesh the engine had played (Paladin 9 805,
`Character.glb` 19 322): the rigged version is a decimated SECOND file, the original stays static.

⚠️ **A decimator must see the seams closed.** glTF stores a vertex once per (position, normal, UV):
every UV seam is a row of duplicated vertices, i.e. an open border. Decimated as imported, each
side of a seam collapses on its own and the surface tears along all of them. Import with the
duplicates merged and let the UVs ride on the face corners.

⚠️ **Never re-export the textures through the DCC.** Blender re-encodes a JPEG on export. The images
are copied byte for byte from the source file into the result.

### 8.2 Bind pose ≠ rest pose

§ 3's shortcut needs both skeletons in the same pose. A generated mesh stands however its generator
posed it — arms down at 13° from the vertical here, a 79° swing away from SOMA's T-pose. The full
treatment, written for a skeleton being BUILT rather than mapped:

```
G(j)      the BIND rotation of joint j: the global rotation that carries SOMA's rest direction of
          the bone leaving j onto the direction that bone has in the mesh
offset(j) = G(parent)ᵀ · (p(j) − p(parent))     the bone, in its parent's T frame
L_bind(j) = G(parent)ᵀ · G(j)                   the node's rest rotation — rest == bind
IBM(j)    = [G(j) | p(j)]⁻¹                     the mesh is bound where it stands
```

A Kimodo rotation is then the node's animated rotation, unchanged: at `q = identity` the skeleton
stands in SOMA's T-pose with this character's bone lengths; at `q = L_bind` it stands in the mesh.

- `G(j)` is a **swing** only (shortest arc), taken **relative to the parent** so that a bent elbow
  stays a pure bend instead of picking up a roll.
- The **hand** needs a second direction or its roll about the forearm is arbitrary: the thumb. On the
  Revenant the thumb moves the hand frame by 27.5° from the swing-only answer.
- The **head** has no child along its axis: it aims at the midpoint of the eyes.
- The hand bone follows the **palm** (wrist → knuckle), not the fingertip: curled claws would
  otherwise be read as a flexed wrist and straightened backwards.
- ⚠️ **The pitch of ankle → toe base IS the sole.** The foot's swing maps SOMA's flat foot (20.9°
  below the horizontal) onto that direction: a toe base placed higher or lower tilts every planted
  foot by the difference. The assembly prints both angles.

Proof, in § 5's order: skinning the mesh with the hierarchy as stored moves it by **6.6e-8** of its
height (rest == bind), read back from the file alone.

### 8.3 Root translation — an affine map, not a scale

X and Z scale by the leg-length ratio `s` (§ 3). The **height** does not: a character's standing hip
height is not `s` times the source's — boots, a long pelvis, a heroic build.

```
y_target = h_target + s · (y_source − h_source)
```

`h` is the hip height at which each skeleton, legs straight, has its feet flat on the ground
(Revenant: `s = 0.5034`, `h_source = 0.9886`, `h_target = 0.5531`; a pure scale would give 0.4977 and
sink the feet by 5.6 % of the mesh height). Standing maps to standing, a crouch keeps its depth in
leg lengths.

⚠️ **This probably explains most of § 4's Paladin residual, and that was NOT re-measured.** The
prototype scaled the height: `0.9558 − 0.9200 × 0.9887 = 0.046` — 4.6 cm of sinking at standing,
against the "2 to 5 cm" recorded there and attributed to the −13 % shin. Re-measure with the affine
map before sizing a ground-contact pass on that number.

### 8.4 Skin weights

Bone heat (Blender's automatic weights), then the model's own corrections, then the top-4.

⚠️⚠️ **Bone heat fails on a small mesh, in silence.** On the unit-height mesh (0.98 high, 74 803
welded vertices) it bound **0 vertex** and reported it only through a UI warning; scaled ×10 or
×100, same topology, nothing cleaned, it bound **all of them**. Solve on a scaled copy — weights
carry no length — and assert that something was bound.

What bone heat cannot know, each seen on the Revenant and fixed by a rule in the model's data file:

| artefact | cause | rule |
|---|---|---|
| the shoulder-mounted launchers bend like rubber | they sit above the clavicles, which take them: the two clavicles came out dominant on 11 378 and 11 765 welded vertices, more than any other joint | **rigid** region → `Chest` |
| the collar and the harness follow the head | the neck bones are the nearest ones | neck and head **confined** to the skull, the chin and the neck column |
| part of the chin moves with the thorax (owner, on screen) | the mandible hangs in front of the collar: bone heat gave part of the jaw to the clavicles and the chest (156 skull vertices under 0.95 on head and neck), and the first confinement box cut the chin at the wrong height | the whole skull, chin included, **rigid** → `Head` (a union of three boxes); 1 467 chin vertices now at 0.984 to 1.0 on `Head` |
| the thigh plates flare out like a skirt when the arms rise | the hands hang 2 cm from them in the bind pose | forearm and hand confined to the arm's side of the gap, and the leg excluded from the hand's |

⚠️ A confinement written as "the leg acts only inside this half-space" also cut the **boots**, wider
than the plane lower down: it took 1 240 units of weight off 1 570 vertices of one boot and gave them
to whatever else touched those vertices — the hand, for the ones left with nothing. Such a rule is an
EXCLUSION bounded in height (3 units on 198 vertices once written that way), and its report (weight
moved, vertices touched) is read every time.

⚠️ **Draw a region on exact cross-sections, never on a front view.** The first head box was read off
a shaded front view, which took the tooth line for the chin: the jaw actually ends 4 % of the mesh
height lower, and it is a separate piece in FRONT of the neck column, fused to the collar on both
sides. Intersecting the triangles with a few sagittal planes showed all of that in one picture. A
skull is one bone: bone heat alone leaves a tenth of its weight on the neck joints (`Head` 0.893 of
1.0 on average), which bends the jaw whenever the head and the neck disagree.

Which SOMA joints carry skin at all — measured over 13 clips, the largest rotation each joint takes:

| joints | max rotation | decision |
|---|---|---|
| `Jaw` | 0.8° | no skin: it never moves |
| `LeftEye` / `RightEye` | 13° | no skin on a face with no separate eyeballs |
| `HandThumbEnd` / `HandMiddleEnd` | 42° / **90°** | no skin: they are END joints, a vertex bound to one turns about the fingertip |

So the hands and the skull are rigid. The "End" joints do rotate — they are not constant padding.

### 8.5 What the result measures

Read from the produced file by an independent evaluator (model units; the mesh is 0.98 high):

| | |
|---|---|
| lowest point of the skinned mesh, idle | +0.003 to +0.004 — it floats by 0.4 %, the source's own offset (§ 4 of the Kimodo doc: toes never below +1.8 cm) |
| lowest point, walk | −0.009 to +0.010 — the sole of a boot rolling, no contact pass |
| foot slide per FLAT stance, walk | mean 0.0054, max 0.0125 |
| the same stances, target against source × `s` | target mean **0.0027**, source 0.0049: the rig adds no slide |

⚠️ **A stance detector decides the number.** "The ankle is low" takes the roll of the foot for a
stance and reported a mean of 4 % and a maximum of 19 % of the height on that same walk. Require the
ankle AND the toe base near their rest clearance (§ 5, step 4).

### 8.6 Game clips — in place, and looping where they must

A Kimodo motion is a TAKE: the character travels, a walk starts and stops wherever the take does,
nothing loops. An actor moved by the game needs clips that stay on the spot, or the two displacements
stack and the model jumps back at every loop — and `LoaderOptions::stripRootMotion` does not act on
glTF ([`todo/root-motion-mode.md`](todo/root-motion-mode.md)). The clips are therefore prepared in
the tooling, ON THE SOMA MOTION, which makes them generic: any character rigged on SOMA-30 bakes the
same set (`tools/soma_clips.py`, a `clips.json` manifest beside the takes, `--clips` of the assembly).

| mode | what it does |
|---|---|
| `once` | the take, cut to a frame range, standing on the origin at its first frame; the motion RELATIVE to that start is kept (a stagger still steps back, a body falls where it falls) |
| `loop` | one cycle — the pair of frames whose poses AND velocities match best — closed exactly: the residual pose difference spread over the cycle, the travel removed as a straight drift (the sway around it stays), the last key a copy of the first |

The cycle's own travel over its duration is the clip's **natural speed**: the speed at which the feet
of an actor moved by the game do not slide. The first consumer walks and runs at exactly those.

| clip | take → clip | measured |
|---|---|---|
| `idle` | frames 28..119 of 150 | 3.03 s, seam mismatch 1.3° before closure |
| `walk` | frames 140..175 of 210 | one gait cycle, 1.17 s, seam 1.0°, **1.291 m/s** on the SOMA body |
| `run` | frames 95..117 of 180 | one gait cycle, 0.73 s, seam 2.4°, **2.462 m/s** |
| `fire` | frames 4..70 of 75, amplitude 0.27, speed 1.5 | 1.47 s; the chest leans 20.6° and holds above 92 % of it from key 17 to key 23: a projectile leaves at 17 |
| `strike_1` (2026-11-22) | frames 0..54 of 60 | 1.8 s; a fist that CRUSHES something: on the 2.40 m character the right fist goes back and up to 2.36 m (frame 9), the right foot steps 0.2 m into the blow, the fist comes down at 8.5 m/s to 0.82 m, 0.5 m ahead (frame 19), the head bowing from 2.21 to 1.47 m and the hips shifting 0.35 m; the body is within 10° of its first pose at frame 50. The event `strike` at key 13 (26 logic cycles), when the fist passes 1.5 m. Prompt "angrily smashes something on the ground in front with the right fist, one hard blow, then stands up straight", seed 44. ⚠️ **Chosen by the owner, BY EYE, in the kimodo.cpp demo viewer** — after FOUR takes picked on measures were each rejected on screen, each for what the measures of the moment did not look at. 1: a "hammer strike" kept for its reach carried the hand to 2.40 m and brought it back at 2.2 m/s — a blow ABOVE a 1.75 m target. 2: a fist slammed from above the head — the OTHER hand travelled 2.86 m, the chest turned 85°: "a dance move". 3: "pounds a table once", calm, amplitude 1.5 — the fist stopped at 1.31 m with the arm held out: "he is handing something over". 4: "punches downward toward the ground", first of a ranking that weighed all of that — a deep bow, the fist put on the ground, a slow rise: "he is picking something up". The take kept was SIXTH of that ranking's real blows: it counted the step and the shift of the hips against it, and they are what makes it a blow. **A ranking finds the takes worth looking at; a person chooses.** Show the takes: `tools`-free, the demo viewer reads a gallery directory (`<id>.json` + `<id>/local_rotations_xyzw.f32`, `root_positions.f32`; `./build/release/kimodo-demo -addr 127.0.0.1:8094 -generator build/release/kmd-generate -output <gallery>`, the list newest first). And before any take, ask the INTENTION in one sentence ("he wants to crush something with his fist"): it names the prompt; "a blow in the manner of a robot" did not |
| `strike_2` (2026-11-22) | frames 0..50 of 60 | 1.7 s; the ALTERNATIVE strike, chosen by the owner by eye like `strike_1` (the character draws one of the two per blow: two clips of one attack carry each its own event). Standing, feet planted (0.01 m), the head still: the take starts with the fists half raised (1.50 m), the right fist rises to 2.05 m (frame 6), is held there until frame 12, raps down at 5.8 m/s to 1.57 m, 0.58 m ahead (frame 15), springs back to 1.87 m, and the arms come down slowly. The event `strike` at key 14 (28 logic cycles), when the fist passes 1.75 m. Prompt "stands with feet planted and smashes downward with the right fist as if crushing a bug on a table", seed 42. The ranking called it a WEAK blow — the fist stops at 1.57 m: on a 1.75 m target that is its head |
| `hit_1`, `hit_2` | frames 0..44 of 60 | 1.47 s |
| `death_1`, `death_2` | frames 0..95 of 120 | 3.17 s, the body laid on the ground, its props sunk into it (below) |

Read back from the produced file: every loop ends on its first key (0.04° of float rounding, root
identical); nothing goes under the ground but the props of a dead body, on purpose (below).

⚠️ **A loop ends on a COPY of its first key, one frame period after the last distinct pose.** A glTF
clip lasts until its last key: without that copy the wrap skips one period every cycle.

⚠️ **Pick a take on a measurement, the prompt is not obeyed.** "A steady pace" gave a run that
accelerates to 3.5 m/s then decays to 1 m/s (best cycle: 7.2° of seam); of four takes the steadiest
closed at 2.4°. "Shoulders slightly hunched" produced an idle leaning 16 to 41° with the hands on the
hips. The chest's pitch (forward kinematics of four joints), the height of the hands and the drift
of the feet sorted the takes in seconds; the prompt, the seed and the frame count of every kept take
are recorded beside it (`generation.json`).

⚠️ **The model gives the SHAPE of a motion, not its measure — scale the take.** The firing clip took
two rounds. "Leans forward bracing" bent the chest 60°; the take kept at first stayed upright but
crossed its arms, and the owner read it at once: "it seems to hold a real bazooka — it should simply
stand facing forward and push its torso forward to present the launchers". Six takes asking for a
SLIGHT lean, arms at the sides, all bow by 68 to 85°; one of them has exactly the wanted shape (arms
hanging, feet planted, heading within 5°, back on its first pose). `amplitude: 0.27` scales every
joint's excursion from the first key — `(q · q0⁻¹)^0.27 · q0`, the root alike — and the bow becomes a
lean of 20.6°; `speed: 1.5` resamples it to 1.47 s. An arm that hangs vertical through a bow still
hangs vertical through the scaled lean: both rotations scale together.

⚠️ **A body lying down, and what it carries.** SOMA's hips come within 6 cm of the floor; mapped
onto a character with launchers and a backpack, the mesh went 15 % of its height under the ground.
The `ground: lift` pass skins every key, measures the lowest vertex and raises the root by the
depth, widened then smoothed over nine keys so it never falls short. Done on the WHOLE mesh, the body
rests on its props and floats on them — the owner, on screen: "the body floats above the ground,
because of the size of the launchers" — and he chose the simple answer over a re-posed body: **the
props sink**. A rigid rule of the model marked `sinks_into_ground` (the launchers, the backpack) is
left out of the search: the body ends 0 to 0.5 % of its height from the ground, its props 12 to 13 %
under it. With the launchers alone marked, a body on its back still floated 5 % on the backpack.

### 8.7 Not done

- The correction of the joints by hand, and a way to paint weights by hand (the chain recomputes them).
- The owner's judgement of the idle, the walk, the run and the two reactions on screen (the fire and the deaths were
  judged and redone, § 8.6).
- A ground-contact pass; the 0.4 % float is removable by calibrating `h_source` on a standing clip.
- Any GPU measurement of a 116 341-vertex skinned mesh (shadow passes, the ray-tracing mirror pass).
- Secondary motion: cables, chains and armour plates are skin.

## Link Index

- [`todo/skeletal-animation-retargeting.md`](todo/skeletal-animation-retargeting.md) — the open work
- [`todo/root-motion-mode.md`](todo/root-motion-mode.md) — who consumes the root translation
- [`text-to-motion-kimodo.md`](text-to-motion-kimodo.md) — the AI motion source this was proven against
- [`coordinate-system.md`](coordinate-system.md) — the engine frame
- [`../src/Animations/AGENTS.md`](../src/Animations/AGENTS.md) — the animation subsystem
