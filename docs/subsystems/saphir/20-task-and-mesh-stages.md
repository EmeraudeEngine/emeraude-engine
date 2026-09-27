## Task and mesh stages (optional, Sep 2026)

`ShaderType::TaskShader` / `ShaderType::MeshShader` (appended after `ComputeShader`), classes `TaskShader`
and `MeshShader`, `Program::initTaskShader()` / `initMeshShader()`. A mesh-shading program has NO vertex,
tesselation or geometry stage (`Program::isComplete()` enforces it) and its last stage before the fragment
is the mesh one. Both stages enable `GL_EXT_mesh_shader` themselves; the task payload is a verbatim
`taskPayloadSharedEXT` declaration given to BOTH stages (`setTaskPayload()`). Only usable when
`Vulkan::Device::meshShadersEnabled()` (never on MoltenVK today).

**`MeshShader` IS an `AbstractVertexStage`** (2026-09-22, step 2 of engine item
`mesh-shader-displaced-surface`): the material, light and shadow generators address it exactly as a vertex
shader and every synthetic variable comes from the SAME code (§ The per-vertex stage contract). Its
`onSourceCodeGeneration()` only builds the frame around that code:

- `SetMeshOutputsEXT(V, P)`, then a loop over the workgroup's vertices (`MeshShader::VertexIndex` =
  `msVertexIndex`, strided by the workgroup size);
- each iteration runs the **vertex source**'s prologue (`setVertexSource(count, prologue, providedAttributes)`),
  which defines the vertex attributes as LOCALS of their attribute names (`vaPosition`, `vaNormal`...): an
  attribute a vertex shader would fetch `in` is PROVIDED here. One the synthesis needs and the source does not
  provide fails the generation, naming it;
- the synthesis writes its outputs into locals of their canonical names; the iteration ends by copying them
  into per-vertex arrays declared at the **same locations** (`<name>MS[]`, Saphir `StageOutput` with array
  size -1), and the light pass `OutputBlock` through a local `Structure` of the same members into the block
  array (`<instance>MS[]`). Vulkan links stages by location, so
  `FragmentShader::connectFromPreviousShader(const MeshShader &)` shares ONE body with the vertex overload
  (`connectFromPerVertexStage()`);
- the clip position is the local `msPosition` (`AbstractVertexStage::setPositionOutput()`, `gl_Position` for a
  vertex shader), copied into `gl_MeshVerticesEXT[i].gl_Position`;
- the **primitive source** (`setPrimitiveSource(count, code)`) runs once per primitive (`msPrimitiveIndex`);
- the loops close in `AbstractShader::setMainEpilogue()`, emitted after every output instruction.

The two other divergence points are DATA set by the concrete stage, never virtuals (owner: overloads, not
virtuals): the clip-position variable and the InstanceTransforms index (`setInstanceIndexExpression()`,
`gl_InstanceIndex` for a vertex shader — a mesh workgroup has none). Refused in a mesh stage: MDI, instancing
attributes, billboards, per-primitive outputs. The vertex path was re-proven byte-identical after this step
(410 generated sources). No generator builds a mesh program yet (step 3).
⚠️⚠️ **glslang's mesh limits must be set** (`ShaderManager.cpp`, `maxMeshOutputVerticesEXT` and siblings):
left at zero, glslang rejects every mesh shader. They hold glslang's reference values; the device's real
ceilings are `PhysicalDevice::meshShaderProperties()`. Verified 2026-09-22 by compiling a task + mesh pair
through `ShaderManager::getShaderModuleFromSourceCode()`: both modules created, 0 VUID (after `maintenance4`,
see `src/Vulkan/AGENTS.md` § Mesh Shader Support). ⚠️ `ShaderManager::getShaderModules()` returns at most 5
modules — enough for task + mesh + fragment.
