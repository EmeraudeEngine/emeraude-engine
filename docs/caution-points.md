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
> **Resolved follow-ups** (May 2026, see entries below):
> - Alpha-test layers in RT — handled via ray-query candidate confirmation +
>   `gl_RayFlagsNoneEXT` + per-material `IsAlphaTest` flag + opacity sampling.
> - Multi-instance / same-BLAS material aliasing — replaced by multi-geometry
>   BLAS (one `VkAccelerationStructureGeometryKHR` per sub-geometry) + shader-
>   side `rayQueryGetIntersectionGeometryIndexEXT` lookup. One TLAS instance per
>   renderable now, with per-sub-geo material in `GPUMeshMetaData::materialIndices`.
>
> **Files involved:**
> - `Scenes/Scene.rendering.cpp` — three RT-batch-creation sites (scene visuals,
>   static entities, scene nodes)
> - `Scenes/SceneMetaData.cpp:rebuild()` — already supports per-batch
>   `subGeometryIndex` for material lookup (no change needed)

### Fixed: Multi-Geometry BLAS Resolves Multi-Layer Material Aliasing (May 2026)

> [!CRITICAL]
> **Renderables with multiple opaque layers used to produce multiple TLAS
> instances pointing to the same BLAS. Vulkan's ray query picks one instance
> per hit, so triangles from layer A could be attributed to layer B's material
> — e.g. palm trunk wood texture appeared on the leaves' triangles in
> reflections, and vice versa.**
>
> **Fix:** each `Geometry::Interface` now builds a BLAS with one
> `VkAccelerationStructureGeometryKHR` per sub-geometry (sharing VB/IB but each
> with its own `primitiveOffset`/`primitiveCount` derived from `subGeometryRange(i)`).
> A single TLAS instance per renderable suffices; the RT trace shaders use
> `rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true/false)` to identify
> which sub-geometry was hit and look up the correct material via
> `GPUMeshMetaData::materialIndices[geomIdx]`.
>
> **Data layout change:** `GPUMeshMetaData` grew from 32 B to 48 B (3 `uvec4`
> instead of 2). The new third `uvec4` is `materialIndices[MaxSubGeometriesPerMesh]`
> with `MaxSubGeometriesPerMesh = 4`. RTR/RTGI shader indexing uses `* 3u` stride.
>
> **Caveat — animated sprites:** the procedural sprite quad builder emits one
> group per animation frame slot (`MaxFrames = 120`), so the BLAS has many more
> sub-geometries than the renderable's logical material count (1). The shader
> clamps `geomIdx` to 0 when `meshMeta.subGeometryCount == 1`, so multi-frame
> sprite BLAS still resolves to a single material. If you add new procedural
> geometries that produce more than `MaxSubGeometriesPerMesh` materials, bump
> the constant in `Scenes/GPUMeshMetaData.hpp` (memory grows linearly with
> instance count).
>
> **Files involved:**
> - `Vulkan/AccelerationStructureBuilder.{hpp,cpp}` — `buildBLAS` now takes
>   `std::vector<BLASGeometryInput>`, each input has a `firstIndex` field
> - `Graphics/Geometry/Interface.cpp:buildAccelerationStructure` — iterates
>   sub-geometries via `subGeometryRange(i)`
> - `Scenes/GPUMeshMetaData.hpp` — 48 B layout with `materialIndices[4]`
> - `Scenes/SceneMetaData.cpp:rebuild` — fills `materialIndices` per sub-geo,
>   adds one TLAS instance per renderable, FORCE_NO_OPAQUE if any sub-geo is
>   alpha-test
> - `Scenes/Scene.rendering.cpp` — one RT batch per renderable (was per layer)
> - `Graphics/Effects/Framebuffer/RTR.cpp`, `RTGI.cpp` — `getHitMaterialIndex`
>   uses `rayQueryGetIntersectionGeometryIndexEXT` + clamp on `subGeometryCount`

### Fixed: Sprite RT Pipeline — Per-Frame Bindless + CPU Billboard (May 2026)

