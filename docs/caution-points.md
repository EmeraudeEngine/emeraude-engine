# Caution Points

Critical warnings, known pitfalls, and hard-won lessons for Emeraude Engine development.

## Table of Contents

- [Graphics/Material System](#graphicsmaterial-system)
- [Shader/GLSL Pitfalls](#shaderglsl-pitfalls)
- [Platform-Specific](#platform-specific)

---

## Graphics/Material System

### Critical: Shared UBO Offset (Materials)

> [!CRITICAL]
> **The `Buffer::getDescriptorInfo()` function MUST correctly apply byte offsets!**
>
> Materials use a **SharedUniformBuffer** where multiple materials share a single Vulkan UBO.
> Each material has a unique `m_sharedUBOIndex` that determines its offset in the buffer.
>
> **Bug pattern (fixed in Jan 2026):**
> - `Buffer.hpp:getDescriptorInfo()` was ignoring the offset parameter (`offset = 0`)
> - All materials read from offset 0, regardless of their actual UBO index
> - Result: Material B reads Material A's data → wrong reflection/refraction amounts
>
> **Files involved:**
> - `Vulkan/Buffer.hpp` - `getDescriptorInfo(offset, range)` must use `offset`
> - `Vulkan/UniformBufferObject.cpp` - Must convert element index to byte offset: `elementOffset * m_blockAlignedSize`
> - `Graphics/Material/StandardResource.cpp` - Uses `m_sharedUBOIndex` for UBO slot

### Material Property Array Layout (std140)

The `StandardResource` material uses a float array with specific offsets (std140 aligned):

| Offset | Property | Range | Notes |
|--------|----------|-------|-------|
| 0-3 | ambientColor | vec4 | RGBA |
| 4-7 | diffuseColor | vec4 | RGBA |
| 8-11 | specularColor | vec4 | RGBA |
| 12-15 | autoIlluminationColor | vec4 | RGBA |
| 16 | shininess | float | 0-128+ |
| 17 | opacity | float | 0-1 |
| 18 | autoIlluminationAmount | float | 0-1 |
| 19 | normalScale | float | 0-1 |
| **20** | **reflectionAmount** | float | 0-1 |
| **21** | **refractionAmount** | float | 0-1 |
| **22** | **refractionIOR** | float | 1.0-3.0 |

**Debugging tip:** If reflection/refraction amounts seem wrong, trace:
1. C++ side: Are values written to correct offsets?
2. Shader side: Is the UBO struct layout matching?
3. Descriptor: Is the correct byte offset used?

### Fresnel Effect (Reflection + Refraction)

When both reflection AND refraction components are present:

1. **`fresnelFactor` is auto-generated** by `StandardResource.cpp` during shader generation
2. It's computed using the Schlick approximation with IOR
3. The lighting code in `LightGenerator.cpp` uses it to blend between reflected and refracted colors
4. **`refractionIOR` is clamped** to [1.0, 3.0] - values below 1.0 (like 0.33) become 1.0

### Material Types Registration

> [!CRITICAL]
> **All material types must be registered in `Material::Types` array!**
>
> The `Material::Types` array in `Materials.hpp` is used by `FastJSON::getValidatedStringValue()`
> to validate material type strings from JSON. If a type is missing, it falls back to `BasicResource`.
>
> **Bug pattern (fixed Jan 2026):**
> - `PBRResource::ClassId` was missing from `Material::Types`
> - Mesh JSON with `"MaterialType": "MaterialPBRResource"` silently fell back to Basic
> - Result: PBR materials loaded as Basic materials
>
> **Files involved:**
> - `Graphics/Material/Materials.hpp` - Types array definition
> - `Graphics/Renderable/MeshResource.cpp:parseLayer()` - Uses validated type

### MeshResource Layer Parsing (C++20 Pattern)

The `parseLayer()` function uses a C++20 lambda template to avoid code duplication:

```cpp
auto loadMaterial = [&] < typename ResourceType > () -> std::shared_ptr< Material::Interface >
{
    auto * container = serviceProvider.container< ResourceType >();
    if ( !materialResourceName )
    {
        TraceError{ClassId} << "...";
        return container->getDefaultResource();
    }
    return container->getResource(materialResourceName.value());
};

if ( materialType == PBRResource::ClassId )
    return loadMaterial.operator() < PBRResource > ();
```

**Code reference:** `Graphics/Renderable/MeshResource.cpp:parseLayer()`

### Shader Variable Naming: TBN Matrices

> [!IMPORTANT]
> **`TangentToWorldMatrix`** transforms vectors **FROM tangent space TO world space**.
>
> Construction: `NormalMatrix * mat3(Tangent, Bitangent, Normal)` where T,B,N are columns.
>
> **Code references:**
> - `Saphir/Keys.hpp:ShaderVariable::TangentToWorldMatrix`
> - `Saphir/VertexShader.cpp:synthesizeTangentToWorldMatrix()`

---

## Shader/GLSL Pitfalls

### GLSL smoothstep Undefined Behavior

> [!CRITICAL]
> `smoothstep(edge0, edge1, x)` is **undefined when `edge0 >= edge1`** per GLSL spec.
> On some GPUs this produces NaN, causing visual flickering/darkening.

**Affected pattern** (SSS wrap lighting):
```glsl
// WRONG — when sssIntensity = 1.0, this is smoothstep(1.0, 1.0, x) → UB
float sssWrap = sssIntensity;
float wrapFactor = smoothstep(sssWrap, 1.0, NdotLWrap);

// CORRECT — clamp to ensure edge0 < edge1
float sssWrap = min(sssIntensity, 0.99);
```

**Code reference:** `LightGenerator.PBR.cpp` lines 702, 716

### Clear Coat Normal: Do NOT Use Vertex TBN

The clear coat normal map must use a fragment-local tangent frame, NOT `ViewTBNMatrix`. Using vertex TBN causes:
- GPU hangs when base normal mapping is not active
- GLSL compilation errors (`svViewTBNMatrix` undeclared)

Use the same `cross(N, up)` pattern as anisotropy. See: `Saphir/AGENTS.md` (Clear Coat Normal section).

### POM GPU Stress on Large Surfaces

Parallax Occlusion Mapping ray-marching is expensive at far distances, especially on large surfaces. The engine implements distance-based fade (8-18 world units) to mitigate this. See: `Graphics/AGENTS.md` (POM section).

---

## Platform-Specific

### String Conversions on Windows

> **CRITICAL:** All UTF-8 ↔ Wide string conversions on Windows **MUST** use `Helpers.hpp` functions (`convertUTF8ToWide`, `convertWideToUTF8`). Do NOT create local wrappers with `MultiByteToWideChar`/`WideCharToMultiByte`.

**Files involved:**
- `PlatformSpecific/Helpers.hpp` — Declarations
- `PlatformSpecific/Helpers.windows.cpp` — Implementations

### Video Capture — macOS First-Frame Timing

AVFoundation's `startRunning` is asynchronous. The macOS `VideoCaptureDevice::open()` waits up to 3 seconds for the first frame via `std::condition_variable`. Without this, the first `captureFrame()` call would always fail.

**File:** `PlatformSpecific/VideoCaptureDevice.mac.mm:waitForFirstFrame:`

---

## Related Documentation

- `@AGENTS.md` - Engine root context
- `@src/PlatformSpecific/AGENTS.md` - Platform-specific system details
- `@src/Graphics/AGENTS.md` - Graphics system
- `@src/Saphir/AGENTS.md` - Shader generation system
