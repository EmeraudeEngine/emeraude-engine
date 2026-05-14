# Caution Points

Critical warnings, known pitfalls, and hard-won lessons for Emeraude Engine development.

## Table of Contents

- [Graphics/Material System](#graphicsmaterial-system)
- [Ray Tracing / Acceleration Structures](#ray-tracing--acceleration-structures)
- [Scene Rendering](#scene-rendering)
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

## Ray Tracing / Acceleration Structures

### Fixed: TLAS Instance Transform Must Include Renderable Scale (Apr 2026)

> [!CRITICAL]
> **The TLAS instance transform must combine entity world coordinates WITH the
> renderable instance's transformation matrix (which carries `uniformScale`).**
>
> **Bug pattern (fixed Apr 2026):**
> - `SceneMetaData::rebuild()` built the TLAS instance transform from
>   `worldCoordinates->getModelMatrix()` only (position + rotation)
> - The `renderableInstance->transformationMatrix()` (containing `uniformScale`)
>   was ignored
> - Result: RT effects traced against raw object-space geometry while the
>   rasterizer rendered the scaled version → **phantom/ghost shapes** for any
>   mesh with a non-identity transformation matrix (e.g. scaled meshes)
>
> **Fix:**
> ```cpp
> auto finalMatrix = batch.renderableInstance()->transformationMatrix();
> if ( const auto * worldCoordinates = batch.worldCoordinates(); worldCoordinates != nullptr )
> {
>     finalMatrix = worldCoordinates->getModelMatrix() * finalMatrix;
> }
> ```
>
> **Files involved:**
> - `Scenes/SceneMetaData.cpp:rebuild()` - TLAS instance construction

### Fixed: RTR Self-Reflection Rejection Too Aggressive (May 2026)

> [!WARNING]
> **The RTR trace shader's self-reflection rejection rejected any hit whose
> normal was within ~25° of the ray-origin surface normal — silently excluding
> all reflections off parallel surfaces at a distance (cube tops, ceilings,
> stacked horizontal walls reflected in the floor).**
>
> **Old check** (`Graphics/Effects/Framebuffer/RTR.cpp` ~line 372):
> ```glsl
> if (dot(hitNormal, worldNormal) > 0.9) { outReflection = vec4(0.0); return; }
> ```
> Intent was to reject the floor reflecting itself (a numerical artifact from
> imperfect normal-offset). But cube tops have normal=(0,−1,0) identical to the
> floor's UP, so `dot=1.0 > 0.9` → rejected → cube *never* appears in the floor's
> reflection. Curved surfaces (sphere, sphere of the palm-like meshes) happen to
> work because their normals vary, never hitting the threshold.
>
> **Fix:** combined check that requires BOTH normal-parallelism AND tiny hitT:
> ```glsl
> if (dot(hitNormal, worldNormal) > 0.99 && hitT < 0.05) { /* reject */ }
> ```
> Real reflections off parallel surfaces have `hitT >> 0.05` and pass through.
> True self-intersection artifacts are caught by the `hitT < 0.05` clause.
>
> **Open thread:** SSR.cpp likely shares this pattern (memory references both
> SSR and RTR using the rejection). Not modified in this fix; verify next time
> SSR is tuned.
>
> **Files involved:**
> - `Graphics/Effects/Framebuffer/RTR.cpp` — trace shader self-rejection check

### Fixed: RT TLAS Collection Hardcoded to Layer 0 (May 2026)

> [!CRITICAL]
> **The RT batch creation in `Scenes/Scene.rendering.cpp` only checked
> `renderable->isOpaque(0)` — a renderable whose first layer was alpha-tested
> or transparent was excluded *entirely* from the TLAS, even if other layers
> were opaque.**
>
> **Symptom:** the palm tree (`MultiLayerMesh` with leaves on layer 0 + opacity
> map → `isOpaque(0) = false`) was completely invisible to RT rays. The user
> could see the sphere's reflection in the floor *through* the palm trunk's
> position — proof that rays were passing through where the palm geometry should
> have been. Even the opaque trunk on a later layer was missed.
>
> **Old code** (three call sites: scene visuals ~line 994, static entities
> ~line 1041, scene nodes ~line 1098):
> ```cpp
> if ( renderable != nullptr && renderable->isOpaque(0) )
> {
>     RenderBatch::create(rtList, distance, renderableInstance, &worldCoordinates, 0);
> }
> ```
>
> **Fix:** iterate all layers, emit one RT batch per opaque layer with the
> correct `subGeometryIndex`. Each batch becomes a separate TLAS instance with
> its own material lookup in `SceneMetaData::rebuild`:
> ```cpp
> if ( renderable != nullptr )
> {
>     const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();
>     auto & rtList = isLighted ? m_rtOpaqueLightedList : m_rtOpaqueList;
>     const auto layerCount = renderable->layerCount();
>     for ( uint32_t layer = 0; layer < layerCount; ++layer )
>     {
>         if ( renderable->isOpaque(layer) )
>         {
>             RenderBatch::create(rtList, distance, renderableInstance, &worldCoordinates, layer);
>         }
>     }
> }
> ```
> Validated visually 2026-05-14: palm trunk reflects in the floor and correctly
> *occludes* the sphere's reflection that was previously visible through the
> trunk's screen position.
>
> **Open threads:**
> - **Alpha-test layers still excluded.** `isOpaque(layer)` is false for opacity-
>   mapped materials, so palm leaves never enter the TLAS. Proper handling needs
>   either an any-hit shader doing alpha-test against the opacity map at hit time,
>   or a relaxed filter that admits alpha-test layers (with the cutout simply
>   ignored — visible but wrong-looking foliage in reflections).
> - **Multi-instance / same-BLAS material ambiguity.** When a renderable has
>   multiple opaque layers, multiple TLAS instances point to the *same* BLAS
>   (different materials each). The BLAS doesn't separate triangles by
>   sub-geometry, so Vulkan reports whichever instance is found first — the
>   material may not match the triangle's actual sub-geometry. Architectural fix:
>   build multi-geometry BLAS (one `VkAccelerationStructureGeometryKHR` per
>   sub-geometry) and use `rayQueryGetIntersectionGeometryIndexEXT` in the trace
>   shader. Not observed in current scenes (palm has only one opaque layer);
>   flag if multi-opaque-layer renderables appear later.
>
> **Files involved:**
> - `Scenes/Scene.rendering.cpp` — three RT-batch-creation sites (scene visuals,
>   static entities, scene nodes)
> - `Scenes/SceneMetaData.cpp:rebuild()` — already supports per-batch
>   `subGeometryIndex` for material lookup (no change needed)

### Known Issue: MRT Normal Blend for Translucent Materials

> [!WARNING]
> **The MRT normal attachment for TranslucentGB materials uses incorrect alpha
> for blending, making normal-mapped reflections nearly invisible on low-roughness
> translucent surfaces (e.g. water).**
>
> **Problem:**
> - `SceneRendering::onCreateGraphicsPipeline()` duplicates the color blend state
>   for MRT normal/material property attachments
> - The MRT normal output packs roughness+metalness into alpha:
>   `outNormal.a = roughness + metalness * 2.0`
> - For the AmbientPass of translucent materials (Normal blending mode),
>   `SRC_ALPHA` uses `outNormal.a` (e.g. 0.03 for water) instead of visual opacity
> - Result: water normal contributes only 3% to MRT — ground normal dominates at 97%
> - RT post-process effects (RTR, SSR, SSAO, RTAO) see a flat surface
>
> **Status:** Diagnosed. A fix using `blendEnable = VK_FALSE` for MRT attachments
> in AmbientPass was tested and confirmed working (visible with exaggerated normal
> intensity), but caused the water surface to disappear. Needs a more careful approach —
> possibly separate blend states per attachment using Vulkan independent blend.
>
> **Files involved:**
> - `Saphir/Generator/SceneRendering.cpp:onCreateGraphicsPipeline()` - MRT blend state duplication
> - `Vulkan/GraphicsPipeline.cpp:configureColorBlendState()` - Blend state per render pass type

### Fixed: MRT Material Properties Alpha Preservation in Light Passes (May 2026)

> [!WARNING]
> **MRT attachments inherit the color attachment's blend state, but opaque light
> passes use REPLACE alpha (`srcAlpha=ONE, dstAlpha=ZERO`). A light-pass shader
> writing `vec4(0.0)` to an alpha-encoded MRT attachment zeroes out the alpha
> channel in every lit pixel — silently corrupting any data stored in alpha.**
>
> **Symptom:** post-process effects reading the matprops MRT alpha (fog response,
> DoF mask) saw `A = 1.0` only on sky/unlit pixels (untouched clear value) and
> `A = 0` on lit pixels. AtmosphericFog modulation `fogAmount *= fogResponse`
> produced visible halos at lit/unlit transitions (cube silhouettes, object
> edges, SSR reflections).
>
> **Fix:** light-pass shader writes `vec4(0.0, 0.0, 0.0, 1.0)` instead of
> `vec4(0.0)`. With opaque-light-pass REPLACE alpha blend, `srcAlpha=1` →
> `newA = 1.0`, preserving the AmbientPass-encoded matprops alpha nibbles
> (fogResponse | dofMask) in every lit pixel. RGB is unchanged because the
> additive RGB blend with `srcRGB=0` still adds 0.
>
> **Why not modify the blend state instead?** Attempted: lambda-override on the
> matprops attachment to set `srcAlphaBlendFactor=ZERO, dstAlphaBlendFactor=ONE`.
> Result: the entire framebuffer rendered as a uniform fog blob — geometry color
> never reached the final image. Root cause unconfirmed (possibly pipeline-cache
> hash mismatch or unexpected interaction with color attachment writes). The
> shader-level fix is more surgical: no pipeline-state change, no side effects.
>
> **Same root cause as the Translucent Normal MRT issue above**, but a different
> symptom path. The normals attachment also writes `vec4(0.0)` in light passes
> and theoretically loses `roughness+metalness` from alpha in lit pixels; SSR/RTR
> appear to work anyway (likely reading the AmbientPass normal value before
> light passes overwrite, or not depending on alpha). Not fixed here.
>
> **Future:** if a material ever needs a per-pixel non-`1.0` matprops alpha
> (e.g., HUD with custom fogResponse/dofMask), the light pass write must emit
> the AmbientPass alpha value instead of the hardcoded `1.0` — currently
> `LightGenerator::materialPropertiesExpression()` hardcodes alpha nibbles to
> 15 (full), so this constraint is dormant.
>
> **Files involved:**
> - `Saphir/Generator/SceneRendering.cpp` (~line 461): light-pass matprops write
> - `Saphir/LightGenerator.cpp` (`materialPropertiesExpression()`): AmbientPass alpha is hardcoded `1.0`
> - `Vulkan/GraphicsPipeline.cpp:configureColorBlendState()` (~line 668): opaque light-pass alpha=REPLACE

---

## Scene Rendering

### Fixed: Scene Visual Components Null Check (Feb 2026)

> [!WARNING]
> **`m_sceneVisualComponents[0]` can be null when no background is set.**
>
> In `Scene.rendering.cpp:getRenderableInstanceReadyForRendering()`, the environment cubemap check previously assumed a background always exists:
> ```cpp
> // BUG: m_environmentCubemap is ALWAYS non-null (initialized from default cubemap)
> // but m_sceneVisualComponents[0] is null without a background → crash
> if ( m_environmentCubemap != nullptr && renderableInstance == m_sceneVisualComponents[0]->getRenderableInstance() )
> ```
>
> **Fix:** Added null check: `m_sceneVisualComponents[0] != nullptr &&`
>
> **Trigger:** Scenes without skybox/background (e.g. closed rooms with no `enableBasicBackground()`).
>
> **File:** `Scenes/Scene.rendering.cpp:1385`

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

### Critical: World-Space Y Reconstruction from Depth (Y-DOWN)

> [!CRITICAL]
> **`cross(right, forward) = viewYAxis` (row 1 of view matrix). NOT `cross(forward, right)`!**
>
> When reconstructing world-space positions from depth using camera basis vectors,
> the camera "up" vector must be computed as `cross(cameraRight, cameraForward)`, not
> `cross(cameraForward, cameraRight)`. In a right-handed coordinate system with axes
> (right, viewY, backward):
>
> - `cross(right, forward) = cross(X, -Z) = +Y` → correct view Y axis
> - `cross(forward, right) = cross(-Z, X) = -Y` → **inverted**, causes Y-flipped reconstruction
>
> **Symptom:** Height-dependent effects (fog, height-based coloring) appear vertically
> inverted — e.g. fog disappears from screen bottom when looking up.
>
> **Note:** SSAO/SSR work in view space with relative positions, so the sign error
> cancels out. Only effects that need **absolute world-space height** (like atmospheric
> fog) expose this bug.
>
> **Code reference:** `Effects/Framebuffer/AtmosphericFog.cpp` — shader `cameraUp` computation

### Critical: Inscattering Light Direction Convention

> [!IMPORTANT]
> **When `setLightDirection()` takes the emission direction (sun → scene), negate it
> for inscattering `dot(rayDir, -lightDir)`.**
>
> The `dot(rayDir, lightDir)` product gives cosAngle = +1 when looking **away** from
> the sun (same direction as light travel), which is the physically correct forward
> scattering peak. But the **expected visual result** (UE5-style sun glow on the horizon)
> requires maximum inscattering when looking **toward** the sun.
>
> **Fix:** Use `dot(rayDir, -lightDir)` so the glow appears around the sun, not
> at the anti-solar point.
>
> **Code reference:** `Effects/Framebuffer/AtmosphericFog.cpp` — shader inscattering section

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
