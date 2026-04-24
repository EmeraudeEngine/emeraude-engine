# Animations System

Context for developing the Emeraude Engine animations system.

## Module Overview

**Status: GPU SKINNING IMPLEMENTED** — Full CPU→GPU skeletal animation pipeline in place. Data types stabilized in `Libs/Animation/`, managed resources in `src/Animations/`, GPU skinning via vertex shader with SSBO bone matrices.

## Animations-Specific Rules

### System Scope
- **Skeletal animations**: Skinning, mesh deformation via skeleton (IMPLEMENTED)
- **Animation interfaces**: Possibility to animate values via interfaces (property animation system)
- **NOT transform animations**: Use scene graph for position/rotation/scale animation

### Architecture

The animation system is split into three layers:

**Data layer (`Libs/Animation/`)** — Stabilized, header-only:
| Type | File | Purpose |
|------|------|---------|
| `Joint` | `Libs/Animation/Joint.hpp` | Joint struct: name, parentIndex, local T/R/S, inverseBindMatrix |
| `Skeleton` | `Libs/Animation/Skeleton.hpp` | Ordered joint collection, name lookup, hierarchy validation |
| `AnimationChannel` | `Libs/Animation/AnimationChannel.hpp` | Per-joint keyframes (VectorKeyFrame for T/S, QuaternionKeyFrame for R), 3 interpolation modes (Step, Linear, CubicSpline) |
| `AnimationClip` | `Libs/Animation/AnimationClip.hpp` | Named channel collection, duration auto-computed, skeleton-independent |
| `Skin` | `Libs/Animation/Skin.hpp` | Mesh-to-skeleton binding: joint index remapping, inverse bind matrices, GLTF JOINTS_0 indirection |

**Resource layer (`src/Animations/`)** — Managed engine resources:
| Type | File | Purpose |
|------|------|---------|
| `SkeletonResource` | `SkeletonResource.hpp/.cpp` | Wraps `Skeleton<float>` as a named, shared resource via `Container<SkeletonResource>` |
| `AnimationClipResource` | `AnimationClipResource.hpp/.cpp` | Wraps `AnimationClip<float>` as a named, shared resource via `Container<AnimationClipResource>` |

**Runtime layer (`src/Animations/`)** — Per-instance animation evaluation:
| Type | File | Purpose |
|------|------|---------|
| `SkeletalAnimator` | `SkeletalAnimator.hpp/.cpp` | Per-instance evaluator: keyframe sampling, FK, skinning matrix computation |

**Property animation (existing, separate):**
| Type | File | Purpose |
|------|------|---------|
| `AnimationInterface` | `AnimationInterface.hpp` | Base interface for all value animations |
| `AnimatableInterface` | `AnimatableInterface.hpp/.cpp` | Mixin for objects that can be animated (map of animations, update loop) |
| `Sequence` | `Sequence.hpp/.cpp` | Keyframe-based value interpolation (Linear, Cosine) |
| `ConstantValue` | `ConstantValue.hpp` | Always returns the same value |
| `RandomValue` | `RandomValue.hpp/.cpp` | Random value within range |

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

### Loader Integration

**Shape no longer carries skeletal data.** The `ShapeLoadResult<V,I>` struct bundles `Shape` + `optional<Skeleton>` + `optional<Skin>`. All file format interfaces (`FileFormatInterface::readStream()`) use this struct.

**GLTF** (`AssetLoaders/GLTFLoader.cpp`) and **FBX** (`AssetLoaders/FBXLoader.cpp`):
- `loadSkins()` builds Skeleton, creates `SkeletonResource` via resource manager, stores `Skin` per skin index
- `loadAnimations()` reads channels/samplers (glTF) or resamples `anim_stack` at 30 Hz (FBX), creates `AnimationClipResource` via resource manager
- After loading: attaches skeletal data to renderables via `SkeletalDataTrait::setSkeletalData()`
- Pipeline order: Images → Materials → Meshes → Skins → Animations → **Attach skeletal data** → Nodes
- Bone influences detected from vertex data: `shape->vertices()[0].influences()[0] >= 0` sets `EnableInfluence | EnableWeight` geometry flags