> [!WARNING]
> **Sprites combine two RT-hostile features: their albedo is an
> `AnimatedTexture2D` (a `VK_IMAGE_VIEW_TYPE_2D_ARRAY` not samplable via the
> bindless `sampler2D[]` descriptor), and their geometry is a flat XY quad in
> object space, rotated face-camera only by the rasterizer's vertex shader.**
>
> **AnimatedTexture path:** `AnimatedTexture2D` now pre-creates one
> `VK_IMAGE_VIEW_TYPE_2D` view per layer (in addition to the main 2D_ARRAY
> view). `SceneMetaData::rebuild` walks `m_textureRegistrationCache` each
> frame, detects textures with `frameCount() > 1` via `dynamic_cast` to
> `AnimatedTexture2D`, computes the current frame from `Scene::lifetimeMS()`,
> and refreshes the bindless slot via `updateTexture2DFromDescriptorInfo` —
> UPDATE_AFTER_BIND makes this safe while the GPU is reading. Animation in
> reflections stays in sync with the rasterizer's animation state.
>
> **Billboard path:** for `Renderable::isSprite()` instances,
> `SceneMetaData::rebuild` overwrites the rotation columns of the TLAS instance
> transform with a cylindrical (Y-axis-only) face-camera rotation toward the
> current camera position. Translation and uniform scale are preserved. The
> sprite stays upright regardless of camera elevation.
>
> **Caveats:**
> - Texture-array bindless slot would be cleaner long-term (no per-frame
>   descriptor update churn) but requires a new descriptor binding. The
>   pragmatic per-frame swap is fine for the current sprite count.
> - Cylindrical billboard means cameras directly above/below a sprite see a
>   degenerate horizontal direction — the code falls back to a default
>   `(fx=0, fz=1)` facing to keep the rotation well-defined.
> - Sprites also need `Material::Interface::isAlphaTest()` to return true so
>   the trace shader skips transparent texels — see `SpriteResource.cpp` where
>   `setOpacity` is ALWAYS called (even at Opacity=1.0) to set the
>   `OpacityEnabled` flag.
>
> **Files involved:**
> - `Graphics/TextureResource/AnimatedTexture2D.{hpp,cpp}` — per-frame 2D views
>   and `imageViewForFrame(uint32_t)` accessor
> - `Scenes/SceneMetaData.{hpp,cpp}` — `rebuild()` signature adds `sceneTimeMS`
>   and `cameraPosition`; per-frame bindless refresh + billboard rotation
> - `Scenes/Scene.rendering.cpp` — passes `lifetimeMS()` and
>   `viewMatrices().position()` to `rebuild()`
> - `Graphics/Renderable/SpriteResource.cpp` — `setOpacity` always called

### Fixed: Unlit Sprite Alpha/Blending Regression from RT `setOpacity` Forcing (June 2026)

> [!WARNING]
> **Side-effect of the RT alpha-test fix above.** Forcing `setOpacity()` on every
> sprite (to set `OpacityEnabled` for `isAlphaTest()`) silently broke the **raster**
> rendering of all unlit alpha-blended sprites (smoke, fireball, any
> `BasicResource` sprite without `enableLighting()`).
>
> **Root cause:** the unlit fragment output path (`SceneRendering.cpp` →
> `Material::Interface::fragmentColor()`) returned `vec4(SurfaceColor.rgb,
> <uniform Opacity>)` whenever `OpacityEnabled` was set — **discarding the
> texture's per-texel alpha channel**. With the RT fix now setting `OpacityEnabled`
> on every sprite (uniform Opacity defaulting to 1.0), the output alpha became 1.0
> everywhere → `Normal` blending with `srcAlpha=1.0` rendered the sprite as an
> opaque quad instead of a soft alpha gradient.
>
> **Why the lit path was immune:** `BasicResource::setupLightGenerator()` already
> prioritizes the texture alpha channel (`SurfaceColor.a`) over the uniform
> opacity. Only the unlit `fragmentColor()` ignored it.
>
> **Fix:** `BasicResource::fragmentColor()` now mirrors `setupLightGenerator()` —
> when the texture has an alpha channel (`m_textureComponent->alphaEnabled()`), the
> output alpha is driven by `SurfaceColor.a` (optionally `× uniform Opacity` when a
> global fade is also requested). The uniform-opacity-only branch is now a fallback
> for alpha-less textures. RT alpha-test (driven by `OpacityEnabled || BlendingEnabled`)
> is unaffected.
>
> **Takeaway:** `OpacityEnabled` must NOT mean "override the texture alpha". A flag
> repurposed as an RT signal must stay neutral on the raster path.
>
> **Files involved:**
> - `Graphics/Material/BasicResource.cpp` — `fragmentColor()` respects texture alpha

### Fixed: Lit Sprite Shadow-Map Vertex Shader — `vaModelMatrix` Undeclared (June 2026)

