## Animations-Specific Rules

### System Scope
- **Skeletal animations**: Skinning, mesh deformation via skeleton (IMPLEMENTED)
- **Animation interfaces**: Possibility to animate values via interfaces (property animation system)
- **NOT transform animations**: Use scene graph for position/rotation/scale animation

### Architecture

The animation system is split into three layers:

**Data layer (`Base/Animation/`)** — Stabilized, header-only:
| Type | File | Purpose |
|------|------|---------|
| `Joint` | `Base/Animation/Joint.hpp` | Joint struct: name, parentIndex, local T/R/S, inverseBindMatrix |
| `Skeleton` | `Base/Animation/Skeleton.hpp` | Ordered joint collection, name lookup, hierarchy validation |
| `AnimationChannel` | `Base/Animation/AnimationChannel.hpp` | Keyframes for ONE target (VectorKeyFrame for T/S, QuaternionKeyFrame for R), 3 interpolation modes (Step, Linear, CubicSpline), and the sampling itself: `sampleVector(t)` / `sampleQuaternion(t)` |
| `AnimationClip` | `Base/Animation/AnimationClip.hpp` | Named channel collection, duration auto-computed, skeleton-independent |
| `Skin` | `Base/Animation/Skin.hpp` | Mesh-to-skeleton binding: joint index remapping, inverse bind matrices, GLTF JOINTS_0 indirection |

**Resource layer (`src/Animations/`)** — Managed engine resources:
| Type | File | Purpose |
|------|------|---------|
| `SkeletonResource` | `SkeletonResource.hpp/.cpp` | Wraps `Skeleton<float>` as a named, shared resource via `Container<SkeletonResource>` |
| `AnimationClipResource` | `AnimationClipResource.hpp/.cpp` | Wraps `AnimationClip<float>` as a named, shared resource via `Container<AnimationClipResource>` |

**Runtime layer (`src/Animations/`)** — Per-instance animation evaluation:
| Type | File | Purpose |
|------|------|---------|
| `SkeletalAnimator` | `SkeletalAnimator.hpp/.cpp` | Per-instance evaluator: keyframe sampling, FK, skinning matrix computation |
| `PlaybackWrap` | `PlaybackWrap.hpp` | The wrap enum (Once/Loop/PingPong), SHARED by both clip evaluators |
| `Scenes::Component::NodeAnimation` | `Scenes/Component/NodeAnimation.hpp/.cpp` | The **second** clip evaluator: moves NODES, not joints (see below) |

**Property animation (existing, separate):**
| Type | File | Purpose |
|------|------|---------|
| `AnimationInterface` | `AnimationInterface.hpp` | Base interface for all value animations |
| `AnimatableInterface` | `AnimatableInterface.hpp/.cpp` | Mixin for objects that can be animated (map of animations, update loop) |
| `Sequence` | `Sequence.hpp/.cpp` | Keyframe-based value interpolation (Linear, Cosine) |
| `ConstantValue` | `ConstantValue.hpp` | Always returns the same value |
| `RandomValue` | `RandomValue.hpp/.cpp` | Random value within range |
| `LampFlicker` | `LampFlicker.hpp/.cpp` | An ailing lamp: sag, breathing and dropout bursts, driven by a single `health` |
| `FlameFlicker` | `FlameFlicker.hpp/.cpp` | An open flame: buoyant puffing at a frequency set by its base diameter, turbulence, gusts |

### LampFlicker — why `RandomValue` is not enough (Aug 2026)

`RandomValue` draws a fresh uniform EVERY cycle, which is white noise at the logic rate: attached
to a light it strobes. A failing lamp is **correlated** — steady most of the time, then a burst of
brutal cuts. `LampFlicker` is a small state machine producing that, from one continuous parameter:

- `health` 1 → perfectly steady (it short-circuits and returns the nominal value, so attaching it
  to a healthy lamp costs a multiply), 0 → dead.
- **Sag**: the mean output drops as the health does, and the lamp *breathes* around it through two
  **incommensurate** oscillators (3.1 and 7.7 Hz) plus a little grain. Incommensurate on purpose —
  their sum never repeats, so the sag cannot be read as a loop the way a `Sequence` would.
- **Dropouts**: 2 to 12 cycles (~30-200 ms) at ~4% output, never a hard zero (a filament keeps a
  dull glow through a brief cut, and a true zero reads as the light being deleted). The per-cycle
  odds are QUADRATIC in the sag, and a dropout opens a burst window that multiplies them — which
  is what groups the cuts into the two-or-three stutter of a real bad contact.

