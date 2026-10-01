## Critical Points

- **Rotation matrix convention**: Use `Quaternion::toRotationMatrix4()` (standard column-major), NOT `rotationMatrix()` (row-major data in column-major storage). See `Base/AGENTS.md` for details.
- **Descriptor set ordering**: PerModel (skinning SSBO) MUST be between PerLight and PerModelLayer in BOTH `prepareUniformSets()` and render-time binding. Mismatch causes Vulkan validation errors.
- **StaticVector capacity**: Descriptor set layout vectors are `StaticVector<5>` (was 4 before PerModel). Changed across entire codebase including Vulkan layer.
- **Bone indices as floats**: VBO stores int32→float. Shader does `ivec4(vaBoneInfluence)` to recover indices.
- **Shape has NO skeletal data**: Skeleton/Skin removed from Shape. Use `ShapeLoadResult` for loading, `SkeletalDataTrait` for renderables.
- **SSBO array declaration**: Use `addMember(VariableType::Matrix4, "bones[]")` with brackets in the name. `addArrayMember(..., 0)` silently fails (0 means "not an array" in AbstractBufferBackedBlock).
- **SSBO access qualifier**: Skinning SSBO must use `ssbo.setAccessQualifier(Declaration::AccessQualifier::ReadOnly)` — omitting this causes `VUID-RuntimeSpirv-NonWritable-06341` on GPUs without `vertexPipelineStoresAndAtomics`.
- **Skinning code timing in VertexShader**: Skinning code must be emitted AFTER `generateMainUniqueInstructions()` (which populates `m_vertexAttributes`), then PREPENDED to `topInstructions` via `insert(0, ...)`. Emitting before causes missing attribute checks.
- **Conditional normal/tangent/binormal skinning**: Only emit `skinnedNormal`/`skinnedTangent`/`skinnedBinormal` when the corresponding vertex attribute is declared. Shadow shaders don't declare normals.
- **The skeleton is PARENTS FIRST**: `SkeletalAnimator::computeWorldMatrices()` is one forward pass. `setSkeleton()`
  refuses a skeleton that fails `Skeleton::isValid()`; `setSkin()` (and `setSkeleton()` for a skin set before) refuses
  a skin with a joint index outside the skeleton or an inverse bind matrix count different from its joint count. The
  glTF loader reorders an out-of-order skin (glTF allows any order): scenes-loaders `06-critical-rules.md`.
- **`RandomValue` draws composites per component** (vectors, color with alpha, cartesian frame position); the base
  `quickRandom()` for integers is always inside [min, max] since 2026-10-01 (a signed 8 / 16-bit draw went negative).
- **NaN positions / health are refused**: `Sequence::addKeyFrame(float)` / `setCurrentTime(float)` (a NaN converted to
  `uint32_t` is UB), `LampFlicker::setHealth()`; the `LampFlicker` constructor and its static helpers take a NaN
  health as 1 (a new lamp).
