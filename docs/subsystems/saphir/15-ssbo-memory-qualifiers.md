## SSBO Memory Qualifiers

`ShaderStorageBlock` supports GLSL memory access qualifiers via `setAccessQualifier()`:

| Qualifier | GLSL Output | Use Case |
|-----------|-------------|----------|
| `AccessQualifier::None` | `buffer Name { ... }` | Read-write access (default) |
| `AccessQualifier::ReadOnly` | `readonly buffer Name { ... }` | GPU reads only (e.g., bone matrices, per-draw data) |
| `AccessQualifier::WriteOnly` | `writeonly buffer Name { ... }` | GPU writes only (e.g., compute output buffers) |

> [!CRITICAL]
> **SSBOs in vertex/geometry/tessellation shaders MUST be `readonly` unless `vertexPipelineStoresAndAtomics`
> is enabled.** Without the `readonly` qualifier, Vulkan requires this device feature for any storage buffer
> accessed in these stages. Omitting it causes `VUID-RuntimeSpirv-NonWritable-06341` validation errors and
> pipeline creation failure on many GPUs.
>
> **Rule of thumb:** If the shader only reads from an SSBO, always mark it `ReadOnly`. This is both
> semantically correct and avoids unnecessary feature requirements.

```cpp
Declaration::ShaderStorageBlock ssbo{setIndex, 0, Declaration::MemoryLayout::Std430, "MyData", "ubMyData"};
ssbo.setAccessQualifier(Declaration::AccessQualifier::ReadOnly);
ssbo.addMember(Declaration::VariableType::Matrix4, "matrices[]");
vertexShader->declare(ssbo);
```

**Code references:**
- `Declaration/ShaderStorageBlock.hpp` — `setAccessQualifier()` / `accessQualifier()`
- `Declaration/Types.hpp` — `AccessQualifier` enum
- `Generator/SceneRendering.cpp` — Skinning SSBO (ReadOnly)
- `Generator/ShadowCasting.cpp` — Skinning SSBO (ReadOnly)