⚠️ **It drives the INTENSITY only, and that is a design constraint, not an omission.**
`AnimatableInterface::updateAnimations()` calls `getNextValue()` once per REGISTERED animation, so
two channels fed from one generator would advance it twice per cycle and desynchronise. The colour
of a dying lamp tracks the average state of its supply — minutes — not the individual flickers, so
it is a pure function of the health: `LampFlicker::colorForHealth()`, applied once with
`setColor()` when the health changes. Physically right and structurally simpler.

The value it emits is a luminous intensity in CANDELA, matching the `Intensity` animation id:

```cpp
/* ⚠️ Since 2026-09-25 a light colour is a unit-luminance chromaticity: colorForHealth() only REDDENS the lamp, and
 * the dimming its cooler colour used to cost goes to the intensity through luminanceForHealth(). */
const auto candela = nominalCandela * LampFlicker::luminanceForHealth(healthyColor, 0.5F);

light->addAnimation(Component::SpotLight::Intensity, std::make_shared< LampFlicker >(candela, 0.5F));
light->setColor(LampFlicker::colorForHealth(healthyColor, 0.5F));
```

### FlameFlicker — a flame is not a failing lamp (2026-09-28)

`LampFlicker` models a bad contact: steady, then cuts. An open flame never cuts; it BREATHES, at a
rhythm fixed by its size. `FlameFlicker (nominalCandela, baseDiameter, seed)` emits a candela value
(the `Intensity` animation id) = nominal × a level in [0.2, 1.5]:

- **Puffing**: a buoyant diffusion flame oscillates at f ≈ 1.5 / √D Hz, D the base diameter in metres
  (Cetegen & Ahmed, *Experiments on the periodic instability of buoyant plumes and pool fires*,
  Combustion and Flame 93, 1993) — a torch (0.08 m) ~5.3 Hz, a brazier (0.4 m) ~2.4 Hz, a bonfire
  (2 m) ~1 Hz. `puffingFrequency()` exposes it. Amplitude 10 %; the phase DIFFUSES (Wiener increment),
  so two identical fires drift apart and the puffing never reads as a sine.
- **Turbulence**: an Ornstein-Uhlenbeck process (8 %, τ 0.15 s), discretised exactly (no accumulation
  of a first-order error at a variable time step).
- **Gusts**: rare (0.08 / s) half-sine dips of 25-50 % over 0.4-1.2 s — a draught.
- The seed makes each fire independent and reproducible (`Base::Randomizer`, Box-Muller gaussians).

Cost: measured free — 18 animated lights in `citadel`, 12.3 ms per frame with it vs 12.9 without (noise).

### Complete Data Flow
```
Fichier glTF / MD5
  │
  ▼ Loaders
GLTFLoader / FileFormatMDx
  ├──→ SkeletonResource    (Container, shared via resource manager)
  ├──→ AnimationClipResource (Container, shared via resource manager)
  ├──→ MeshResource + SkeletalDataTrait (skeleton ref, skin value, clips refs)
  └──→ IndexedVertexResource (VBO with bone indices + weights via Weighted4)
  │
  ▼ Per-instance
Component::Visual
  ├── Detects SkeletalDataTrait on renderable (lazy init)
  ├── Creates SkeletalAnimator (setSkeleton, setSkin, addClip)
  ├── Creates skinning SSBO + descriptor set on RenderableInstance
  │   (one aligned section per frame in flight; DEDICATED device memory on portability-subset
  │    devices — MoltenVK residency, see Vulkan/AGENTS.md § Dedicated Device Memory)
  │
  ▼ Each frame
  ├── SkeletalAnimator.update(dt) → skinningMatrices[]
  ├── RenderableInstance.updateSkinningMatrices() → SSBO upload
  │
  ▼ GPU render
  ├── Saphir generates vertex shader with skinning code
  │   (bone attributes + SSBO declaration + skinMatrix computation)
  ├── PerModel descriptor set bound between PerLight and PerModelLayer
  └── Vertex shader applies skinMatrix to position/normal/tangent/binormal
```

### SkeletalAnimator — Dual Time Control

The `SkeletalAnimator` is agnostic about who controls time:
- **Direct mode** (`update(dt)`): Gameplay-driven, the animator advances its own clock. Use for character gameplay animations.
- **Timeline mode** (`evaluate(t)`): External timeline drives the time. Use for cutscenes, scripted sequences.

Both modes produce the same output: `skinningMatrices[]` ready for GPU upload.

**Playback modes**: `PlaybackWrap::Once`, `Loop`, `PingPong`

### ⚠️ TWO clip evaluators — a clip's target is NOT always a joint

An `AnimationClip` is **target-agnostic**: `AnimationChannel::targetIndex` (renamed from
`jointIndex`, Aug 2026) names *the animated thing inside the structure the clip belongs to*.
Which structure that is depends on the evaluator:

