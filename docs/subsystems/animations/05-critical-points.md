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
