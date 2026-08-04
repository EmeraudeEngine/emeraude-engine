# Caution Points

Critical warnings, known pitfalls, and hard-won lessons for Emeraude Engine development.

## Table of Contents

- [Graphics/Material System](#graphicsmaterial-system)
- [Ray Tracing / Acceleration Structures](#ray-tracing--acceleration-structures)
- [Scene Rendering](#scene-rendering)
- [Shader/GLSL Pitfalls](#shaderglsl-pitfalls)
- [Platform-Specific](#platform-specific)
- [Vulkan Validation](#vulkan-validation)

---

## Graphics/Material System

### Fixed: Diamond-square ground/terrain — non-power-of-two division now snaps instead of failing (Jun 2026)

> **Symptom:** loading a demo whose ground used `loadDiamondSquare` (e.g. `basic-scenery`)
> crashed with `SIGSEGV` in the demo's `onBuilding` at `ground->getLevelAt(...)`. The log showed
> `The grid division (2000) must be a power of two to use diamond square!`.
>
> **Root cause:** diamond-square displaces the grid by recursive halving, so the division must be
> a power of two. `sceneAreaSize()` returns `2000` (a non-power-of-two), which every demo passed
> verbatim as `gridDivision`. `loadDiamondSquare` rejected it and returned `false` →
> `onSetupGroundLevel` returned `nullptr` → `m_groundLevel` was null → the demo dereferenced it.
>
> **Fix (zero-failure):** `BasicGroundResource::loadDiamondSquare` and
> `TerrainResource::loadDiamondSquare` now **snap `gridDivision` up to the next power of two**
> (`EmEn::Base::Math::nextPowerOfTwo`, e.g. 2000 → 2048) and log an info instead of failing. The
> relief is still generated and a ground is **always** produced — callers never get a null ground
> for a non-power-of-two division.
>
> **Takeaway:** `gridDivision` is not guaranteed to be used verbatim by diamond-square loaders — it
> is snapped to a valid power of two. `loadPerlinNoise` and the flat `load` accept any division.
>
> **Files:** `Graphics/Renderable/BasicGroundResource.cpp`, `Graphics/Renderable/TerrainResource.cpp`,
> `emeraude-base Math/Base.hpp` (`nextPowerOfTwo`).

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

### Critical: Shared UBO Registry Is Reached Concurrently (Materials)

> [!CRITICAL]
> **Never write `getSharedUniformBuffer()` then `createSharedUniformBuffer()`.** Use the atomic
> `SharedUBOManager::getOrCreateSharedUniformBuffer(name, blockSize)`.
>
> Material buffer identifiers encode only the material kind and its texture count
> (`MaterialPBRResource2Textures`), so distinct materials share one identifier **by design** — and
> materials are loaded in parallel on the resource thread pool. A separate get-then-create is a
> check-then-act race.
>
> **Bug pattern (fixed Aug 2026):**
> - Two threads load two 2-texture PBR materials of the same mesh; both miss the lookup, both create.
> - The loser gets `nullptr` from `createSharedUniformBuffer()` and its **whole material fails to
>   load** → every sub-mesh using it vanishes from the scene.
> - Lived on the `reflexion-debug` dragon: wings missing on some runs, body on others.
>
> **Log signature to recognize it instantly:**
> ```
> [Info] There is no shared uniform buffer named 'MaterialPBRResource2Textures' !   <- TWICE in a row
> [Error] A shared uniform buffer named 'MaterialPBRResource2Textures' already exists !
> [Error][MaterialPBRResource] Unable to get the shared uniform buffer !
> [Error][MaterialInterface] Unable to load the material resource '...' !
> ```
> The **doubled info line is the proof of concurrency**: single-threaded, the first miss would have
> created the buffer. A non-deterministic *"part of the mesh is missing, differently each run"* should
> always send you looking for a check-then-act on a shared registry.
>
> **Two aggravating factors were present and are also fixed:**
> - `m_sharedUniformBuffers` was a bare `std::map` with **no mutex**: two concurrent `emplace()` are a
>   plain data race, not merely a lost insertion — red-black tree corruption was possible.
> - `SharedUniformBuffer::addElement()` scanned and claimed a seat unguarded, so two materials could
>   take the **same** UBO offset and silently overwrite each other's uniform block. The mutex meant for
>   it (`m_memoryAccess`) was declared and **never locked anywhere** — a dead mutex. It is now
>   `m_elementsAccess` and actually held.
>
> **Files:** `Graphics/SharedUBOManager.{hpp,cpp}`, `Graphics/SharedUniformBuffer.{hpp,cpp}`,
> `Graphics/Material/Interface.cpp`. Full contract in
> [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) §5.

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

### Fixed: RTGI Bounce Lighting Leaked Through Walls — Missing Shadow Rays (Jul 2026)

> [!WARNING]
> **Symptom:** in an enclosed scene (GlobalIllumination demo, S-shaped Cornell box,
> single shadow-casting omni light), RTGI flooded fully occluded rooms with bright
> indirect light. Surfaces with no line of sight to the light (back faces of columns,
> shadow zones behind occluders) glowed **brighter** than directly exposed ones,
> reading like a "ray inversion". Color bleeding direction was correct — only the
> injected energy was wrong.
>
> **Root cause:** the trace pass' `computeDirectLighting()` (direct Lambert lighting
> evaluated at every bounce hit point) had **no occlusion test toward the lights**.
> Every hit point received full `color × intensity × NdotL × attenuation` straight
> through any wall. The raster direct pass is shadow-mapped, so the final image mixed
> correct direct shadows with unoccluded indirect fill — worst exactly where the
> scene should be darkest.
>
> **Fix:** one shadow ray per light per bounce hit (`shadowRayVisibility()` in
> `RTGI.cpp`'s trace shader): ray query with `gl_RayFlagsTerminateOnFirstHitEXT |
> gl_RayFlagsOpaqueEXT` from `hitPos + hitNormal × max(bias, 0.001)` toward the light
> (`tMax` = distance to a point/spot light, 10000 for directionals), contribution
> zeroed when occluded. Early-out skips the ray when `NdotL × attenuation ≤ 0`.
>
> **Expected behavior after the fix (NOT bugs):**
> - Occluded floors/ceilings adjacent to lit walls can stay near-black: this is
>   **one-bounce** GI — a surface that only "sees" shadowed geometry gets nothing,
>   because bounce hit points on shadowed surfaces contribute zero direct light.
>   Filling those areas requires a second bounce (future axis), not a bug fix.
> - A wall facing a bright opening lights up while the floor/ceiling of the same
>   corridor stay dark: cosine weighting — the wall faces the lit surfaces head-on,
>   floor/ceiling see them at grazing angles. SSGI shows the same structure.
>
> **Known gap:** `RTR.cpp` has the same unshadowed `computeDirectLighting()` — the
> error only shows in reflections OF shadowed regions (too bright), masked by
> Fresnel/roughness/distance fade. Porting the shadow ray to RTR is pending
> (validate against a reflection-heavy demo before/after).
>
> **Files involved:**
> - `Graphics/Effects/Framebuffer/RTGI.cpp` — `shadowRayVisibility()` + gated
>   contribution in `computeDirectLighting()` (trace shader)

### Fixed: SSGI Indirect Light Ignored Receiver Albedo — New Albedo G-Buffer Attachment (Jul 2026)

> [!WARNING]
> **Symptom:** a coloured surface lit ONLY by indirect light rendered grey in SSGI (a green
> column in shadow lost its colour entirely; a blue one washed to lavender). RTGI was correct:
> it recovers the receiver's albedo via a primary ray + the RT material SSBO. SSGI, screen-space,
> had no albedo source — its apply pass did `color += gi` with no receiver modulation
> (indirect diffuse must be `albedo × irradiance`).
>
> **Fix:** a fourth MRT attachment on the scene render target — **albedo,
> `VK_FORMAT_R8G8B8A8_SRGB`** (written linear, encoded on store, decoded on sample), written by
> the ambient/simple pass from `LightGenerator::albedoShaderExpression()` (PBR albedo /
> Standard-Basic diffuse / white fallback), and consumed by SSGI's apply pass
> (`gi *= texture(albedoTex, vUV).rgb`).
>
> **Contract points (MUST stay consistent when touching any of this):**
> - **Fixed MRT order:** `[0]=color, [1]=normals, [2]=materialProperties, [3]=albedo`, depth
>   last. The shader generator detects the layout **by color attachment count**
>   (`SceneRendering.hpp`: `>1`, `>2`, `>3`) — each attachment forces every one before it
>   (`Renderer::recreateSceneTarget()`: albedo ⇒ matprops ⇒ normals).
> - **Clear values:** `Renderer::m_clearColors` is now `std::array<VkClearValue, 5>` with
>   **`[4]` = depth** (was `[3]`); the per-combination subset arrays in `Renderer.cpp`
>   (both CLEAR and LOAD dispatch blocks) must cover every attachment combination.
> - **Blend states:** one `appendColorBlendAttachment()` per present MRT attachment
>   (`SceneRendering::onGraphicsPipelineConfiguration()`) — count must equal the subpass
>   color attachment count.
> - **Requirements plumbing:** `IndirectPostProcessEffect::requiresAlbedo()` →
>   `PostProcessStack::requiresAlbedo()` → `PostProcessor::updateCachedRequirements()/configure()`
>   (now 5 bool params) → scene target formats → GrabPass (albedo image + copy in
>   `PostProcessor::recordBlit()`) → `GrabPassAlbedoAdapter` → effect
>   `execute(..., inputMaterialProperties, inputAlbedo, lightSet, ...)` (signature grew by
>   one param across ALL 16 effects).
> - Allocation is **on demand**: no stack effect requires albedo → no attachment, zero cost.
>
> **Files involved:** `Graphics/{Renderer,SceneRenderTarget,GrabPass,PostProcessor,PostProcessStack}.{hpp,cpp}`,
> `Graphics/IndirectPostProcessEffect.hpp`, `Saphir/Keys.hpp` (`OutputAlbedo`),
> `Saphir/LightGenerator.{hpp,cpp}` (`albedoShaderExpression()`),
> `Saphir/Generator/SceneRendering.{hpp,cpp}`, `Graphics/Effects/Framebuffer/*.{hpp,cpp}`
> (signature), `SSGI.{hpp,cpp}` (consumer).

### Fixed: RTAO/RTGI tMin Skipped Near Occluders + SSAO Double Intensity & Screen-Edge Band (Jul 2026)

> [!WARNING]
> Three AO/GI defects caught by the GlobalIllumination demo's symmetric mode bench:
>
> **1. RTAO bright crease line (`RTAO.cpp`):** the ray query used `tMin = adaptiveBias`
> (`bias × max(1, cameraDist) × grazingFactor≤10` — can exceed a metre). tMin skips REAL
> geometry closer than it, so at wall/floor creases the adjacent surface was never hit →
> a bright line exactly where AO must be darkest, wider with distance/grazing. **Fix:**
> tMin is a tiny constant (0.001); the adaptive origin offset alone prevents
> self-intersection (hemisphere directions never descend below the surface). Same fix
> applied to `RTGI.cpp` bounce rays (same pattern → light leak at creases).
>
> **2. SSAO applied its intensity TWICE (`SSAO.cpp`):** once in the compute pass
> (`occ × intensity`) and once in the apply pass via an EXTRAPOLATING `mix(1.0, ao,
> intensity)` (t = 1.5 by default → overshoots below the computed AO). Default SSAO was
> far too dark ("violent"). **Fix:** compute pass stores the pure visibility term;
> intensity applied once in the apply pass, clamped.
>
> **3. SSAO black band at screen edges:** projected sample UVs were never validated; the
> clamp-to-edge depth sampler recycled border depth → false full occlusion (ragged solid
> black strip at the bottom of the frame on close grazing floors). **Fix:** out-of-frame
> samples are skipped (treated as unoccluded).
>
> **4. RTAO had the SAME double-intensity defect as SSAO (§2):** trace pass multiplied
> `occlusion × intensity` AND the apply pass ran an unclamped extrapolating
> `mix(1.0, ao, intensity)` — with the 1.5 default, more than double the physical
> darkening ("violent" AO). **Fix:** trace stores the pure visibility term; intensity
> applied once at apply, clamped. Both AO defaults dropped **1.5 → 1.0** (pure
> visibility; owner decision — raise via settings if a stronger look is wanted).
>
> **New settings keys (override the Parameters structs in `create()`, like the GI group):**
> - `Core/Graphics/RayTracing/AmbientOcclusion/` gained `Intensity` (1.0), `Bias` (0.005),
>   `MaxDistance` (2.0), `BlurRadius` (4), `NormalSigma` (0.5).
> - **First screen-space group** `Core/Graphics/ScreenSpace/AmbientOcclusion/`:
>   `Radius` (0.5), `Intensity` (1.0), `Bias` (0.025), `SampleCount` (32) — read by
>   `SSAO::create()` (which previously read no settings at all).
> - `Core/Graphics/ScreenSpace/GlobalIllumination/` (same day): `MaxDistance` (5.0),
>   `Intensity` (0.8), `Thickness` (0.5), `SampleCount` (8), `StepCount` (16),
>   `BlurRadius` (4), `DepthSigma` (1.0), `NormalSigma` (0.5) — read by `SSGI::create()`;
>   demos construct SSGI bare. The `ScreenSpace/` group mirroring `RayTracing/` is the
>   prerequisite for the RT/SS effect-pair factory idea.

### Fixed: RTR Shadow Rays + The "Reflections Must Match The Raster" Contract (Jul 2026)

> [!WARNING]
> **Porting the RTGI shadow-ray fix to RTR naively caused two REGRESSIONS** (caught in a
> reflection-heavy indoor scene): floor-tile reflections vanished, and reflections showed a
> phantom dark shape that did not exist on screen.
>
> **Root cause of both:** the shadow ray disagreed with the raster's shadowing model.
> 1. In raster, **a light without a shadow map deliberately shines through geometry**.
>    Shadow-raying ALL lights made reflected surfaces darker than the surfaces themselves,
>    and painted exact ray-traced shadows (of real occluders) that the raster never draws —
>    the phantom shape.
> 2. `computeDirectLighting()` had no ambient term (a hardcoded `vec3(0.15)` stood in),
>    so once the unshadowed leak was gone, ambient-lit reflected surfaces went black.
>
> **The contract (applies to every RT effect evaluating direct light at hit points):**
> **reflections/bounces must reproduce what the raster shows — no more, no less.**
> - Shadow rays are traced ONLY for lights that cast shadows in raster. The flag is
>   carried per light in the RT light SSBO's 4th vec4, slot `.z` (`LightSet.cpp` fill:
>   `isShadowCastingEnabled()`); both `RTR.cpp` and `RTGI.cpp` gate on it.
> - The reflection hit lighting includes the ACTUAL scene ambient
>   (`LightSet::ambientLightColor() × ambientLightIntensity()`, via RTR push constants).
>
> **Files involved:** `Scenes/LightSet.cpp` (flag), `Graphics/Effects/Framebuffer/RTR.{hpp,cpp}`
> (shadowRayVisibility + gating + ambient push constants), `RTGI.cpp` (gating).

### Fixed: removeStaticEntity() forgot the entity BEFORE unlinking its components — ghost lights, pure virtual crash (Jul 2026)

> [!WARNING]
> **Removing an entity that carries a light crashed the render thread with
> `pure virtual method called`.** Core-dump backtrace: `Core::renderingTask()` →
> `Scene::updateVideoMemory()` → `LightSet::updateVideoMemory()` →
> `Component::Abstract::getWorldCoordinates()` → `__cxa_pure_virtual`.
>
> **Root cause:** `Scene::removeStaticEntity()` called `this->forget(entity)` FIRST, then let the
> entity die. The component-destruction notifications (`DirectionalLightDestroyed`, ...) — the
> mechanism that unregisters lights from the `LightSet` — fired while the scene was no longer
> observing: they went into the void, the light stayed registered, and the render thread then
> dereferenced a destroyed component.
>
> **Fix:** `staticEntity->clearComponents()` BEFORE `this->forget()` — every component unlink
> notification is dispatched while the scene still listens.
>
> **Lesson:** an entity's components must be unlinked while its observers are still attached;
> `forget()` is the LAST step of a removal, never the first.

### LIFTED (IBL lot 3, Jul 2026): star-less skies now model objects; reflections follow the live sky

> [!NOTE]
> The two July 2026 limitations of the environment consumption are **gone**:
> - **A star-less sky models objects**: the ambient pass reads the baked diffuse irradiance
>   cubemap (reserved slot 1) by the world normal — verified live (Backrooms room, zero
>   analytic light: the model is shaded by the ceiling lights and pink walls).
> - **Reflections follow the LIVE sky**: `setReflectionComponentFromEnvironmentCubemap()`
>   never captured anything (it flags the bindless path); the shader now reads the
>   GGX-prefiltered cubemap (reserved slot 2) which is RE-BAKED at every background switch
>   (`Scene::updateEnvironmentIBL`). The old raw slot-0 mirror read is gone with it.
>
> ⚠️ Remaining sibling caution: the LEGACY `setReflectionComponent(texture)` (explicit
> texture) is still a static per-material capture — by design.
>
> SSR's ray-miss environment fallback (lot 4) also reads the bindless prefiltered slot
> (set 1 of its resolve pipeline, roughness-driven LOD) — its old dedicated `envCubemap`
> binding and the never-called `setEnvironmentCubemap()` are GONE, and
> `IndirectPostProcessEffect::recordFullscreenPass()` gained an optional bindless-set
> parameter any post effect can reuse.

### Fixed: SimplePass normal-mapped shader referenced an undeclared `N` — in BOTH quality levels (Jul 2026)

> [!NOTE]
> **Obsolete since the static-lighting removal (Jul 2026)**: the `SimplePass` is no longer
> remapped to a light-pass type (`checkRenderPassType()` is gone) — it is strictly unlit, and the
> `N` declaration guard covers the `AmbientPass` only. Kept for history; the two-quality-levels
> caution below remains fully valid.

> [!WARNING]
> **A `SimplePass` material with a normal map fails to compile the moment a post-process
> effect enables the normal G-buffer attachment.** Symptom seen on WaterWorld / BallsOfSteel:
> `Unable to compile shader 'RenderableInstanceSimplePassFragmentShader'`, then a cascade of
> `Unable to get ready the renderable instance` for every affected renderable — the scene
> renders nothing. Three distinct GLSL errors were hit, one per shading path:
> `'N' : undeclared identifier` (high-quality Blinn-Phong), `'N' : redefinition` (high-quality
> PBR), `'ViewTBNMatrix' : undeclared identifier` (low-quality Gouraud, any material).
>
> **Root cause:**
> - `SceneRendering` writes `svOutputNormal` for the `AmbientPass` **and** the `SimplePass`,
>   using `LightGenerator::finalNormalViewSpaceExpression()`, which returns the bare identifier
>   `N` (`= normalize(transpose(ViewTBNMatrix) * surfaceNormal)`) whenever normal mapping is on.
> - `N` was only declared in the `AmbientPass` branch of `generateFragmentShaderCode()`. But
>   `checkRenderPassType()` **remaps `SimplePass` to a light-pass type**, so a `SimplePass` never
>   entered that branch → `N` (and its `ViewTBNMatrix` dependency) was never declared.
> - **Latent** until then: with no post-process the normal attachment does not exist, so the
>   `svOutputNormal` write is never generated and the missing symbols are never referenced.
>
> **The quality trap (⚠️ the load-bearing subtlety):** high and low quality
> (`Core/Graphics/Shader/EnableHighQuality`, default `false`) route through **different
> generators**. In **high** quality PBR self-declares a two-sided-flipped `N` and Blinn-Phong
> does not; in **low** quality **every** material — PBR included — is shaded by the **Gouraud**
> generator, which declares neither `N` nor requests `ViewTBNMatrix`. So a fix validated in one
> quality level can still be broken in the other. The first fix attempt passed high quality
> then failed low quality on the exact same scene.
>
> **Fix (two guards, both quality-aware):**
> - `generateFragmentShaderCode()` declares `N` up-front for
>   `AmbientPass || (SimplePass && !(m_usePBRMode && highQualityEnabled()))` — everything except
>   the *only* self-declaring path, high-quality PBR (declaring it there would redefine `N`).
> - `generateVertexShaderCode()` synthesizes `ViewTBNMatrix` (`ToNextStage`) for
>   `SimplePass && !highQualityEnabled()` — the low-quality Gouraud vertex path is the only one
>   that does not request it itself. Keeps the G-buffer normal normal-mapped in both levels.
>
> **Files involved:**
> - `Saphir/LightGenerator.cpp:generateFragmentShaderCode()` — up-front `N` guard
> - `Saphir/LightGenerator.cpp:generateVertexShaderCode()` — `ViewTBNMatrix` request for low-Q SimplePass
> - `Saphir/LightGenerator.PBR.cpp` — high-quality PBR path self-declares `N`
> - `Saphir/LightGenerator.PerVertex.cpp` — Gouraud (low quality), declares no `N`
> - `Saphir/Generator/SceneRendering.cpp:561` — `svOutputNormal` write (AmbientPass || SimplePass)
> - See `src/Saphir/AGENTS.md` → "MRT normal output — the `N` declaration contract".
>
> **Verification:** WaterWorld covers all four combinations in one scene — Standard floor
> (`WaterWorldSceneFloor`) + PBR sea (`WaterWorldSea`) × high/low quality. Toggle
> `EnableHighQuality` and confirm zero shader errors in both.

### Fixed: Direct diffuse was missing the Lambertian 1/pi — Standard vs PBR disagreed by a factor of pi (Jul 2026)

**Symptom.** Sunlit ground rendered markedly brighter than the sky that lit it, and brighter than a
PBR surface beside it under the same light. Owner's words: "flashy as hell". On `water-world`, sand
under a 100000 lx sun reached ~28000 nits against an 8000-nit sky dome.

**Root cause.** Light intensities are ILLUMINANCE in lux, so a Lambertian surface emits
`albedo * E * cos(theta) / pi`. That `1/pi` existed in exactly ONE place in the whole Saphir
generator — the ambient pass (`LightGenerator.cpp`, `albedo * 0.3183098862`). The PBR generator had
its own (`LightGenerator.PBR.cpp`: `kD * albedo / 3.14159265`). The **legacy/Standard direct diffuse
had none**, so the two material models differed by pi (~3.14x) on direct lighting.

**Fix.** `LightGenerator::generateFinalFragmentOutput()` now builds a `diffuseIlluminance`
expression (`lightIntensity * 0.3183098862`) and uses it at all four diffuse emission sites (plain,
reflection, refraction, reflection+refraction).

> [!WARNING]
> The normalisation is deliberately **NOT** folded into `finaleDiffuseFactor`: that same expression
> is reused as a raw geometric N.L term inside the PBR low-quality specular `pow()` further down, and
> scaling it there would change the highlight exponent's input rather than its energy.

**Still open:** the legacy specular is not normalised either (see `TODO.md`) — that one changes the
look of every legacy material, so it is an owner decision.

### Fixed: the material-facing grab pass was 8-bit under an HDR scene, and its transmission was scaled twice (Jul 2026)

Two coupled defects on grab-pass transmission (water, glass — any `setTransmissionComponentFromGrabPass`).

**(a) Format.** `Renderer` allocated the grab with `swapChainCreateInfo.imageFormat` — an 8-bit
`B8G8R8A8_SRGB` — while the scene target it is blitted from is `R16G16B16A16_SFLOAT` under HDR.
`vkCmdBlitImage` therefore **clamped the scene radiance to [0,1]**, so every refraction sampled a
blown-out white image. Note the `PostProcessor` has its own, separate grab that always honoured HDR;
only the material-facing one did not.

Fixed by `Renderer::refreshGrabPass()`, which derives format and extent from the image the grab
actually copies (the scene target when post-processing is active, the swap chain otherwise). It is
called at renderer init, from both exits of `recreateSceneTarget()`, and on resize.

**(b) Unit.** With (a) fixed, the shallow water turned solid white. The PBR reflection+transmission
block scaled BOTH terms by `scaledIBLIntensity()` = `IBLIntensity * backgroundLuminance`. That is
correct for the reflection — a cubemap texel is normalized [0,1] and needs the sky luminance to
become a luminance — but **wrong for a grab-pass transmission, which is already the rendered scene
in nits**. It multiplied the scene by the sky luminance a second time. Worst at the shoreline, where
Beer's law absorbs almost nothing so the sunlit sand comes through undimmed.

Fixed by teaching the generator the unit of its source:
`LightGenerator::declareSurfaceTransmission(..., bool transmissionIsSceneRadiance)`, passed as
`m_isUsingGrabPassForTransmission` from `PBRResource`. When set, the transmission term skips the
luminance scale; the reflection term always keeps it.

> [!IMPORTANT]
> **The rule: scale by the environment luminance only what came out of the environment cubemap.**
> Anything read back from the rendered scene (grab pass, and by extension any screen-space capture)
> is already an absolute luminance.

### Fixed: the legacy specular was PHONG despite being named Blinn-Phong (Jul 2026)

**The trap.** `LightGenerator` dispatches the non-PBR paths to functions called
`generatePhongBlinnVertexShader` / `generatePhongBlinnFragmentShader` /
`generatePhongBlinnWithNormalMap*`. The names promised Blinn-Phong; the maths inside was plain
**Phong** — `R = reflect(rayDirection, N)` then `pow(max(dot(R, V), 0), shininess)`. There was no
half vector anywhere in the legacy path. Owner's verdict: "une vieille erreur".

**Two axes, both named after Phong — do not confuse them.**

| Axis | Options | Where it lives here |
|---|---|---|
| WHERE the lighting is evaluated | **Gouraud** (per-vertex colour) vs **Phong shading** (per-fragment, interpolated normals) | `highQualityEnabled()` — `Core/Graphics/Shader/EnableHighQuality`. `false` → `generateGouraud*`; `true` → `generatePBR*` / `generatePhongBlinn*` |
| WHICH specular formula (the BRDF) | **Phong model** (`R·V`) vs **Blinn-Phong model** (`N·H`) vs **Cook-Torrance GGX** | PBR materials → GGX; everything else → Blinn-Phong since this fix |

Blinn-Phong is **not** "Blinn plus Phong": it is Blinn's 1977 amendment of Phong's model, replacing
the reflected ray by the half vector `H = normalize(L + V)`. It says nothing about which shader stage
evaluates it — the two axes combine freely, and this engine offers all four combinations.

> [!IMPORTANT]
> `DefaultEnableHighQuality` is **`false`**, so a fresh install runs the **Gouraud** path. It is not
> dead code — verify changes to the legacy lighting in BOTH quality levels. Note also that the
> grab-pass transmission path is gated on high quality, so water renders flat and opaque with HQ off.

**Why the half vector, concretely.**
- `dot(R, V)` goes negative over a wide region at grazing angles and the highlight is **truncated
  along a hard edge**. `dot(N, H)` stays positive whenever light and eye are on the same side, so the
  falloff is continuous. Most visible exactly on the large flat surfaces this engine renders — ground
  planes and sea level.
- Phong's lobe is rotationally symmetric around `R`, so the highlight stays a **disc** at any viewing
  angle. The half-vector lobe **stretches** with obliquity, which is what real specular reflection does
  on a flat surface — the sun's glitter path on water.
- `N·H` makes the term a microfacet **normal distribution over H**, the same family as the PBR path's
  GGX, so `shininess` and `roughness` can be related. Parameterised around `R`, they cannot be — which
  is structurally why a `StandardResource` and a `PBRResource` could never agree under one light.

**Changed sites:** `LightGenerator.PerFragment.cpp` (view space, reuses `twoSidedN` / `twoSidedV`),
`LightGenerator.PerFragment.NormalMap.cpp` (**tangent** space — N, V and H all in tangent space),
`LightGenerator.PerVertex.cpp` (view space, computed in the vertex shader). The function names are now
truthful, so nothing was renamed.

> [!CAUTION]
> **Material shininess values are now wrong, in a specific direction.** `dot(N, H) > dot(R, V)` for the
> same geometry, so at an UNCHANGED `shininess` every highlight is **WIDER** than the Phong one it
> replaced. Verified live on `water-world`: the sand went visibly glittery. Rule of thumb — a Blinn
> exponent needs roughly **4x** the Phong one for the same visual width (`Grounds/desert001`'s 192
> wants something nearer 768). Tracked in `TODO.md`.
>
> **RESOLVED (Jul 2026)** — both the normalisation and the shininess retune were done in one pass,
> exactly as this note asked. See the next entry, "The legacy specular was not energy-normalised, and
> `Shininess` was authored as a glossiness". The retune did NOT become a per-file sweep: the manifest
> key was re-interpreted at the parse boundary instead.

### Fixed: the legacy specular was not energy-normalised, and `Shininess` was authored as a glossiness (Jul 2026)

**Two independent defects that compounded into one symptom** — every sunlit Standard surface read as
a uniform bright sheet, and the sky it stood under looked washed out by comparison. Diagnosed on
`basic-scenery` under `Clouds` (6000 nits dome, 50000 lx sun at 5000 K).

**Defect 1 — the term was a raw multiple of the ILLUMINANCE.** The legacy specular was emitted as
`specularColor * illuminance * pow(max(dot(N, H), 0), shininess)`: no `(n+2)/(8*pi)` normalisation
and, less obviously, **no cos(theta)** either — `SpecularFactor` was multiplied by `LightFactor`
(shadow/attenuation) but never by `N.L`. Its diffuse sibling had carried the Lambertian `1/pi` since
the fix two entries up, so the two terms of the SAME material were on different scales, and neither
could be compared to a light authored in lux or candela.

**Defect 2 — the manifest key was never an exponent.** `"Shininess"` is consumed directly as the
Blinn-Phong exponent, but the data store was authored as a perceptual glossiness in `[0,1]`:

| Authored value | Files |
|---|---|
| `0.1` | **3834** of 3917 |
| `0.5` | 37 |
| `0.9` | 29 |
| `2.0` / `9.0` / `10.0` / `20` / `160` | 13 |

An exponent of `0.1` never decays: `pow(0.8, 0.1) = 0.978`, so the lobe covers the whole hemisphere
and every surface behaves as a uniform mirror sheet. **Measured on the sand** (`Grounds/Dust001`,
albedo 0.27, specular grey 0.5, shininess 0.5) under that sun:

| Term | Value |
|---|---|
| Diffuse `0.27 * 35031 / pi` | 3 011 nits |
| Specular `0.5 * 50000 * pow(0.8, 0.5)` | **22 350 nits** |
| The sky's own bright cloud | ~4 000 nits |

The ground outshone the sky by 5x, so the auto-ISO stopped down to hold it, and the sky lost its
substance. That is the whole "flashy ground / flat sky" signature.

**The fix, and where each half lives.**

| Half | Site | Note |
|---|---|---|
| Energy normalisation + `N.L` | `LightGenerator.PerVertex.cpp`, `.PerFragment.cpp`, `.PerFragment.NormalMap.cpp` | `pow(N.H, n) * ((n + 2) / (8*pi)) * DiffuseFactor` — `DiffuseFactor` already carries `N.L * LightFactor`, so the attenuation is applied exactly once. `TODO.md` proposed scaling next to `finaleSpecularFactor` in `generateFinalFragmentOutput()` instead; the factor sites were chosen because the exponent AND `N.L` are both in scope there. |
| Glossiness -> exponent | `StandardResource::specularExponentFromGlossiness()`, called ONLY from the two JSON parse sites | `exp2(1 + 10 * gloss)` (UE3 convention): `0.0 -> 2`, `0.1 -> 4`, `0.2 -> 8`, `0.4 -> 32`, `0.5 -> 45`, `0.9 -> 1024`. |

> [!CAUTION]
> **The remap belongs to the JSON boundary and NOWHERE else.** The C++ API carries real exponents:
> `DefaultShininess` is `32`, `MaxPBRShininess` is `128`, and `setRoughness()` feeds `setShininess()`
> through `pow(1 - roughness, 2) * 128` (roughness 0.5 -> 32). Remapping inside `setShininess()` would
> turn that 32 into `exp2(321)`. For the same reason the absent-key fallback had to become
> `DefaultGlossiness{0.4F}`, which maps BACK to 32 — using `DefaultShininess` there would have been
> the same bug with a different trigger.
>
> **Do not remap twice.** A value read from a manifest is a glossiness; a value held by a
> `StandardResource` or reaching the shader uniform is an exponent. The 13 files that legitimately
> carried exponents were converted to glossiness in the data store (`gloss = (log2(n) - 1) / 10`), so
> the key now has exactly ONE meaning everywhere.

**Verified.** `basic-scenery`, ground + sky only, controlled camera. The rendered ground/sky mean
luminance ratio landed at **1.64**, against **1.65** computed independently from the manifest
(ground 3 070 nits = 3 011 diffuse + 59 specular; sky mean = linear(`AverageColor` 0.31) x 6000 =
1 860 nits). The ground stopped clipping (band max 238 -> 194) and the sky mean ROSE 53 -> 79,
because the exposure no longer had to absorb a 22 000-nit ground.

> [!NOTE]
> Still unnormalised, deliberately out of scope here: the **PBR low-quality specular approximation**
> in `LightGenerator.cpp` (`lqSpecPower`), which multiplies the raw illuminance and reuses `N.L`
> inside its own `pow()`. Tracked in `TODO.md`.

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

### Critical: Resource getOrCreateResource Lambdas Run on Loading Threads — Capture By VALUE

> [!CRITICAL]
> The loader lambdas passed to `Resources::Container::getOrCreateResource()` may execute
> **asynchronously on the resource manager's loading threads**, after the calling scope has
> returned. Capturing a local buffer by reference is a use-after-free: symptoms range from
> "The manual loading function has return an error !" spam (garbage data failing validation)
> to hard segfaults. **Move buffers into the lambda** (`[pixels = std::move(rgba)]`,
> `[shape]`, `[geometry, materialList, ...]`). Caught while writing `AssetLoaders::WADLoader`
> (Jul 2026); GLTFLoader/FBXLoader already follow the rule — keep it that way.

## Scene Rendering

### Fixed: IntermediateRenderTarget VK_DEPENDENCY_BY_REGION_BIT → stale-frame block corruption in motion (Jun 2026)

> [!CRITICAL]
> **Symptom:** with any framebuffer post-process effect enabled (RTGI, RTR, RTAO, volumetric light,
> bloom…), the image showed a grid of hard blocks arranged on a diagonal — looking like H.264
> macroblocks / "blocks from the previous frame". **Only visible while the camera moved**; perfectly
> stable (invisible) when static. Identical on every RTX card. Unaffected by GI sample count, the
> noise hash, or temporal accumulation — because it was **not** a shading/denoising artifact.
>
> **Root cause:** `IntermediateRenderTarget::createRenderPass()` declared its subpass→external (and
> external→subpass) dependency with **`VK_DEPENDENCY_BY_REGION_BIT`**. A by-region dependency only
> orders the SAME (x,y) framebuffer tile between the write and the subsequent read — valid only for a
> strict 1:1 passthrough. But every effect that consumes an IRT samples it **non-locally**: the
> bilateral blurs read a neighbourhood, the volumetric light marches radially, temporal reprojection
> reads a far reprojected UV. With by-region, a neighbouring/reprojected tile **may not be written
> yet** when sampled, so the read returns the previous frame's residual (the IRT loads with
> `LOAD_OP_DONT_CARE`, so old content lingers), in the GPU's diagonal tile order → oblique blocks of
> stale frame-N-1 content. Invisible when static (frame N ≡ N-1), visible in motion.
>
> **Fix:** drop `VK_DEPENDENCY_BY_REGION_BIT` from both IRT subpass dependencies (`dependencyFlags = 0`).
> A full (non-by-region) write→read dependency makes the **entire** write complete before any read.
> Fixes the whole framebuffer-effect class at once (shared IRT).
>
> **Takeaway:** `VK_DEPENDENCY_BY_REGION_BIT` is ONLY valid when the consumer reads the exact same
> pixel it processes. Any blur / gather / reprojection / radial read MUST use a full dependency.
> This class of bug reads "previous-frame blocks in a diagonal pattern, only in motion" — that is a
> read-before-write-complete **synchronization hazard**, not a shading artifact. Confirm with the
> Vulkan **Synchronization Validation** layer (it flags `SYNC-HAZARD-READ-AFTER-WRITE`).
>
> **File:** `Graphics/IntermediateRenderTarget.cpp::createRenderPass()`.

### Fixed: Bindless texture manager was not multi-scene — stale/freed slots on scene switch (Jun 2026)

> [!CRITICAL]
> **Symptom:** after loading two scenes and deleting one (even the inactive one), the log spams
> `[Error][BindlessTextureManagerService] Invalid raw descriptor info !` every frame.
>
> **Root cause:** `BindlessTextureManager` is a single global service, but scene code
> (`SceneMetaData`, light emitters, `Scene::enable`) registered textures **directly** into it
> and stored raw slot indices. `~SceneMetaData` never unregistered its slots, lights
> (`~AbstractLightEmitter = default`) never unregistered theirs, and nothing released a scene's
> slots on disable. The manager had no notion of which scene owned which slot, so a deleted
> scene left dangling descriptors that the per-frame refresh kept re-writing.
>
> **Fix — per-scene `Scenes::BindlessTextureSet` (the bindless analogue of `LightSet`):**
> - Each scene owns a `BindlessTextureSet` describing its textures and allocating its own
>   dynamic slots (from `FirstDynamicSlot`). Scene code registers into the **set**, never the
>   manager.
> - The manager only READS the **active** scene's set, via `syncTextureSet(set, sceneTimeMS)`
>   called by the `Renderer` each frame right after `Scene::prepareRender`. It writes the
>   descriptor table, performs the animated-texture per-frame view swap, and writes the
>   environment-cubemap reserved slot (engine-default fallback when the scene has none).
> - The manager's dynamic `registerTexture*/unregisterTexture*` and free-lists were removed.
>   `updateTexture*` (reserved-slot writes) stay for the Renderer (grab pass, default env, IBL).
> - Slots are scene-local → table capacity is the largest scene, not the sum.
>
> **Second pass — clearing the GPU descriptor table on scene disable (the `vkDestroySampler` part):**
> mirroring only the active set is not enough. When a scene stops being active (loading another
> demo disables the previous one), its descriptors stay in the **global** table until overwritten.
> A later `deleteScene` then destroys that scene's samplers/images while the descriptor set still
> references them → `VUID-vkDestroySampler-sampler-01082` ("currently in use by VkDescriptorSet")
> plus `Invalid texture descriptor info !` spam. Fix: `Scenes::Manager::disableActiveScene()` calls
> `BindlessTextureManager::clearTextureSet(scene.bindlessTextureSet())` under the exclusive lock,
> which overwrites each of that scene's freed dynamic slots with an engine-owned dummy (2D → dummy
> color-projection 2D, cube + reserved env slot → default cubemap). After that the leaving scene
> has zero descriptor references. **`clearTextureSet` does NOT waitIdle** — disable/switch stays
> hitch-free (overwriting bound descriptors mid-flight is safe via UPDATE_AFTER_BIND, and the
> dormant scene's textures stay alive). The drain that protects destruction lives in
> `Scenes::Manager::deleteScene`: `device->waitIdle()` before `m_scenes.erase`, so a delete (active
> or inactive) is safe. **Known gap:** cube-array slots (animated cubemap gobos) have no
> engine dummy and are left untouched — rare, add a dummy cube-array if a scene uses them.
>
> **Files:** `Scenes/BindlessTextureSet.{hpp,cpp}` (new), `Graphics/BindlessTextureManager.{hpp,cpp}`,
> `Scenes/SceneMetaData.{hpp,cpp}`, `Scenes/Component/AbstractLightEmitter.{hpp,cpp}` (+ Directional/Point/Spot),
> `Scenes/Scene.{hpp,cpp}`, `Scenes/Scene.rendering.cpp`, `Graphics/Renderer.cpp`. See
> `src/Graphics/AGENTS.md` §6 "Bindless Textures Manager".

### Fixed: bindless table sizes were hardcoded — 5x over the device budget on macOS (Aug 2026)

> [!CRITICAL]
> **Symptom:** with the validation layers on, **no scene loads** —
> `VUID-VkPipelineLayoutCreateInfo-descriptorType-03022` / `-pSetLayouts-03036`, "sampler bindings
> count (4933) exceeds maxPerStageDescriptorUpdateAfterBindSamplers (1024)". The post-process stack
> fails first and takes the whole act down.
>
> **Root cause:** the five arrays were `static constexpr` (4928 `COMBINED_IMAGE_SAMPLER`
> descriptors) and the device limits were **never queried**. Full analysis, including why 4933 and
> not 4928, in [`docs/troubleshooting.md`](../docs/troubleshooting.md) → macOS / MoltenVK.
>
> **Fix:** `BindlessTextureManager::computeCapacities()` resolves the capacities at initialization
> from `PhysicalDevice::propertiesVK12()`. `Scene`'s constructor pushes them into its
> `Scenes::BindlessTextureSet` via `setCapacities()` — otherwise the set would hand out a slot beyond
> the table and the manager's bounds check would silently drop the texture.
>
> **Rule:** bound a slot index with `manager.maxTextures2D()` and friends, **never** with
> `DesiredMaxTextures2D`. The generated GLSL declares unbounded arrays
> (`Declaration::Sampler::UnboundedArray`), so capacities never leak into shader code — keep it that
> way. Startup logs the effective table; read it before blaming a missing texture on anything else.
>
> **Files:** `Graphics/BindlessTextureManager.{hpp,cpp}`, `Scenes/BindlessTextureSet.{hpp,cpp}`,
> `Scenes/Scene.cpp`.

### Fixed: Acceleration structure builder was per-scene via a global static (Jun 2026)

> [!CRITICAL]
> **Symptom (multi-scene):** reloading/deleting a scene caused BLAS/TLAS build errors and a GPU
> stall — geometries silently stopped getting their BLAS.
>
> **Root cause:** `Vulkan::AccelerationStructureBuilder` was created **per scene** (in
> `SceneMetaData`) and published through a **global static** pointer
> `Geometry::Interface::s_accelerationStructureBuilder`. But BLAS are built for **shared
> geometries** (which outlive any scene). Deleting a scene ran
> `setAccelerationStructureBuilder(nullptr)`, clobbering the builder the *active* scene still
> needed → `Interface::buildAccelerationStructure` saw null and skipped → geometries without BLAS
> → invalid TLAS instances → GPU hang.
>
> **Fix:** the builder is now a **single Renderer-owned instance** (`Renderer::m_accelerationStructureBuilder`,
> created at init when RT is enabled, exposed via `Renderer::accelerationStructureBuilder()`).
> The global static was **removed**: `Geometry::Interface::buildAccelerationStructure` fetches the
> builder from `serviceProvider().graphicsRenderer()`, and `SceneMetaData` borrows it (non-owning)
> for TLAS / buffer addresses. The builder is stateless infra (command pool + fence + fps, mutex-
> guarded) and the BLAS it returns is owned by the geometry, so a single shared instance is safe.
>
> **Files:** `Graphics/Renderer.{hpp,cpp}`, `Graphics/Geometry/Interface.{hpp,cpp}`,
> `Scenes/SceneMetaData.{hpp,cpp}`, `Scenes/Scene.cpp`.

> [!IMPORTANT]
> **Systemic pattern (the five Jun 2026 fixes share one root):** a resource that is **shared or
> global** had its *ownership / lifecycle* wired **per-scene** (or via a global static):
> view-matrices on camera connect, bindless textures, the cached sampler, the AS builder — or a
> renderer-global service (the PostProcessor) that kept running after its scene was deleted. The
> engine was built and tested single-scene; multi-scene load/switch/delete exposes these. **Rule
> of thumb:** anything shared across scenes (samplers, pipelines, programs, layouts, the AS
> builder, the bindless table) is owned ONCE at the Renderer/device level; per-scene objects only
> *borrow* or *describe* (cf. `LightSet`, `BindlessTextureSet`). Avoid global statics for these —
> reach the owner through the renderer. Audit new singletons/statics against multi-scene teardown.

### Fixed: Texture resources destroyed the SHARED cached sampler (Jun 2026)

> [!CRITICAL]
> **Symptom (multi-scene):** deleting an inactive scene spammed
> `Invalid texture descriptor info` for EVERY texture of the *active* scene, plus
> `VUID-vkDestroySampler-sampler-01082`. Only a handful of distinct samplers were destroyed for
> dozens of broken textures — the tell-tale sign of a shared sampler.
>
> **Root cause:** samplers come from a **renderer-level cache** keyed by kind
> (`Renderer::getSampler("Texture2D", …)` → `m_samplers`), so every `Texture2D` shares ONE
> `Vulkan::Sampler`; the cache owns it and destroys it at renderer shutdown. But each texture
> type's cleanup (`Texture1D/2D/3D`, `TextureCubemap`, `AnimatedTexture2D`,
> `AnimatedTextureCubemap`) did `m_sampler->destroyFromHardware()` before resetting. When a few
> textures unique to the deleted scene unloaded (refcount 0), they destroyed the **shared**
> sampler from hardware — instantly invalidating it for every other texture still using it (and
> leaving the bindless descriptor set referencing a dead `VkSampler`).
>
> **Fix:** texture cleanup now only **releases its reference** (`m_sampler.reset();`) — it must
> NOT destroy a cache-owned, shared object. The cache (`Renderer::m_samplers`) remains the sole
> owner and destroys every sampler once, at shutdown.
>
> **Takeaway:** anything obtained from a shared cache (`getSampler`, and by analogy pipelines,
> programs, layouts) is reference-held, not owned — drop the smart pointer, never call
> `destroyFromHardware()` on it.
>
> **Files:** `Graphics/TextureResource/{Texture1D,Texture2D,Texture3D,TextureCubemap,AnimatedTexture2D,AnimatedTextureCubemap}.cpp`.

### Tooling: automatic GPU fault dump on DEVICE_LOST (Jun 2026)

> [!NOTE]
> **When you hit `VK_ERROR_DEVICE_LOST`, read the `DEVICE LOST (...) — GPU diagnostics follow:`
> block in the log first.** The engine now auto-dumps GPU fault info at every loss site
> (`Device::dumpDeviceLostDiagnostics`, called from `Queue`/`Fence`/`Device` — reported once per
> device). It combines `VK_EXT_device_fault` (faulting addresses; Mesa/AMD/Intel) and
> `VK_NV_device_diagnostic_checkpoints` (last GPU region reached; NVIDIA).
>
> **The checkpoint marker is the smoking gun** — it names the GPU command region that was executing
> when the device died, NOT the CPU call that observed the loss (which is almost always innocent).
> Markers are placed at `AS-build:begin/:end` and `transfer:image-layout-transition`. To blame a new
> region, drop a `Device::setCheckpoint(cmdBuf, "literal")` there (string literal only — read back
> after the loss). On the NVIDIA proprietary driver, `VK_EXT_device_fault` is **absent**, so
> checkpoints carry the diagnosis. See `src/Vulkan/AGENTS.md` → *GPU device-lost diagnostics*.

### Fixed: BLAS builds raced buffer uploads across transfer queues → DEVICE_LOST at load (Jul 2026)

> [!CRITICAL]
> **Symptom:** intermittent `VK_ERROR_DEVICE_LOST` (`Xid 109 CTX SWITCH TIMEOUT`, checkpoint
> markers straddling `AS-build:begin/end`) while loading streaming-heavy scenes, with **zero**
> validation-layer errors. Probability rose with load-phase framerate (deterministic with the
> GI effect disabled) and streaming density (foliage) — the second, independent cause of the
> historical "intermittent Sponza DEVICE_LOST" (the first was the runtime-destruction
> use-after-free family, fixed by `Vulkan::DeferredDestructor`).
>
> **Root cause:** buffer uploads are submitted round-robin across the transfer family's
> queues (2 on the reference GPU), asynchronously. `AccelerationStructureBuilder::buildBLAS()`
> guarded against pending uploads by waiting `waitIdle()` on ONE `getGraphicsTransferQueue()`
> — itself round-robined. Whenever the vertex/index upload of the geometry being built sat on
> the sibling queue, the build read mid-DMA data: garbage triangles can stall the GPU's
> acceleration-structure unit past the kernel context-switch watchdog. Validation layers
> cannot see it (device-address reads, memory content).
>
> **Fix:** `Device::waitTransferQueuesIdle()` — waits EVERY queue of the transfer
> configuration (graphics fallback when no dedicated family) — used by `buildBLAS()`.
> **Rule:** never assume a `waitIdle()` on a round-robined queue covers a previously
> submitted operation; wait the whole family or track the operation's own fence.
>
> **Acceptance:** the two deterministic repros (GI disabled / GI 4 spp, glTF Sponza+extras,
> no frame cap — ~100% fault before) pass 6/6 clean.

### Fixed: GPU use-after-free on runtime destruction → intermittent DEVICE_LOST / segfaults (Jul 2026)

> [!CRITICAL]
> **Symptom:** intermittent `VK_ERROR_DEVICE_LOST` (kernel `Xid 109 CTX SWITCH TIMEOUT`,
> engine checkpoint marker `AS-build:begin`) while loading streaming-heavy scenes (Sponza +
> foliage extras), plus occasional SIGSEGV at load with validation errors
> `VUID-vkFreeDescriptorSets-pDescriptorSets-00309` (set freed while in use) and
> `VUID-vkDestroyBuffer-buffer-00922` (buffer in use by a descriptor set). Frequency depended
> on machine load (desktop compositor contention widened the race window).
>
> **Root cause:** several code paths destroyed GPU-visible objects **in place at runtime**
> while in-flight command buffers still referenced them:
> 1. `SceneMetaData`: when the RT instance list became transiently empty during streaming,
>    the live TLAS and every retired build request were destroyed immediately.
> 2. `SceneMetaData`/TLAS retirement: the retired-request deque was capped **by count**
>    ("keep at most 3"), not by elapsed frames — rebuild bursts under-covered the
>    frames-in-flight window.
> 3. `PostProcessor::configure()` relied on a mid-frame `vkDeviceWaitIdle()` before freeing
>    its descriptor sets and grab pass (GPU stall; proceeds on device-loss errors).
> 4. `Renderer::recreateSceneTarget()` destroyed the previous scene target **in place** when
>    the scene's post-process stack became ready (swap-chain-format target → HDR target):
>    its view-matrices descriptor set + UBO were freed while in-flight frames still used
>    them — the deterministic load-time segfault (`VUID-vkFreeDescriptorSets-…-00309` +
>    `VUID-vkDestroyBuffer-…-00922`, always the same early handles).
>
> **Fix:** central `Vulkan::DeferredDestructor` owned by the `Renderer` — frame-stamped
> retirement queue, drained after `framesInFlight` render ticks, flushed at terminate.
> All three sites migrated; the scene-target retirement vector was unified into it as well.
> **Contract:** any new runtime destruction of GPU-visible objects MUST go through
> `Renderer::deferredDestructor()`. See `src/Vulkan/AGENTS.md` ("Deferred destruction
> contract").

### Fixed: PostProcessor composited with no active scene → device lost (Jun 2026)

> [!CRITICAL]
> **Symptom:** deleting a post-processed scene (e.g. the `citadel` demo: SSR/RTAO/Bloom/FXAA…)
> crashed with `VK_ERROR_DEVICE_LOST` (SIGABRT in the fence wait). The validation log — once
> Vulkan objects were named (see `src/Vulkan/AGENTS.md`) — showed
> `VUID-vkCmdBeginRenderPass-initialLayout-00900` (PRESENT_SRC vs COLOR_ATTACHMENT) and
> `VUID-vkCmdDrawIndexed-None-08114` with `PostProcessorService-…-Descriptor` `uPrimarySampler`
> = imageView `0x0`.
>
> **Root cause:** the `PostProcessor` is a **renderer-global** service, enabled by the demo
> (`renderer.postProcessor().enable(true)`, `Builtin/AbstractDemo`) and **never disabled on scene
> delete** — so `isEnabled()` stayed true after the scene was gone. Both render paths then ran the
> final composite. `Renderer::renderFrameDirect` gated its in-place post-process block **only** on
> `m_postProcessor.isEnabled()`, regardless of the active scene: with no scene it did
> `recordBlit` + composite against a destroyed/null primary-sampler image (the deleted scene's
> effect output, e.g. `FXAASharpenOutput`) with an incompatible render-pass layout → broken
> command buffer → device lost.
>
> **Fix:** the composite only runs for an **active scene**. `renderFrameDirect`'s post-process
> block is gated on `scenePtr != nullptr`; the `renderFrame` dispatch takes the internal-target PP
> path only when `scene != nullptr`. With no scene, a scene-less frame falls through as a plain
> cleared (black) frame, exactly like a non-post-processed scene. `Scenes::Manager::deleteScene`
> also now holds the exclusive active-scene lock across `device->waitIdle()` + erase, so the
> render thread can't submit a frame referencing the scene while `~Scene` runs.
>
> **Takeaway:** a renderer-global service that *records GPU work* (not just holds resources) must
> **only run for the ACTIVE scene** — its inputs belong to a scene that may be gone. Mirror the
> active scene like the bindless table; never key the work on a "user enabled" master switch alone.
>
> **Files:** `Graphics/Renderer.cpp` (`renderFrame`, `renderFrameDirect`), `Scenes/Manager.cpp`
> (`deleteScene`). See [`multi-scene-resource-ownership.md`](multi-scene-resource-ownership.md)
> anti-pattern #5.

 ### Fixed: View-matrices freed on camera disconnect — render-thread use-after-free (Jun 2026)

> [!CRITICAL]
> **Crash signature:** segfault in `Vulkan::DescriptorSet::handle()` (`return m_handle;`),
> called from `RenderableInstance::Abstract::render()` →
> `renderTarget->viewMatrices().descriptorSet()->handle()`, on the **rendering thread**, when
> unloading a demo/scene after interacting with it.
>
> **Root cause:** each render target created its view-matrices GPU resource (UBO + descriptor
> set) on **camera connect** (`onInputDeviceConnected`) and **destroyed it on camera disconnect**
> (`onInputDeviceDisconnected` → `m_viewMatrices.destroy()`). The rendering thread synchronizes
> scene swaps only through `Scenes::Manager::m_activeSceneSharedAccess` (shared lock for
> render/logic, exclusive for scene enable/disable). Camera teardown reaches the swap-chain
> through a **different** channel — `CameraDestroyed` → `AVConsole::removeVideoDevice` →
> `disconnectFromAll` → `SwapChain::onInputDeviceDisconnected` — that does **not** take that
> lock. Destroying the primary camera while the scene was still active (e.g. projet-alpha
> `Stage::unloadActiveAct` resets the player Act *before* calling `disableActiveScene`) freed the
> swap-chain descriptor set under the render thread → `descriptorSet()` returned `nullptr` →
> crash.
>
> **Fix:** the view-matrices resource lifecycle now belongs to the **render target**, not the
> input camera:
> - Created in `RenderTarget::Abstract::createRenderTarget()`, destroyed in `destroyRenderTarget()`.
> - `onInputDeviceConnected` / `onInputDeviceDisconnected` overrides removed from SwapChain,
>   Texture, ShadowMap, View, SceneRenderTarget (base no-op used); camera connection only feeds
>   matrix **data** via `updateDeviceFromCoordinates()`.
> - The main `DescriptorPool` is now created **before** the swap-chain in `Renderer::onInitialize()`
>   (the swap-chain creates its view matrices at init time and needs the pool ready).
>
> **Files:** `Graphics/RenderTarget/Abstract.cpp`, `Graphics/Renderer.cpp` (init order),
> `Vulkan/SwapChain.{hpp,cpp}`, `Graphics/SceneRenderTarget.{hpp,cpp}`,
> `Graphics/RenderTarget/{Texture,ShadowMap,View}.hpp`. See
> [`docs/render-targets.md`](render-targets.md) → "View Matrices Lifecycle".

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

### RESOLVED: Sub-pixel projection jitter raced the single-buffered view UBO (Jul 2026)

**Symptom.** With TAA enabled, a **perfectly static camera** produced a visibly vibrating
image — in a static interior scene (Sponza), starting on the very **first** rendered frame,
before any temporal history existed.

**Why code review could not find it.** Every verifiable path read correct: jitter sign, the
`prepareFrameJitter` → UBO/SSBO upload → record → `archiveStateAfterRendering` ordering, the
scene/swap-chain view sharing (`setSourceViewMatrices`), the SSBO staging *and* upload inside
the same `prepareRender`, `ClipPositionCurrent` deliberately independent from `gl_Position`,
and `stageEntry()` falling back to the current model matrix on the first frame. A CPU trace
of the two view-projection matrices, each stripped of its own frame's jitter, reported
`maxAbs(A - B) == 0` on **683 consecutive frames**. Static reasoning therefore concluded
"the velocity must be exactly zero" — and it was wrong, because the race is between a CPU
write and a GPU read of the **same** memory, which no amount of source reading reveals.

**Root cause.** The jitter was written into the projection matrix of the **single-buffered**
view UBO (see `src/Graphics/AGENTS.md` § 16 Rule 4). Scene shaders on the advanced-matrices
path build `MVP = ubView.projectionMatrix * pcMatrices.viewMatrix * model`, so the raster
could use frame N±1's jitter while the velocity outputs subtracted frame N's jitter (read
from the correctly per-frame `InstanceTransforms` SSBO header).

**Diagnostic signatures — reuse these.**
- **Residual proportional to the image gradient** `|∇I|` (a gradient-magnitude-looking
  difference map) means the image is being **displaced** sub-pixel, not noised. Noise does
  not correlate with gradients; ghosting is confined to silhouettes.
- **Velocity uniform across every depth** in the frame ⇒ it is a **constant in NDC space**,
  so it cannot be camera motion (which is depth-dependent through parallax). For a static
  camera the only NDC constant available is a jitter delta.
- A **single bilinear tap** used to "unjitter" the current frame is correct only to first
  order — its barycentre is exact (`(1-d)(n-d) + d(n+1-d) = n`), so its error is a
  phase-varying *blur* (∝ Laplacian), never a displacement. If you measure a displacement,
  the unjitter tap is not your culprit.

**Measurement method (works without RenderDoc).** Park the camera, take N screenshots
several seconds apart, and compute the **per-pixel temporal peak-to-peak** over the series;
compare A/B with the feature off. Amplitudes measured here, in 8-bit luma units:
TAA off `0.11` mean / `2.4` p99.9 → TAA on `2.1`–`3.8` mean / `56`–`99` p99.9. **Always run
the same configuration twice** — two identical runs differed by ×1.85 here, and any
conclusion inside that envelope is noise. Isolate terms by forcing them out one at a time
(`prevUV = vUV` kills the reprojection; `alpha = 1` kills history and reprojection entirely,
leaving only the current-frame term — that is the configuration that reproduces a
first-frame vibration). Emitting a buffer *as the resolved colour* turns a screenshot into a
buffer visualisation; a channel carrying a boolean (`isnan`, `> epsilon`) survives tone
mapping intact, raw magnitudes do not.

**Resolution (applied 2026-07-25).** The jitter is now a **per-draw push constant**
(`PushConstant::Component::ProjectionJitter`, a `vec2` after the pushed view/view-projection
matrix) applied to `gl_Position` alone:

```glsl
gl_Position = MVP * vec4(position, 1.0);          /* every matrix is UNJITTERED */
gl_Position.xy += pcMatrices.projectionJitter * gl_Position.w;
```

A push constant is recorded per draw, hence per-frame **and** per-render-target by
construction — shadow maps, RTT and cubemaps push zero because only the main view sets a
jitter. Nothing else changed conceptually: `ViewMatrices2DUBO::updateVideoMemory()` no longer
writes a jittered projection, `archiveStateAfterRendering()` archives the **clean** one, the
`InstanceTransforms` SSBO header lost its `projectionJitters` member (144 → 128 B), and the
velocity clip positions need no subtraction at all.

**The invariant to preserve: NO matrix ever carries the jitter.** It is the whole reason the
velocity outputs are correct, and it is easy to break — three code paths bake a CPU-computed
matrix into their push constants and must therefore stay jitter-consumers only (they output
no velocity): MDI, the MVP fallback, and the 132 B advanced fallback (which, having no room
for a jitter member, is simply rasterized unjittered — assumed limit).

**Lockstep is not optional.** The push block layout is declared in
`Saphir::Generator::Abstract::declareMatrixPushConstantBlock()`, read in
`VertexShader::isProjectionJitterPushed()`, and written in `RenderableInstance::Unique`/
`Multiple`. Adding the member to a branch without updating the two others is a *silent*
defect when the shader reads uninitialized push memory (no validation error), and a hard
`glslang` error (`'projectionJitter' : no such field`) when the shader reads a member the
generator did not declare — that second failure mode is how the miss got caught here, on the
`RenderableInstanceSimplePassVertexShader` (the classic VP-pushed path). Grepping the
generated GLSL for declaration-vs-use is the cheap exhaustive check (see
`ShowSourceCode` in the measurement doc).

**Residual after the fix** (same protocol, Sponza, static camera): `0.29` mean / `12.9` p99.9
with TAA on, against `0.11` / `2.4` with TAA off — a factor ~8 better than the broken state.
Follow-up measurement corrected the reading of that gap: with ray tracing disabled the raster
is **bit-stable** (peak-to-peak exactly `0.000` with TAA off), so the `0.11` was the RT
effects' own temporal noise, not a floor. On that clean bench the TAA residual is `0.195` on
Sponza and `0.021` on a plain box room — **content-driven**, not a leftover of this race. See
`TODO.md` § "TAA" for the state of the art applied to the resolve (measurement-neutral at the
8-bit capture floor). That `0.195` was inspected and judged acceptable to the eye by the owner
on 2026-07-25 — it is the ACCEPTED baseline of a converged TAA on this content, not an open
defect.

> **Takeaway:** a value that becomes frame-varying silently promotes its container to
> Rule 1 (per-frame copies). The jitter did not break TAA by being wrong — it broke TAA by
> being *new frame-varying data in an old shared buffer*, and it took the rest of the
> motion-vector chain down with it, including consumers (RTGI) whose own history validation
> hid the damage instead of reporting it.

### A velocity buffer has a floating-point noise floor — do not amplify it (Jul 2026)

**Symptom.** The motion blur, whose shutter angle multiplies the per-frame velocity, turned a
perfectly static camera into a whole-frame shimmer as soon as the angle was allowed above 1.

**Mechanism.** The velocity output is `(current.xy / current.w) - (previous.xy / previous.w)`,
and the two clip positions come from two DIFFERENTLY COMPUTED matrix products — the current one
from a pushed view-projection, the previous one from the SSBO header. Mathematically equal on a
static camera, they are not BIT-equal: the rounding leaves ~1e-7 NDC, i.e. ~1e-4 px. Harmless
until something multiplies it: a 1/60 s exposure at 500 fps is a shutter angle of 8.3, and a
64 px tile then spreads whatever crossed the gate over a 192x192 px neighbourhood — the whole
frame.

> **Takeaway:** any consumer that AMPLIFIES the velocity must gate on the **raw per-frame**
> magnitude, before its own scale factor, not after. A post-scale threshold moves with the
> amplification and will always be crossed eventually. Sub-quarter-pixel motion per frame is
> below the raster's own precision, so a dead zone there costs nothing real.
>
> Corollary for measurement: a probe that reports "nothing above 0.5 px" does NOT license
> "nothing above 0.06 px". Ask the probe the question you actually need answered.

**Fix.** `MotionBlur`'s horizontal reduction rejects raw velocities below
`Parameters::deadZonePixels` (0.25 px) before applying the shutter angle.

### A STRUCTURAL matrix mismatch does not cancel on a static camera (Jul 2026)

**Symptom.** With a perfectly frozen camera, the velocity buffer was exactly zero everywhere
except one trapezoid at the top of the screen, carrying a smooth NDC-position-like gradient up
to ~0.3 NDC — the sky, seen through Sponza's translucent skylight glass.

**Two wrong diagnoses before anyone measured.** Both sessions blamed the *translucent pass*,
because the glass and the sky occupy the same screen region and translucency is a plausible
culprit (blend state, write masks, draw order). Reading the generated GLSL showed the
translucent path was byte-identical to the opaque one. The actual discriminator took one
minute: a **boolean velocity probe** (emit `length(velocity) > 1e-6` as the resolved colour,
`return` early) drew the offending surface as a shape — and the shape was the sky, not the glass.

**Root cause.** Renderables using the infinity view (`isUsingInfinityView()`) build their
CURRENT clip position from the pushed **translation-free** view, while their PREVIOUS one came
from the `InstanceTransforms` header, which only carried the **regular** previous
view-projection. The two matrices differ by the whole camera translation.

> **Takeaway:** a temporal artefact does not imply a temporal bug. `velocity = f(current) -
> f(previous)` cancels on a static camera **only if both endpoints use the same function**.
> When two code paths pick their matrices independently — one per-draw from a push constant, the
> other per-scene from a buffer — the mismatch is structural and survives any amount of
> camera stillness. Audit the PAIR, not the history. (Same session, same lesson twice: the
> jitter race above was the temporal variant of this, this one is the structural variant.)

**Fix.** Carry both forms in the header (`previousViewProjection` +
`previousViewProjectionInfinity`, at no size cost — the CURRENT view-projection that used to sit
there was read 0 times by the generated GLSL) and select with a generator flag that is part of
the program cache key. See `TODO.md` § "Infinity-view renderables wrote a garbage velocity".

---

### Skinned Meshes: Three Traps Paid For On The Same Dragon (Aug 2026)

All three were found on the `reflexion-debug` animated dragon (projet-alpha) and fixed in the
engine. Symptoms first, because that is how they will come back:

**1. Whole-body lighting flicker on animation frames (shadow mapping ON only).**
The shadow map holds the ANIMATED mesh depth (the shadow pass skins), but
`LightGenerator::generateVertexShaderShadowMapCode()` computed `PositionLightSpace` /
`DirectionWorldSpace` from the raw `Attribute::Position` — the BIND POSE. Sampling the animated
shadow map at bind-pose positions self-occludes the whole body on any pose far from bind, so the
model collapsed to the ambient term, deterministically per pose. **The shadow term must be
evaluated at `skinnedPosition`** whenever `vertexShader.isSkinningEnabled()` — all 8 generation
sites now go through the `localPosition` expression. Measured before/after with a 10-capture
burst on the dragon crop (dark-pixel fraction 38-81 % bimodal → 47-62 % continuous).

**2. Skinning SSBO was a single copy written by the logic thread.**
`updateSkinningMatrices()` used to `writeData()` immediately from `Visual::processLogics()`
while the GPU read the same buffer for in-flight frames — the per-frame SSBO rule
(`framesInFlight()` copies) applied here too. Now: the SSBO holds one ALIGNED section per frame
in flight (`minStorageBufferOffsetAlignment`), one descriptor set per section, the logic thread
only stages (mutex), and the render thread uploads ONCE per frame at first bind
(`flushSkinningMatrices()`, deduplicated on a monotonic frame cursor set by the Renderer). The
invariant that matters: every pass of a frame (shadow, ambient, lights, TBN) binds the SAME
section.

**3. Skinned visuals were culled on a volume that never followed the animation.**
Wings vanished at the screen edge. The frustum culling reads the entity's collision-model AABB
(or the bare position without one). Fix: `SkeletalAnimator` recomputes a model-space joints AABB
at every pose update; `Visual` expands it by a per-axis "flesh margin" measured ONCE on the
asset (bind mesh box vs first joints box) and publishes `ComponentBoundariesModified`; the
entity refreshes the collision model SHAPE only (`refreshCollisionBoundaries()` — not the full
`updateEntityProperties()`).

> [!CAUTION]
> **GENERAL RULE — a signal fired from inside an iteration must be DEFERRED (twice lived,
> same day, Aug 2026).** The engine walks its collections under non-recursive mutexes; any
> callback fired DURING such a walk that re-enters the collection (directly or through a
> handler) self-deadlocks the calling thread. Pattern: the signal sets an atomic/dirty flag,
> a well-defined point OUTSIDE the lock consumes it.
> 1. `notify()` from `Component::processLogics()` runs UNDER `m_componentsMutex`
>    (`AbstractEntity::processLogics()` holds it while calling each component) —
>    `ComponentBoundariesModified` sets a dirty flag consumed at the END of processLogics.
> 2. `Scene::signalOnDemandRenderTargets()` fires from
>    `getRenderableInstanceReadyForRendering()`, i.e. INSIDE the Renderer's render-to-textures
>    loop which holds the render target list mutex — walking the lists there froze the render
>    thread (black screen). Atomic flag consumed by `Scene::beginRenderFrame()`.

## Shader/GLSL Pitfalls

### Push Constants: the 128-Byte Minimum Guarantee (Jul 2026)

> [!CRITICAL]
> The Vulkan spec only guarantees **128 bytes** for `maxPushConstantsSize`. NVIDIA exposes
> 256, but part of the AMD/Intel fleet exposes exactly 128 — a pipeline layout declaring a
> larger range fails to create there (`vkCreatePipelineLayout` rejects it).
> VALIDATION (2026-07-25): `Vulkan::PipelineLayout::createOnHardware()` now rejects any
> range above the DEVICE limit (hard error) and warns on any range above the 128-byte
> minimum guarantee. A min-spec warning in the logs is a portability defect to fix.

**Known state:**
- RTGI's former trace push constants were exactly 128 B; adding the previous-frame matrix
  for temporal reprojection forced the migration to a **per-frame UBO** (`FrameUBOData`).
  Use the same pattern for any effect whose per-frame data outgrows 128 B:
  `IndirectPostProcessEffect::getInputLayout(samplerCount, uniformBufferCount)` +
  `createPerFrameUniformBuffers()` + `updateUniformBufferData()`.
- **FIXED (2026-07-25, motion vectors B1):** the scene-pass `useAdvancedMatrices` path
  pushed **132 bytes** (view 64 + model 64 + frameIndex 4). With the `InstanceTransforms`
  SSBO (`SetType::PerSceneTransforms`), the non-instanced scene paths now push
  VP + frameIndex (classic) or V + frameIndex (advanced) = **68 B**; the model matrix comes
  from the per-instance SSBO entry (`gl_InstanceIndex`, slot in `firstInstance`). The 132 B
  block only survives as a fallback for scenes whose instance transforms failed to
  initialize. See `src/Saphir/AGENTS.md` § "InstanceTransforms SSBO Path".
- **FIXED (2026-07-25, B1 milestone 5):** the INSTANCED advanced/billboard block declared
  V + VP + frameIndex = **132 bytes**. It now pushes V (+ frameIndex) only — the shader
  recomposes VP from the view UBO projection × V (`prepareModelViewProjectionMatrix()`
  instanced branches; shadow instanced billboard included — the ShadowCasting vertex
  shader now declares the view uniform block for every instanced program). All scene and
  shadow push blocks are ≤ 128 B; the PipelineLayout creation-time validation keeps it so.

**Rule:** before adding ANY field to a push constant block, sum the struct size; at
> 128 B, migrate to a UBO/SSBO instead. Never rely on the 256 B NVIDIA limit.

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

### Critical: Environment Cubemap Sampling Convention (Y negation)

> [!CRITICAL]
> **A world direction `D` samples any environment cubemap at `vec3(D.x, -D.y, D.z)` —
> never the raw direction.** The engine world is Y-down (UP = -Y) while cubemaps are
> stored Y-up.
>
> Reference sites (visually validated — celestial servoing reproduces star directions
> within 1°): the skybox (`Material/Helpers.cpp` `checkPrimaryTextureCoordinates`) and
> the material reflections (`PBRResource`/`StandardResource` bindless reflection GLSL).
>
> **Symptom of the raw-direction bug:** the sky is read upside-down — GI bounces tinted
> by the ground where the sky should be, ray-miss reflections showing the wrong hemisphere.
> Invisible on near-uniform skies, wrong on sunsets/HDR. RTGI, RTR and SSR all had it
> (fixed Jul 2026, IBL lot 1).
>
> **Any new cubemap sampling site — and the IBL irradiance/prefiltered generation — must
> apply the same negation.**

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
- **Do not pre-emptively rewrite** other concatenations — fix sites as the compiler actually flags them.
- **Sibling variant in emeraude-base:** the same GCC bug family surfaces as `-Wstringop-overflow`
  (write, not read) under `_FORTIFY_SOURCE=2` rather than PCH, and there the `+=`-on-a-named-local
  rewrite *is* enough (no `reserve` needed). Full comparison of both triggers and fixes:
  [`dependencies/emeraude-base/docs/caution-points.md`](../dependencies/emeraude-base/docs/caution-points.md).

### An unqualified `TracerTag` in a service `.cpp` resolves to `ServiceInterface::TracerTag`

`ServiceInterface` declares a **public** `static constexpr auto TracerTag{"ServiceInterface"}`. A
service `.cpp` that also declares a namespace-scope `constexpr auto TracerTag{"MyService"}` does
**not** override it: inside member functions, unqualified lookup finds the *inherited class member*
first, so every `TraceInfo{TracerTag}` in that file logs under **`ServiceInterface`** while the
file-local constant sits unused. The traces are mislabelled and the log becomes unsearchable.

- **Detected by:** clang's `-Wunused-const-variable` — **Debug only**, because `-Wall`/`-Wextra` are
  in the Debug warning set and *not* in the Release one. It reads as a trivial "unused variable"; it
  is a real logging bug.
- **Seen in (fixed Aug 2026):** `Graphics/ExternalInput.cpp` (two traces logged as
  `ServiceInterface`), plus a genuinely dead `TracerTag` in `Graphics/Renderable/Abstract.cpp`.
- **Rule:** in a `ServiceInterface` subclass, trace with the class's own **`ClassId`**. Never
  introduce a file-local `TracerTag` in a service translation unit.

> [!NOTE]
> **Corollary worth its own habit:** Release does not enable `-Wall`/`-Wextra`, Debug does — with
> `-Werror`. A Release-only workflow lets Debug rot until it stops compiling entirely, which is
> exactly what had happened (6 translation units, 4 distinct causes) when this was found.

### PCH masks missing STL includes → build PCH-OFF to catch them

The cascade-wide STL precompiled header (`emeraude-base/src/STLPrecompiledHeaders.hpp`) force-includes
the whole STL hot-set (`<ranges>`, `<string>`, `<ostream>`, `<sstream>`, …) into every translation
unit. With `EMERAUDE_ENABLE_PCH=ON` this silently hides any TU or header that uses `std::…` without
including the right header — it compiles only because the PCH already pulled that header in. Flip the
PCH off and the same code fails: `'std::views' has not been declared`, `'ostream' in namespace 'std'
does not name a type`, `'std::stringstream' has incomplete type`, etc.

- The PCH is an **optimisation, not an include provider**. Every TU and header must `#include` what
  it uses, independently of the PCH hot-set.
- Build `-DEMERAUDE_ENABLE_PCH=OFF` periodically to catch these — **both configs must stay green**. A
  PCH-ON-only habit lets the debt accumulate invisibly. A CI lane enforces it:
  [`.github/workflows/pch-off.yml`](../.github/workflows/pch-off.yml) (engine, Release, PCH OFF, ubuntu-latest).
- 2026-07-13 audit fixed 45 such files: 42 × missing `<ranges>` (`std::views`/`std::ranges`),
  `Physics/SurfacePhysicalProperties.hpp` × `<iosfwd>`+`<string>`, and `Help/ArgumentDoc.cpp` /
  `Help/ShortcutDoc.cpp` × `<sstream>`. The engine now compiles clean with and without the PCH.

### MSVC caps a single string literal at ~16 KB → split embedded shaders into adjacent literals

Embedded GLSL shaders are stored as raw string literals (`static constexpr auto … = R"GLSL( … )GLSL"`).
MSVC enforces a hard limit of **16380 bytes per string literal** (C++ implementations are only
required to support 65536, and MSVC picks the low end). GCC/Clang/MinGW impose no practical limit,
so an oversized shader compiles everywhere *except* MSVC — the breakage is Windows-only and looks
misleading.

- **Symptom (MSVC only):** `error C2026: string too big, trailing characters truncated` (FR:
  *« chaîne trop grande, caractères de fin tronqués »*). The reported line is where the byte counter
  overflows mid-literal, **not** where the code is actually wrong — do not go looking for a bug there.
- **Seen in:**
  - `Graphics/Effects/Framebuffer/RTR.cpp` — `RTRTraceFragmentShader` grew to ~18 KB (fixed Jul 2026).
  - `Graphics/Effects/Framebuffer/RTGI.cpp` — `RTGITraceFragmentShader` grew to ~16.6 KB, split at the
    `computeDirectLighting()` / `main()` boundary into ~10 KB + ~6.7 KB halves (fixed Jul 2026).
- **Wrong fixes:** there is no warning to silence — C2026 is a hard **error**, and the project never
  disables diagnostics anyway. Do **not** move the shader to an external file just to dodge this
  (embedded shaders are the engine convention).
- **Correct fix (zero runtime cost):** split the literal into **adjacent** string literals — the C++
  standard concatenates them at translation time into one identical string. Close and reopen the raw
  literal at a clean boundary (between two GLSL functions):
  ```cpp
  static constexpr auto Shader = R"GLSL(
  … first half (< 16 KB) …
  )GLSL" R"GLSL(
  … second half (< 16 KB) …
  )GLSL";
  ```
  Pick the boundary so the concatenation reproduces the original bytes exactly (mind the newline that
  precedes `)GLSL"` and follows the reopening `R"GLSL(`).
- **Preventive:** when an embedded shader approaches ~16 KB, split it *before* it crosses the line —
  the other RTR shaders (`RTRBlurFragmentShader`, `RTRCompositeFragmentShader`) are still small but any
  shader that keeps growing will re-trigger this on the next Windows build.

## Platform-Specific

### String Conversions on Windows

> **CRITICAL:** All UTF-8 ↔ Wide string conversions on Windows **MUST** use `Helpers.hpp` functions (`convertUTF8ToWide`, `convertWideToUTF8`). Do NOT create local wrappers with `MultiByteToWideChar`/`WideCharToMultiByte`.

**Files involved:**
- `PlatformSpecific/Helpers.hpp` — Declarations
- `PlatformSpecific/Helpers.windows.cpp` — Implementations

### Fixed: `std::filesystem::path` → `const char *` for a C library — encoding AND lifetime (Jul 2026)

Handing a path to a C library that wants a `const char *` has **two** independent traps on Windows.
Both were live in `Overlay/Manager.cpp` (`initImGUI()`), surfaced by the MSVC port.

**Trap 1 — `path::string()` is NOT UTF-8 on Windows.** `path::value_type` is `wchar_t` there, so
`path` has an implicit `operator std::wstring`, not `operator std::string`: assigning a `path` to a
`std::string` compiles on Linux/macOS and is a **hard error** on MSVC (`C2679`). The reflex fix,
adding `.string()`, compiles everywhere but converts through the **ANSI code page**, not UTF-8.
Most C libraries the engine feeds paths to expect UTF-8 — ImGui says so in its own source
(`imgui.cpp`, `ImFileOpen()`: *"We need a fopen() wrapper because MSVC/Windows fopen doesn't handle
UTF-8 filenames"*, then `MultiByteToWideChar(CP_UTF8, …)`). With `.string()`, any user whose profile
path leaves ASCII (`C:\Users\Sébastien\…`) silently gets a mangled filename and a lost `.ini`.

**Use `EmEn::Base::IO::toU8String(path)`** (`emeraude-base`, `IO/IO.hpp`) — `u8string()` on Windows,
`string()` on POSIX where native bytes are already UTF-8. Its mirror is `IO::u8path(std::string)`
for the reverse direction. `toGenericU8String()` does the same with forward slashes.

**Trap 2 — never `.string().c_str()` when the callee STORES the pointer.** `path::string()` returns a
temporary `std::string`; `.c_str()` points into it; the temporary dies at the end of the full
expression. `io.IniFilename = m_iniFilepath.string().c_str();` compiles clean, warning-free, and
leaves a dangling pointer: ImGui keeps that raw pointer and dereferences it at
`ImGui::DestroyContext()` → `SaveIniSettingsToDisk()` (`imgui.cpp:4487`), i.e. at engine shutdown.
Read-after-free, garbage path, or crash — and nothing in the build says a word.

**The rule:** if the C API stores the pointer, a **member must own the bytes** for at least as long
as the API holds them. `Overlay::Manager` keeps `std::string m_iniFilepath` / `m_logFilepath`
(explicitly *not* `std::filesystem::path`, which cannot expose UTF-8 `char` bytes on Windows), fills
them once via `IO::toU8String()`, and hands ImGui `m_iniFilepath.data()`. The Manager outlives the
ImGui context — `releaseImGUI()` is called from `Manager::onTerminate()`.

`.string().c_str()` remains **safe** where the callee consumes the pointer within the call and keeps
nothing: `FBXLoader.cpp:264` / `:1526` (`ufbx_load_file`) and the two `Dialog/*.mac.mm` sites. Those
have no lifetime bug — but the two ufbx calls still carry **Trap 1** on Windows (ufbx expects UTF-8
filenames), so they should move to `IO::toU8String()` too. Not done yet; no non-ASCII asset path has
hit it.

### Video Capture — macOS First-Frame Timing

AVFoundation's `startRunning` is asynchronous. The macOS `VideoCaptureDevice::open()` waits up to 3 seconds for the first frame via `std::condition_variable`. Without this, the first `captureFrame()` call would always fail.

**File:** `PlatformSpecific/VideoCaptureDevice.mac.mm:waitForFirstFrame:`

---

## Vulkan Validation

### Never assume a device capability — query it, and REQUEST it

> [!CRITICAL]
> Two distinct failures of the same reflex bit the engine in the same week, both silent, both
> macOS-only in their symptoms and both engine bugs:
>
> | What was assumed | Reality | Damage |
> |------------------|---------|--------|
> | Descriptor array sizes (4928 samplers) | MoltenVK caps update-after-bind samplers at 1024 | no scene loaded at all |
> | Enabling `VK_KHR_portability_subset` enables its features | they default to **disabled** unless the feature struct is chained | no direct lighting, no shadows, artifacts |
>
> **Querying is not enough — a feature must also be requested at device creation.** A capability the
> device advertises but the application never asks for is a capability the application does not
> have, and nothing tells you: the descriptor write is simply rejected and the descriptor stays
> unwritten. Full stories in [`docs/troubleshooting.md`](troubleshooting.md) → macOS / MoltenVK.
>
> Also remember the asymmetry between the two VUID families: the **non**-update-after-bind limits
> (`maxPerStageDescriptorSamplers`, 16 on MoltenVK) count only sets created *without*
> `UPDATE_AFTER_BIND_POOL_BIT`, while the update-after-bind limits count **every** set of the
> pipeline layout. Budget accordingly.

### The Synchronization Validation layer is NOT enabled — turn it on before hunting artifacts

> [!CRITICAL]
> The engine requests `VK_LAYER_KHRONOS_validation` but **never** requests its synchronization
> checks: there is no `VkValidationFeaturesEXT` / `validate_sync` anywhere in `Vulkan/Instance.cpp`.
> The startup banner tells you exactly what is armed:
>
> ```
> Current Validaiton Enabled:
>   - Core Checks
>   - Stateless Parameter
>   - Object lifetime
>   - Thread Safety
>   - Handle Wrapping        ← no "Synchronization"
> ```
>
> **Consequence:** a clean validation log proves nothing about synchronization. Race conditions,
> missing barriers and missing subpass dependencies are simply not looked for — on **any** platform.
> That is how the swap-chain hazard below survived unnoticed on Windows, Linux and macOS alike.
>
> **Rule: artifacts that look like memory corruption with a clean core-validation log are a
> synchronization hazard until proven otherwise.** Re-run with it enabled:
>
> ```bash
> VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT ./projet-alpha …
> ```
>
> Two things to know when you do. The layer **rejects** the offending submit
> (`VK_ERROR_VALIDATION_FAILED_EXT`), so features that ride on a hazardous submit appear *broken*
> while it is on — the screenshot path is one of them, and that is a measurement artifact, not a
> regression. And its message IDs are `SYNC-HAZARD-*`, not `VUID-*`: a grep for `VUID-` will report
> "zero errors" while hazards are flying past.

### Fixed: swap-chain render pass had no subpass dependency at all (Aug 2026)

> [!CRITICAL]
> **Symptom:** frame-to-frame graphical corruption on macOS, with a perfectly clean core-validation
> log. Under Synchronization Validation, 10 ×
> `SYNC-HAZARD-WRITE-AFTER-READ: vkCmdBeginRenderPass writes to resource, which was previously
> accessed by vkAcquireNextImageKHR`.
>
> **Root cause:** `SwapChain::createRenderPass()` went straight from `addSubPass()` to
> `createOnHardware()` — **no `addSubPassDependency()` call**. Its colour attachment declares
> `initialLayout = UNDEFINED`, so beginning the pass transitions the acquired image, while the
> submit waits on the acquisition semaphore at `COLOR_ATTACHMENT_OUTPUT`. Without an explicit
> external dependency the implicit one uses `TOP_OF_PIPE`, which creates **no execution dependency**
> with that wait: the layout transition was free to run before the image was available.
>
> **Why only macOS showed it:** the bug is a spec gap on every platform. With
> `initialLayout = UNDEFINED` the previous contents may be discarded, so desktop drivers usually
> emit no real transition, or schedule it after the wait anyway — nothing to race. MoltenVK has to
> *emulate* render passes over Metal command encoders, so the transition becomes real work that can
> genuinely precede the wait. **Do not read "it works on Windows" as "it is correct".**
>
> **Fix:** an external dependency with `srcStageMask` covering the wait stages. An *execution*
> dependency is sufficient (`srcAccessMask = 0`): what must be guaranteed is the order between the
> semaphore wait and the transition, not a cache flush.
>
> **Verified:** 10 hazards → 0, no more rejected submits. **File:** `Vulkan/SwapChain.cpp`.

### Open: SwapChain::capture() writes to an image the engine no longer owns

> [!WARNING]
> `SYNC-HAZARD-WRITE-AFTER-PRESENT` — the capture path transitions a **presented** swap-chain image
> out of `PRESENT_SRC` to download it. A presented image belongs to the presentation engine until it
> is **re-acquired**; writing to it (a layout transition is a write) is illegal.
>
> **This is an ownership problem, not a timing one.** A host-side drain (`vkDeviceWaitIdle`) does
> **not** fix it — that was tried and reverted. Capturing `SceneRenderTarget` instead is not
> equivalent either: it holds the HDR buffer *before* tone mapping, so it cannot show what reaches
> the screen.
>
> **Correct fix (not implemented):** capture inside the frame, right after the post-process pass and
> **before** the present, while the image is still acquired — the console command would only arm a
> request and read the result on the following frame.
>
> **Impact today:** none outside Synchronization Validation, where the layer rejects the submit and
> the capture returns nothing. **File:** `Vulkan/SwapChain.cpp::capture()`.

### Fixed: Present semaphore was indexed by frame in flight, not by swap-chain image (Aug 2026)

`VUID-vkQueueSubmit-pSignalSemaphores-00067`, fired on the **first frames** and aborting the
submission with `VK_ERROR_VALIDATION_FAILED_EXT`. Surfaced the day the validation layers were
enabled **on Windows**; the same code had been running clean on Linux for a long time.

```
pSubmits[0].pSignalSemaphores[0] is being signaled by VkQueue ...
Most recently acquired image indices: 0, [1], 2, 0, 0.
Swapchain image 1 was presented but was not re-acquired, so the semaphore may still be in use.
```

That index list **is** the diagnosis: `0, 1, 2, 0, 0` is not a cycle. `vkAcquireNextImageKHR()`
hands back image indices in whatever order the presentation engine chooses — under MAILBOX
(selected on Windows for triple buffering) it repeats and skips freely, whereas under FIFO
(Linux/Mesa) it happens to cycle, which is exactly why the platforms disagreed.

The semaphore signaled by the frame submission and waited on by `vkQueuePresentKHR()` lived in
`RendererFrameScope`, i.e. **one per frame in flight**, selected with `m_currentFrameIndex`,
which advances `+1 % framesInFlight()`. `createRenderingSystem()` sizes the frame scopes from
the swap-chain image count, so the *counts* matched and hid the defect — but the *mapping* was
wrong. The frame slot eventually came back around and re-signaled the very semaphore that the
still-pending present of another image was waiting on.

> [!CAUTION]
> **Why a fence does not save you here.** The frame's in-flight fence proves the *submission*
> completed. It says nothing about the *present*: no fence observes the completion of a
> `vkQueuePresentKHR()` (that is precisely what `VK_KHR_swapchain_maintenance1` exists to add).
> The only proof that a present released its semaphore is the **re-acquisition of the image it
> presented**. Hence the asymmetry inside one frame:
> - **image-available** semaphore → **per frame in flight** (the image index does not exist yet
>   at acquisition time; the fence proves reuse is safe),
> - **present** semaphore → **per swap-chain image** (nothing but re-acquisition proves it).

**Fix:** `Renderer::m_presentSemaphores`, one semaphore per swap-chain image, indexed by the
value `acquireNextImage()` returned. `RendererFrameScope::m_renderFinishedSemaphore` is gone —
the wrongly-indexed primitive was removed rather than left available to be misused again. The
array is owned by the **rendering system** and not by `SwapChain::Frame`, so that it survives
swap-chain recreation: `vkDeviceWaitIdle()` does not retire pending present operations, so
destroying those semaphores on every resize would trade this VUID for a
destruction-while-in-use on the resize path.

**Second defect, same root, fixed in the same pass:** every early `return` placed *after* a
successful acquisition leaked signaled semaphores (and leaked the acquired image, which nothing
but a swap-chain recreation can give back). `Renderer::discardAcquiredImage()` now drains them
with an empty synchronization batch — `Queue::submit(const SynchInfo &)`, no command buffer, a
new engine contract — deduplicated because the caller may already have appended the acquisition
semaphore, signaling the fence back only when it had already been reset. It then declares the
swap-chain degraded. `getCommandBuffer()` can return a null pointer and was being dereferenced
unchecked at that same spot; it is now tested.

> [!TIP]
> **Index discipline.** In `renderFrame()` the acquired value is named `imageIndex`, never
> `frameIndex`. Anything addressed by it (framebuffer, colour image, present semaphore) uses it;
> anything belonging to the frame slot (fence, command pool, per-frame SSBOs and descriptors)
> uses `m_currentFrameIndex`. Equal counts do not make them the same number.

**Files:** `Graphics/Renderer.{hpp,cpp}` (`m_presentSemaphores`, `createRenderingSystem()`,
`discardAcquiredImage()`, `renderFrame()`), `Graphics/RendererFrameScope.{hpp,cpp}`,
`Vulkan/Queue.{hpp,cpp}` (sync-only `submit()`), `Vulkan/SwapChain.{hpp,cpp}` (`present()`
`@warning`). Contract: `src/Graphics/AGENTS.md` § 16 Rule 5.

---

### Fixed: ToneMapping auto-exposure readback — AdaptLum images lacked TRANSFER_SRC usage (Jul 2026)

Enabling HDR on a camera (`Component::Camera::enableHDR(true)`) produced a storm of validation
errors from the auto-exposure metering path — five distinct VUIDs, **one** root cause.

`ToneMapping` reads its 1x1 adaptation target back to a host-visible buffer every frame: it
barriers the image `SHADER_READ_ONLY_OPTIMAL` -> `TRANSFER_SRC_OPTIMAL`, calls
`vkCmdCopyImageToBuffer`, then barriers it back. All of that was correct. But the images came
from `IntermediateRenderTarget::create()`, which hardcoded
`COLOR_ATTACHMENT_BIT | SAMPLED_BIT` — **no `TRANSFER_SRC_BIT`**.

An image can only be transitioned to a layout its usage flags support, so the first barrier was
rejected, the tracked layout stayed `SHADER_READ_ONLY_OPTIMAL`, and everything downstream
cascaded off a stale layout:

| VUID | What it was really reporting |
|---|---|
| `VkImageMemoryBarrier-oldLayout-01212` | the ROOT CAUSE: `newLayout` incompatible with the usage flags |
| `vkCmdCopyImageToBuffer-srcImage-00186` | `srcImage` lacks `TRANSFER_SRC` usage |
| `vkCmdCopyImageToBuffer-srcImageLayout-00189` | declared `TRANSFER_SRC`, actual `SHADER_READ_ONLY` |
| `VkImageMemoryBarrier-oldLayout-01197` | the restore barrier's `oldLayout` matched nothing |

**Fix:** `IntermediateRenderTarget::create()` gained a defaulted `extraUsageFlags` parameter
(0 by default, so the other ~65 call sites are untouched), and `ToneMapping` passes
`VK_IMAGE_USAGE_TRANSFER_SRC_BIT` for its two adaptation targets.

> [!IMPORTANT]
> **Behavioural consequence:** the metered-luminance readback was invalid before this, so the
> auto-exposure loop was fed undefined data. It is now actually driven by the scene luminance —
> expect the exposure of an HDR camera to differ from (and be correct, unlike) the old one.

**Files:** `Graphics/IntermediateRenderTarget.{hpp,cpp}`,
`Graphics/Effects/Framebuffer/ToneMapping.cpp:createEffect()`

---

### Fixed: transient compute descriptor pools missing FREE_DESCRIPTOR_SET_BIT (Jul 2026)

`VUID-vkFreeDescriptorSets-descriptorPool-00312`, fired by the IBL bake on every sky change.
`Vulkan::DescriptorSet::destroyFromHardware()` **unconditionally** calls
`DescriptorPool::freeDescriptorSet()`, so *every* pool whose sets are wrapped in a
`DescriptorSet` needs `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT` — including the
short-lived ones that were meant to be thrown away whole.

Three pools were missing it: two in `IBLBaker` (the BRDF LUT bake and the per-environment
prefilter/irradiance bake) and one in `XRayAnalyzer`.

The failure also *looked* like something else: `DescriptorPool::freeDescriptorSet()` logged
`"Unable to allocate a descriptor set"` on the free path (copy-paste), so the symptom read as an
allocation failure. The message now says `free` and names the missing flag.

**Files:** `Graphics/Compute/IBLBaker.cpp`, `Graphics/Compute/XRayAnalyzer.cpp`,
`Vulkan/DescriptorPool.cpp:freeDescriptorSet()`. See `src/Vulkan/AGENTS.md`.

---

### Fixed: velocity G-buffer image and DoF focus targets lacked TRANSFER_SRC (Aug 2026)

Two more instances of the exact failure documented in the ToneMapping entry above (read that one
first — same root cause, same VUID cascade off a stale tracked layout):

- **`SceneRenderTarget` velocity image** (`SceneRenderTarget.cpp:createImages()`): it was the
  ONLY G-buffer attachment created without `TRANSFER_SRC_BIT` (color, normals, material
  properties, albedo and depth all had it), yet `PostProcessor::recordBlit()` copies it to the
  grab pass every frame like the others. An oversight from the motion-vectors work: the copy was
  added, the creation flags were not.
- **`DepthOfField` focus targets** (`DepthOfField.cpp:createEffect()`): the 1x1 rack-focus
  ping-pong targets are read back per frame with `vkCmdCopyImageToBuffer` (metered focus
  distance), but the `IntermediateRenderTarget::create()` call did not pass
  `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` — the exact pitfall the `extraUsageFlags` parameter was
  added for, and which `ToneMapping` already did correctly for its twin AdaptLum readback.

> [!WARNING]
> On the NVIDIA driver both paths "worked" silently — the errors only surface with validation
> layers on. When adding ANY readback or image-to-image copy, grep the creation site for
> `TRANSFER_SRC` before assuming the copy is legal. Checklist: readback → `TRANSFER_SRC_BIT` at
> creation, out-of-render-pass barriers around the copy, restore barrier to the layout the next
> consumer expects.

**Files:** `Graphics/SceneRenderTarget.cpp`, `Graphics/Effects/Framebuffer/DepthOfField.cpp`

---

### Fixed: RT post-process effects drew before the TLAS existed (Aug 2026)

During the first frames of a scene (the TLAS is built asynchronously) — or in a scene with no
RT-eligible geometry at all — the four RT effects (RTR, RTGI, RTAO, ContactShadows) still
recorded their trace passes:

- RTR/RTGI/RTAO bind set 0 from `Renderer::rtDescriptorSet()` **only if non-null**, then drew
  anyway → `VUID-vkCmdDraw-None-08600` (set #0 never bound), undefined behavior in the shader's
  ray queries.
- ContactShadows binds its own per-frame set but only WRITES the TLAS binding when the TLAS
  exists → `VUID-vkCmdDraw-None-08114` (descriptor never updated).

**Fix — contract improvement, single site:** `Renderer::isRayTracingReady()` (TLAS non-null,
created, RT descriptor sets allocated) joined the `requiresRayTracing()` skip gate in
`PostProcessor::executeIndirectPostProcessEffects()`. When not ready, RT effects are skipped for
the frame exactly like the existing device/settings gates: the chain forwards the previous
output, and the effects contribute from the first frame the TLAS is consumable.

> [!NOTE]
> The per-effect `if (rtDescSet != nullptr)` binds remain as defense in depth, but the chain
> gate is the contract: an RT effect's `execute()` can assume a consumable TLAS.

**Files:** `Graphics/Renderer.{hpp,cpp}:isRayTracingReady()`,
`Graphics/PostProcessor.cpp:executeIndirectPostProcessEffects()`

---

### Fixed: SkinnedGeometryProcessor leaked its compute pipeline at device destroy (Aug 2026)

At shutdown, validation reported `VUID-vkDestroyDevice-device-05137` for a
DescriptorSetLayout + PipelineLayout + Pipeline trio, the engine reported 4 live references on
the device smart pointer, and — the giveaway — the wrappers logged
`"No device to destroy ..."` **after** `*** Core level terminated ***`.

`Renderer::onTerminate()` released the acceleration-structure builder, RT descriptor sets, MDI,
swap chain... but never `m_skinnedGeometryProcessor` (which owns the RT-skinning compute
pipeline + layouts). The unique_ptr survived until `~Renderer` — long after `m_device.reset()` —
so its Vulkan objects had no device to be destroyed against.

> [!IMPORTANT]
> **Rule:** every Renderer member that owns Vulkan objects MUST be explicitly reset in
> `Renderer::onTerminate()` (after the `waitIdle` + deferred-destructor flush, before
> `m_device.reset()`). Leaving it to the destructor is a guaranteed device-child leak. Symptom
> signature to recognize it instantly: `05137` at `vkDestroyDevice` + wrapper errors printed
> AFTER Core termination.

**Files:** `Graphics/Renderer.cpp:onTerminate()`

### Fixed: a descriptor set skipped at binding shifted every following set (Aug 2026)

`VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358` then
`VUID-vkCmdDrawIndexed-None-08600`, on the **first frames only**, in the `reflexion-debug`
demo options 3 and 4 (the two probe-camera reflection modes — the modes that render the scene
into a cubemap **at scene build time**):

```
pDescriptorSets[0] being bound is not compatible with overlapping descriptorSetLayout at
index 1 ... layout has 1 total descriptors, but the bound one has 3 total descriptors.
... at index 2 ... has 3 total descriptors, but the bound one has 4928 total descriptors.
The VkPipeline statically uses descriptor set 3, but all sets 0 to 3 are not compatible ...
```

The count ladder **is** the diagnosis: 1 → 3 → 4928 is the skinning SSBO, the material and
the bindless array, each landing one slot too low. The recording code walked the sets with a
running `setOffset++` counter, so skipping ONE set shifted every set after it.

The skipped set was `PerModel` (skinning) on the animated glTF dragon. Its two conditions had
diverged:

| | Condition | Owner | When |
|---|---|---|---|
| Pipeline layout (sealed) | `SkeletalDataTrait::hasSkeletalData()` | the **renderable** | as soon as the resource is loaded |
| Binding | `hasSkinningResources()` | the **instance** | first `processLogics()` (LOGIC thread) |

A probe cubemap renders before the first logic tick, so the layout declared the set the
instance did not own yet. Two more traps compounded it: the program cache lives on the
**renderable** (shared by every instance), so a second instance of the same skeletal mesh was
declared ready on the first one's cached program and never created its own descriptor sets;
and `getReadyForShadowCasting()` is a separate entry point from `getReadyForRender()`.

**Fix** (`Graphics/RenderableInstance/Abstract.{hpp,cpp}`, `Scenes/Component/Visual.cpp`):

1. `prepareSkinningResources()` at the top of `getReadyForRender()` **and**
   `getReadyForShadowCasting()` — render thread, same instant as the layout sealing. Removed
   from `Scenes::Component::Visual` (which keeps the animator and the pose upload).
2. `isReadyToRender()` / `isReadyToCastShadows()` answer **false** while
   `isMissingSkinningResources()` — a renderable-level cached program never makes an instance
   ready on its own.
3. Every set is now bound at the index the sealed layout **declares**
   (`program->setIndexes().set(SetType::X)`), never at a running counter, in all three
   recording paths. A declared set with no resource drops the draw and reports once
   (`traceMissingDescriptorSet()`) instead of silently shifting the others.

> [!IMPORTANT]
> **Rule:** every descriptor set has TWO conditions — one at generation, one at binding — and
> they must be equivalent BY CONSTRUCTION. When one is a property of the renderable and the
> other a property of the instance, they will diverge. The full table lives in
> `@src/Saphir/AGENTS.md` § "Descriptor set binding contract".

**Reproduce:** `./projet-alpha --load-demo reflexion-debug --demo-options 0,4,0` with the
validation layers on; the errors fire in the first second, then never again.

**Files:** `Graphics/RenderableInstance/Abstract.cpp` (`prepareSkinningResources`,
`isMissingSkinningResources`, `traceMissingDescriptorSet`, the 3 binding paths),
`Scenes/Component/Visual.cpp:processLogics()`

### Fixed: a queue family release is a ONE-SHOT token (Aug 2026)

`VUID-vkQueueSubmit-pSubmits-02207` ×20 in `animation-debug`, each one killing its submission
with `VK_ERROR_VALIDATION_FAILED_EXT`:

```
contains a VkBufferMemoryBarrier that acquires ownership of VkBuffer 0xaa… for destination
queue family 0, but no matching release operation was queued for execution from source
queue family 1.
  → Queue submit failed ! → BLAS build command submission failed !
  → Unable to build the refit-able BLAS for skinned geometry !
```

Instrumenting `buildBLAS()` gave the answer in one line: the SAME vertex/index buffer pair
built twice, the first build succeeding, the second failing forever (retried every frame).
Two instances of the same skeletal mesh (the two foxes) each build their own refit-able BLAS
from the same source vertex buffer.

`BufferTransferOperation` recorded the ownership RELEASE at upload and left the ACQUIRE to
"whatever command first reads this buffer on the graphics queue" — while
`AccelerationStructureBuilder::buildBLAS()` recorded that acquire **unconditionally, on every
call**. The first consumer matched the release; every consumer after it acquired into the void.

**Fix** (`Vulkan/BufferTransferOperation.{hpp,cpp}`, `Vulkan/TransferManager.cpp`,
`Vulkan/AccelerationStructureBuilder.{hpp,cpp}`): both halves now live in the SAME operation,
in the two-step shape `ImageTransferOperation` already used — transfer queue copies and
releases (signals a semaphore), graphics queue acquires (waits that semaphore, signals the
operation fence). `buildBLAS()` records a plain memory barrier and no acquire at all.

> [!IMPORTANT]
> **Rule:** a queue family ownership transfer is a PAIR, and the pair belongs to one operation.
> Never leave the acquire to an unnamed "first reader" — nothing enforces "first", and the
> second reader is a validation error that costs you the whole submission. The invariant to
> hold on to: *once uploaded, a buffer belongs to the graphics family.*

**Bonus fact the fix exposed:** the raster path reads those same buffers on the graphics queue
and never acquired anything — it was relying on the ownership being transferred by someone
else. Pairing at upload time closes that hole too.

**Measured:** `animation-debug` goes from 20 validation errors to 0, both foxes get their
skinned BLAS (they now log twice instead of once), and terrain load time is unchanged
(6.89/7.10/6.89 s with the fix vs 7.09/7.09/6.88 s without — the extra graphics submission per
upload does not show).

**Files:** `Vulkan/BufferTransferOperation.cpp:transfer()`,
`Vulkan/AccelerationStructureBuilder.cpp:buildBLAS()`

---

## Related Documentation

- `@AGENTS.md` - Engine root context
- `@src/PlatformSpecific/AGENTS.md` - Platform-specific system details
- `@src/Graphics/AGENTS.md` - Graphics system
- `@src/Saphir/AGENTS.md` - Shader generation system