| Evaluator | Reads | `targetIndex` indexes | Writes |
|---|---|---|---|
| `Animations::SkeletalAnimator` | `SceneData::animationClips` | the skeleton's joint array | skinning matrices → SSBO |
| `Scenes::Component::NodeAnimation` | `SceneData::nodeAnimationClips` | `SceneData::nodes` | each node's local `CartesianFrame` |

**Why the second one exists**: glTF animates skin joints and plain nodes through the very same
construct. A rotating glass cover, a swinging door, a turning wheel are TRS tracks on nodes with no
skeleton anywhere — `ChronographWatch.glb` and `IridescentDishWithOlives.glb` are exactly that
(1 animation, **0 skins** each). The skeletal animator has nothing to do with them.

⚠️ **The two lists are kept apart at the CONTRACT** (`Scenes::Loaders::SceneData`), never sorted out
downstream by looking at indices: a node clip fed to the skeletal animator poses the wrong joints in
silence. A glTF animation touching both kinds is **SPLIT by the loader into two clips sharing one
name**, in two resource key spaces (`…/animation/<name>` and `…/node-animation/<name>`) so the cache
cannot serve one half where the other was asked for.

⚠️ `NodeAnimation` deliberately does **NOT** go through `Animations::AnimatableInterface`: that map
is keyed by animation ID — one animation per node, no clip selection — while one clip drives many
nodes. The component sits on the **root** of the imported hierarchy and holds `weak_ptr` targets.

⚠️ **An animated node must never be flattened.** `SceneDataConsumer` drops a node carrying no mesh
and an identity transform — which is precisely the shape of a pivot waiting to be rotated. Flattened,
the clip would drive the PARENT and swing the whole asset. The consumer collects the animated node
indices *before* walking and exempts them.

### ⚠️ "No animation" and "start on THIS clip" are stated on the CONTENT, not called on the animator

`Component::Visual` creates its `SkeletalAnimator` **lazily**, on its first logic cycle — long after
the scene was built. A caller building a scene therefore has *nothing to call `play()` or `stop()`
on*, and the historical default (auto-play clip 0) was unreachable to override. Both intents live on
`Graphics::Renderable::SkeletalDataTrait`:

- `enableAutoPlayFirstClip(false)` — the asset appears in its **bind pose**. This is what
  `Scenes::Viewers::ModelViewer` sets on every skeletal renderable it imports, so a dropped model
  opens at rest and the space bar walks its clips (engine `docs/ai-runtime-control.md` § 3).
- `setAutoPlayClipName("Walk")` — the asset appears **already playing** that clip (implies auto-play on).

⚠️ These mutate a **cached resource**: the flag survives for every later instance of the same asset.

### ⚠️ `stop()` RESTORES a pose, it does not drop one

Both evaluators had, or would have had, the same defect: the consumer only pushes a pose while the
evaluator reports one, and **nothing rewrites what was already staged**. Clearing left the last
animated frame frozen on screen.

- `SkeletalAnimator::stop()` **evaluates the bind pose** (`sampleBindPose()` + FK + skinning) so the
  GPU staging is overwritten with it.
- `NodeAnimation::stop()` writes every target's captured **rest frame** back.

Measured on `asset-loader`: the Paladin returns to an exact, motion-blur-free T-pose after cycling
through all 49 clips, and the watch's second hand snaps back to 12.

### Loader Integration

**Shape no longer carries skeletal data.** The `ShapeLoadResult<V,I>` struct bundles `Shape` + `optional<Skeleton>` + `optional<Skin>`. All file format interfaces (`FileFormatInterface::readStream()`) use this struct.

**GLTF** (`Scenes/Loaders/GLTFLoader.cpp`) and **FBX** (`Scenes/Loaders/FBXLoader.cpp`):
- `loadSkins()` builds Skeleton, creates `SkeletonResource` via resource manager, stores `Skin` per skin index
- `loadAnimations()` reads channels/samplers (glTF) or resamples `anim_stack` at 30 Hz (FBX), creates `AnimationClipResource` via resource manager
- After loading: attaches skeletal data to renderables via `SkeletalDataTrait::setSkeletalData()`
- Pipeline order: Images → Materials → Meshes → Skins → Animations → **Attach skeletal data** → Nodes
- Bone influences detected from vertex data: `shape->vertices()[0].influences()[0] >= 0` sets `EnableInfluence | EnableWeight` geometry flags

**FBX split-animation workflow** — `FBXLoader::loadAnimationClipsOnly(path, skeleton, output)` resamples a standalone animation FBX against an externally-loaded skeleton, resolving bones by **joint name**. Used for Mixamo / Maya / Blender per-action exports where the rig and each animation live in separate files. See `Scenes/Loaders/AGENTS.md` for the full recipe.

