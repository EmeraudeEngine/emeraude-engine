## Important Files

### Resource Layer (src/Animations/)
- `SkeletonResource.hpp/.cpp` — Managed resource wrapping `Skeleton<float>`
- `AnimationClipResource.hpp/.cpp` — Managed resource wrapping `AnimationClip<float>`
- `SkeletalAnimator.hpp/.cpp` — Per-instance evaluator (sampling, FK, skinning matrices)
- `PlaybackWrap.hpp` — The wrap enum, shared by both evaluators
- `Scenes/Component/NodeAnimation.hpp/.cpp` — The node-hierarchy evaluator

### Data Types (Libs/Animation/)
- `Base/Animation/Joint.hpp` — Joint struct
- `Base/Animation/Skeleton.hpp` — Joint collection with validation
- `Base/Animation/AnimationChannel.hpp` — Keyframe channels
- `Base/Animation/AnimationClip.hpp` — Named clip (collection of channels)
- `Base/Animation/Skin.hpp` — Mesh-to-skeleton binding

### Vertex Factory Integration
- `Base/VertexFactory/ShapeLoadResult.hpp` — Bundles Shape + optional Skeleton + optional Skin
- `Base/VertexFactory/FileFormatInterface.hpp` — `readStream()` uses `ShapeLoadResult`

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
- `Base/Math/TransformUtils.hpp` — `composeTRS()` / `decomposeTRS()` for joint transforms
- `Base/Math/Quaternion.hpp` — `slerp()` for rotation interpolation, `toRotationMatrix4()` for skeleton matrix computation

### Tests
- `Testing/test_MathTransformConversions.cpp` — 52 tests: Quat↔Mat4, compose/decompose roundtrips
