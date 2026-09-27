## MDI Shader Generation

When `IsMultiDrawIndirectEnabled` is set on the generator, the shader system produces MDI-specific vertex shader variants:

### Push Constant Layout Change

MDI mode replaces the model matrix push constant with a BDA address pair:
```glsl
layout(push_constant) uniform Matrices {
    uint perDrawAddrLo;   // Low 32 bits of SSBO device address
    uint perDrawAddrHi;   // High 32 bits
    mat4 viewProjectionMatrix;
    float frameIndex;
} pcMatrices;
```

### BDA Buffer Reference Declaration

The vertex shader declares a `buffer_reference` struct matching `PerDrawData` (std430):
```glsl
layout(buffer_reference, std430) readonly buffer PerDrawDataRef {
    mat4 modelMatrix;
    uint frameIndex;
    uint _padding[3];
};
```

### Model Matrix Access via gl_DrawID

All synthesis methods (`synthesizeVertexPositionInWorldSpace`, `prepareModelViewMatrix`, `prepareModelViewProjectionMatrix`, `synthesizeVertexVectorInWorldSpace`) check `isMDIEnabled()` first:
```glsl
const uint64_t addr = packUint2x32(uvec2(pcMatrices.perDrawAddrLo, pcMatrices.perDrawAddrHi));
const mat4 svMDIModelMatrix = mat4(PerDrawDataRef(addr)[gl_DrawID].modelMatrix);
```

### Extension Registration Order (CRITICAL)

> [!WARNING]
> **Extensions MUST be registered in `VertexShader::enableMDI()`, NOT in `prepareMDIModelMatrix()` or
> `onSourceCodeGeneration()`.** The code generation flow is: `generateHeaders()` (emits `#extension`)
> → `onSourceCodeGeneration()` (emits BDA struct). Extensions registered after `generateHeaders()`
> appear AFTER the struct usage → `GL_EXT_buffer_reference: required extension not requested` error.

Required extensions:
- `GL_EXT_buffer_reference` — BDA buffer references
- `GL_EXT_buffer_reference2` — Array indexing on buffer references (`[gl_DrawID]`)
- `GL_ARB_gpu_shader_int64` — `uint64_t` type + `packUint2x32()`
- `GL_ARB_shader_draw_parameters` — `gl_DrawID` built-in

### Objects Excluded from MDI Shader Generation

MDI shaders are NOT generated for objects with special rendering requirements:
- Sprites (`isSprite()`) — need billboard model matrix (`getSpriteModelMatrix()`)
- InfinityView — need rotation-only view matrix
- Depth-test/write-disabled — order-dependent rendering
- The check happens in `Scene.rendering.cpp` before `getReadyForMDI()` is called.

**Code references:**
- `Generator/Abstract.hpp:IsMultiDrawIndirectEnabled` — Generator flag
- `Generator/Abstract.cpp:declareMatrixPushConstantBlock()` — MDI push constant block
- `AbstractVertexStage.hpp:enableMDI()` — Extension registration + `m_MDIEnabled` flag
- `AbstractVertexStage.cpp:prepareMDIModelMatrix()` — BDA reconstruction + SSBO access
- `AbstractVertexStage.cpp:onSourceCodeGeneration()` — `PerDrawDataRef` struct declaration
- `Program.hpp:wasMDIEnabled()` — Query MDI state from program
- `RenderableInstance/Abstract.cpp:getReadyForMDI()` — MDI program generation
- `Renderable/ProgramCacheKey.hpp:isMDIEnabled` — Cache key discrimination