**MD5** (`VertexFactory/FileFormatMDx.hpp`):
- `loadMD5()` returns `ShapeLoadResult` with skeleton and skin alongside the shape
- Builds Skeleton (world→local transforms, inverse bind matrices), vertex influences/weights (top-4 by bias with renormalization)

### Resource System Integration

Both `SkeletonResource` and `AnimationClipResource` are managed resources:
- Registered in `Resources::Manager` under store `"Animations"`
- Type aliases: `Resources::Skeletons`, `Resources::AnimationClips`
- Created by loaders via `getOrCreateResourceSync()` with naming convention: `"prefix/skeleton/skinName"`, `"prefix/animation/clipName"`
- `Complexity::None` (no dependencies on other resources)

### GPU Skinning Pipeline

**Vertex data**: `VertexAttributeType::BoneInfluence` (location 18) and `BoneWeight` (location 19), both `vec4`. Bone indices stored as floats, cast to `ivec4` in shader.

**Descriptor sets**: `SetType::PerModel` (set index 2) used for bone matrix SSBO. Layout cached via `SkinningLayoutHelper::getSkinningDescriptorSetLayout()` through `LayoutManager`.

**Shader generation**: When `IsSkeletalAnimationEnabled` flag is set on the generator:
1. Input attributes `vaBoneInfluence` and `vaBoneWeight` declared
2. SSBO `SkinningMatrices` declared with `layout(std430) readonly buffer` and runtime-sized `mat4 bones[]` array
3. Skinning code injected in `Location::Top` of vertex shader: computes `skinMatrix`, `skinnedPosition`, `skinnedNormal` (+ tangent/binormal if tangent space enabled)
4. All position/normal synthesis methods use skinned variants instead of raw attributes

> [!CRITICAL]
> **The skinning SSBO MUST be declared `readonly`.** Without this qualifier, Vulkan considers the
> shader *may* write to the buffer, which requires the `vertexPipelineStoresAndAtomics` device
> feature. Many GPUs/drivers do not enable this feature by default, causing
> `VUID-RuntimeSpirv-NonWritable-06341` validation errors and pipeline creation failure.
> The bone matrices are CPU-written and GPU-read-only — `readonly` is both correct and required.

**Render-time binding**: SSBO descriptor set bound between PerLight and PerModelLayer in all three render paths:
- `castShadows()` — shadow passes
- `render()` — scene rendering (standard)
- `render()` with `RenderStateTracker` — scene rendering (optimized)

### Renderable Integration

**`SkeletalDataTrait`** (`Graphics/Renderable/SkeletalDataTrait.hpp`):
- Inherited by `MeshResource` and `SimpleMeshResource`
- Carries: `shared_ptr<SkeletonResource>`, `Skin<float>`, `vector<shared_ptr<AnimationClipResource>>`
- `hasSkeletalData()` returns true when skeleton is set
- `setSkeletalData(skel, skin, clips)` — full setter, called by GLTFLoader/FBXLoader after loading
- `setSkeletalData(skel, skin)` — sets skel + skin only, leaves clips untouched
- `addAnimationClips(clips)` — **appends** to the existing clip list (use for incremental loading from multiple sources, but mind the **order**: the runtime auto-plays index 0 at lazy init, see `Scenes::Component::Visual`)
- `setAnimationClips(clips)` — **replaces** the clip list (use when an external clip set should fully supersede whatever the loader attached, e.g. discarding a bind-pose clip embedded in the rig file in favor of split-animation clips)
- `enableAutoPlayFirstClip(bool)` / `setAutoPlayClipName(name)` / `isAutoPlayingFirstClip()` / `autoPlayClipName()` — what the asset shows when it appears; see the lazy-init section above

### Remaining Work
- **Retargeting** — playing a clip authored for ANOTHER skeleton. Absent today, which locks the
  engine out of every mocap library and every AI motion source. The mathematics, a measured worked
  example (Kimodo SOMA-30 onto the Mixamo Paladin) and the traps are in
  [`docs/animation-retargeting.md`](../../animation-retargeting.md); the open work is
  [`docs/todo/skeletal-animation-retargeting.md`](../../todo/skeletal-animation-retargeting.md).
  ⚠️ `LeftLeg` denotes the THIGH in one common skeleton and the SHIN in another — a by-name joint
  mapping, which `FBXLoader::loadAnimationClipsOnly()` already performs elsewhere, breaks the legs
  in silence.
- **Clip serialization** — `AnimationClipResource` loads from a path, a JSON value or memory, but
  nothing writes one back: a clip built at runtime does not survive the session.
- Animation blending (crossfade, layered, additive)
- Animation state machine / controller
- Timeline system (multi-track orchestration for cutscenes)