> [!WARNING]
> **A billboard sprite with `enableLighting()` failed to compile its
> light/shadow-pass vertex shader** (`DirectionalLightPassColorMap`, also spot and
> point-light cubemap passes). GLSL error: `'vaModelMatrix' : undeclared identifier`.
>
> **Root cause:** `LightGenerator::generateVertexShaderShadowMapCode()` computed the
> light-space position as `PositionLightSpace = ViewProjectionMatrix * vaModelMatrix
> * vec4(Position, 1.0)` (and `DirectionWorldSpace` likewise for point lights),
> branching only on `isInstancingEnabled()` → `Attribute::ModelMatrix` (`vaModelMatrix`)
> vs push-constant. But a **billboard sprite** is instanced AND face-camera: its main
> matrix path (`VertexShader::prepareModelViewMatrix()` / MVP) declares
> `ShaderVariable::SpriteModelMatrix` (`svSpriteModelMatrix`, computed in-shader from
> `vaModelPosition`/`vaModelScaling`) and never declares the `vaModelMatrix` attribute.
> The shadow code referenced a variable that does not exist for sprites.
>
> **Fix:** mirror the billboard branch from `prepareModelViewMatrix()`. When
> `isInstancingEnabled() && isBillBoardingEnabled()`, use `ShaderVariable::SpriteModelMatrix`
> instead of `Attribute::ModelMatrix`. Safe because the sprite's `gl_Position`
> synthesis always prepares `SpriteModelMatrix` before the light code runs (it is
> emitted earlier in the final shader).
>
> **Takeaway:** any shader-gen path that consumes the model matrix must account for
> the THREE matrix sources — instanced attribute (`vaModelMatrix`), push constant,
> and billboard-sprite in-shader (`svSpriteModelMatrix`). Grep for `Attribute::ModelMatrix`
> when adding a new vertex code path; a bare instancing check is incomplete.
>
> **Files involved:**
> - `Saphir/LightGenerator.ShadowMap.cpp` — `generateVertexShaderShadowMapCode()`
>   billboard branch for both `PositionLightSpace` and `DirectionWorldSpace`

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

### Fixed: `Node::destroyTree()` Did Not Recurse → Zombie Components on Child Nodes (June 2026)

> [!WARNING]
> **`Node::destroyChildren()` only did `m_children.clear()` — it did NOT tear down
> child subtrees properly.** `clearComponents()` (the ONLY path emitting component
> `*Destroyed` notifications: `PointLightDestroyed`, `CameraDestroyed`,
> `MicrophoneDestroyed`, `ModifierDestroyed`, `SpotLightDestroyed`) was therefore run
> on the SUBTREE ROOT only. Child nodes were destroyed by their default destructors,
> which never fire those notifications.
>
> **Consequence:** any registry that holds a `shared_ptr` to a component and releases
> it only on the `*Destroyed` notification — `LightSet` (lights), `AVConsoleManager`
> (cameras/microphones), modifier lists — kept the component alive as a **zombie**
> after its parent node was freed. The component's `m_parentEntity` reference then
> dangled.
>
> **Crash:** the rendering thread (`LightSet::updateVideoMemory` →
> `Component::Abstract::getWorldCoordinates` → `m_parentEntity.getWorldCoordinates()`)
> dereferenced the freed parent node → use-after-free → intermittent
> `pure virtual method called`. Reproduced with `Actor::Fire` (projet-alpha), whose
> `PointLight` sits on a child node and dies when the fire fades out.
>
> **Fix:** `Node::destroyChildren()` now recurses — `child->destroyTree()` for each
> child BEFORE `m_children.clear()` — so every descendant runs `clearComponents()` and
> emits its notifications before being freed. Registry removal happens synchronously on
> the logics thread under the registry's mutex, correctly serialized against the
> rendering thread.
>
> **File:** `Scenes/Node.hpp` — `destroyChildren()`.
>
> **Takeaway:** the logics/rendering threads run concurrently under a SHARED scene lock
> and are decoupled by double-buffering; the rendering thread must never outlive-read an
> entity. A `*Destroyed` notification that drives registry cleanup MUST fire on every
> teardown path, including deep subtrees — a destructor is not a substitute.

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

## Build / Compiler

### PCH shifts GCC's inlining context → `-Wstringop-overread` false positives

With the shared STL precompiled header enabled (`EMERAUDE_ENABLE_PCH=ON`, applied to the engine
since the cascade-wide PCH wiring), GCC 14 can raise a **`-Werror=stringop-overread`** in
`<bits/char_traits.h>` (`__builtin_memcpy reading N bytes from a region of size 16`) on perfectly
valid `std::string` code. It is a known GCC false positive: the PCH changes how the STL headers are
pre-parsed, which shifts inlining decisions, and GCC's value-range analysis then mis-judges that a
string whose inferred length exceeds the 15-byte SSO buffer could still live in that inline buffer
during a move-construct.

- **Seen in:** `Saphir/LightGenerator.cpp::finalNormalViewSpaceExpression()` — a
  `std::string{"normalize("} + Keys::ShaderVariable::NormalViewSpace + ")"` concat (28-char result).
- **Wrong fixes:** silencing the warning (`-Wno-stringop-overread`, `#pragma GCC diagnostic`,
  `NOLINT`) — the project never disables warnings. Also note that merely rewriting `operator+`
  into `+=` does **not** help: the trip-wire is the move-construct on `return`, not the concat.
- **Correct fix:** make the buffer unambiguously heap-allocated so GCC cannot assume SSO — build the
  string into a local and `reserve()` past 15 bytes before appending. That removes the ambiguity the
  analysis chokes on, with no behavioural change.
- **Only one site in the whole cascade** trips this today (274/275 TUs compile clean under PCH); do
  not pre-emptively rewrite other concatenations — fix sites as the compiler actually flags them.

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