**FBX split-animation workflow** — `FBXLoader::loadAnimationClipsOnly(path, skeleton, output)` resamples a standalone animation FBX against an externally-loaded skeleton, resolving bones by **joint name**. Used for Mixamo / Maya / Blender per-action exports where the rig and each animation live in separate files. See `AssetLoaders/AGENTS.md` for the full recipe.

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

### Remaining Work
- CubicSpline keyframe interpolation (data parsed from glTF but falls back to linear)
- Animation blending (crossfade, layered, additive)
- Animation state machine / controller
- Timeline system (multi-track orchestration for cutscenes)

## Development Commands

```bash
# Animation data type tests (in Testing/)
ctest -R MathTransformConversions
./test --filter="*TransformConversion*"
```

## Important Files

### Resource Layer (src/Animations/)
- `SkeletonResource.hpp/.cpp` — Managed resource wrapping `Skeleton<float>`
- `AnimationClipResource.hpp/.cpp` — Managed resource wrapping `AnimationClip<float>`
- `SkeletalAnimator.hpp/.cpp` — Per-instance evaluator (sampling, FK, skinning matrices)

### Data Types (Libs/Animation/)
- `Libs/Animation/Joint.hpp` — Joint struct
- `Libs/Animation/Skeleton.hpp` — Joint collection with validation
- `Libs/Animation/AnimationChannel.hpp` — Keyframe channels
- `Libs/Animation/AnimationClip.hpp` — Named clip (collection of channels)
- `Libs/Animation/Skin.hpp` — Mesh-to-skeleton binding

### Vertex Factory Integration
- `Libs/VertexFactory/ShapeLoadResult.hpp` — Bundles Shape + optional Skeleton + optional Skin
- `Libs/VertexFactory/FileFormatInterface.hpp` — `readStream()` uses `ShapeLoadResult`

### Renderable Integration
- `Graphics/Renderable/SkeletalDataTrait.hpp` — Trait on MeshResource/SimpleMeshResource
- `Graphics/Renderable/MeshResource.hpp` — Inherits SkeletalDataTrait
- `Graphics/Renderable/SimpleMeshResource.hpp` — Inherits SkeletalDataTrait
- `Graphics/Renderable/ProgramCacheKey.hpp` — `isSkeletalAnimationEnabled` field

### GPU Pipeline
- `Graphics/RenderableInstance/Abstract.hpp/.cpp` — `createSkinningResources()`, `updateSkinningMatrices()`, SSBO binding in render paths
- `Graphics/VertexBufferFormatManager.cpp` — BoneInfluence/BoneWeight declare/jump logic
- `Graphics/Types.hpp` — `VertexAttributeType::BoneInfluence` (18), `BoneWeight` (19)

### Shader Generation
- `Saphir/VertexShader.hpp/.cpp` — `enableSkinning()`, skinned position/normal/tangent substitution
- `Saphir/Generator/Abstract.hpp` — `IsSkeletalAnimationEnabled` flag
- `Saphir/Generator/SceneRendering.cpp` — PerModel set, bone attributes, SSBO declaration
- `Saphir/Generator/ShadowCasting.cpp` — Same for shadow passes
- `Saphir/Generator/SkinningLayoutHelper.hpp` — Shared SSBO descriptor set layout cache
- `Saphir/Keys.hpp` — `Attribute::BoneInfluence`, `Attribute::BoneWeight`
- `Saphir/Declaration/InputAttribute.cpp` — BoneInfluence/BoneWeight type and name mapping

### Component Integration
- `Scenes/Component/Visual.hpp/.cpp` — Owns `SkeletalAnimator`, lazy init from SkeletalDataTrait, per-frame SSBO upload
- `Scenes/GLTFLoader.hpp/.cpp` — Creates SkeletonResource/AnimationClipResource, populates SkeletalDataTrait

### Math Support
- `Libs/Math/TransformUtils.hpp` — `composeTRS()` / `decomposeTRS()` for joint transforms
- `Libs/Math/Quaternion.hpp` — `slerp()` for rotation interpolation, `toRotationMatrix4()` for skeleton matrix computation

### Tests
- `Testing/test_MathTransformConversions.cpp` — 52 tests: Quat↔Mat4, compose/decompose roundtrips

## Critical Points

- **Rotation matrix convention**: Use `Quaternion::toRotationMatrix4()` (standard column-major), NOT `rotationMatrix()` (row-major data in column-major storage). See `Libs/AGENTS.md` for details.
- **Descriptor set ordering**: PerModel (skinning SSBO) MUST be between PerLight and PerModelLayer in BOTH `prepareUniformSets()` and render-time binding. Mismatch causes Vulkan validation errors.
- **StaticVector capacity**: Descriptor set layout vectors are `StaticVector<5>` (was 4 before PerModel). Changed across entire codebase including Vulkan layer.
- **Bone indices as floats**: VBO stores int32→float. Shader does `ivec4(vaBoneInfluence)` to recover indices.
- **Shape has NO skeletal data**: Skeleton/Skin removed from Shape. Use `ShapeLoadResult` for loading, `SkeletalDataTrait` for renderables.
- **SSBO array declaration**: Use `addMember(VariableType::Matrix4, "bones[]")` with brackets in the name. `addArrayMember(..., 0)` silently fails (0 means "not an array" in AbstractBufferBackedBlock).
- **SSBO access qualifier**: Skinning SSBO must use `ssbo.setAccessQualifier(Declaration::AccessQualifier::ReadOnly)` — omitting this causes `VUID-RuntimeSpirv-NonWritable-06341` on GPUs without `vertexPipelineStoresAndAtomics`.
- **Skinning code timing in VertexShader**: Skinning code must be emitted AFTER `generateMainUniqueInstructions()` (which populates `m_vertexAttributes`), then PREPENDED to `topInstructions` via `insert(0, ...)`. Emitting before causes missing attribute checks.
- **Conditional normal/tangent/binormal skinning**: Only emit `skinnedNormal`/`skinnedTangent`/`skinnedBinormal` when the corresponding vertex attribute is declared. Shadow shaders don't declare normals.

## Known Issues

- **TBN debug rendering**: TBNSpaceRendering generator does not support skeletal animation — TBN lines may not appear or may appear at bind-pose positions.
- **MD5 normal map convention**: id Tech normal maps use OpenGL convention (Y+ up). The engine uses DirectX convention (Y+ down). Use `"FlipNormalMapY": true` in the material JSON normal component to flip the green channel at load time. See: `Pixmap::flipNormalMapY()`, `TextureResource::Abstract::enableFlipNormalMapY()`, `Material::Component::Texture` (JSON key `"FlipNormalMapY"`).

### SkeletalAnimator Runtime API

The `SkeletalAnimator` provides runtime clip management:
- `clipNames()` — Returns sorted list of all registered clip names
- `activeClipName()` — Returns the name of the currently playing clip
- `play(clipName, wrap)` — Starts a clip by name (default: `PlaybackWrap::Loop`)
- `stop()` / `pause()` / `resume()` — Playback control
- `setSpeed(float)` — Playback speed multiplier

Code references: `Animations/SkeletalAnimator.hpp/.cpp`

## Detailed Documentation

- **Libs/Animation data types**: See [`src/Libs/AGENTS.md`](../Libs/AGENTS.md)
- **Math/TransformUtils**: See [`src/Libs/AGENTS.md`](../Libs/AGENTS.md) (Math section)
- **GLTF loading pipeline**: See [`src/Scenes/AGENTS.md`](../Scenes/AGENTS.md)
- **Graphics pipeline**: See [`src/Graphics/AGENTS.md`](../Graphics/AGENTS.md)
- **Shader generation**: See [`src/Saphir/AGENTS.md`](../Saphir/AGENTS.md)
