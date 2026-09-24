# Caution Points

Critical warnings, known pitfalls, and hard-won lessons for Emeraude Engine development.

## Table of Contents

- [Graphics/Material System](#graphicsmaterial-system)
- [Ray Tracing / Acceleration Structures](#ray-tracing--acceleration-structures)
- [Resources / Loaders](#resources--loaders)
- [Animation](#animation)
- [Scene Rendering](#scene-rendering)
- [Shader/GLSL Pitfalls](#shaderglsl-pitfalls)
- [Build / Compiler](#build--compiler)
- [Platform-Specific](#platform-specific)
- [Vulkan Validation](#vulkan-validation)

---

## Graphics/Material System

### The UV transform table is INDEXED — the component picks a slot, it does not own one

> **What it replaced (2026-09-14):** six fixed `vec4` pairs in the material UBO, one per component
> type — albedo, roughness, metalness, normal, AO, emissive — and a `switch` in
> `transformedTexCoords()` whose `default:` returned the PLAIN coordinates. Fourteen component types
> fell through it, so `KHR_texture_transform` on a sheen, clear coat, specular, iridescence,
> transmission or volume map was parsed, sometimes logged, and dropped.
>
> **Why indexed rather than wider:** measured over **1347 materials** (Khronos corpus + project),
> only **38** declare any UV transform at all, **31 of those declare exactly ONE** shared by every
> map they carry, and **none declares more than three**. Widening the fixed table to all twenty
> component types would have cost 160 floats for the benefit of 2.8 % of materials; a four-entry
> table plus one index per component type costs 60 and serves every one of them.
>
> **The layout:** `uvwTransform[4]` (scale.xy, offset.zw), `uvwRotation[4]` (cos, sin), and
> `uvwIndex[7]` — one float per `ComponentType`, packed four to a `vec4`. **Slot 0 is always the
> identity**, so a component that declares nothing indexes it and comes out unchanged.
>
> ⚠️⚠️ **The index is a VALUE in the UBO, never a GLSL literal.** The slot POSITION is a function of
> the ComponentType and is identical for every material, so that part is a literal; the entry it
> holds is per-material and must not be. The program cache keys on the descriptor layout and the
> material flag bits, never on values — two materials with the same layout share a program, and a
> baked index would serve one material's transform to the other. Same rule as every other per-material
> value.
>
> ⚠️ **The trig stays on the CPU**, resolved once per material in `syncComponentUVWTransforms()`: the
> angle is a material constant, and a `sin`/`cos` in the shader is paid per pixel per frame forever.
>
> ⚠️ A material declaring more than three distinct transforms keeps the identity on the surplus maps
> and **says so once** in the log. A wrong transform would be worse than an absent one, and silence
> worse than both.
>
> **The control for any change here is `TextureTransformTest`** — the only asset in the bench that
> declares a rotation, and the one that exercises offset, scale and clamp together. The rewrite left
> it **bit-exact** (0.00 % of pixels, delta 0), as did `MetalRoughSpheres` and `WaterBottle`. If it
> moves, the table is wrong.

### A `PixelFactory::Color` CLAMPS to [0,1] — so it cannot carry a factor that may exceed 1

> **Symptom:** a material parameter that the format explicitly allows above 1.0 renders as though
> it were exactly 1.0. No warning, no log line, nothing in the shader to blame — the value is gone
> before the material ever sees it.
>
> **Root cause:** `Color< float >`'s constructor runs every component through `Math::clampToUnit`
> (`emeraude-base/src/PixelFactory/Color.hpp:78`). That is correct for a COLOUR and wrong for a
> MULTIPLIER, and the two are easy to confuse because the glTF field is called
> `specularColorFactor`.
>
> **Measured (2026-09-14):** `KHR_materials_specular` allows `specularColorFactor` above 1 so a
> material's IOR cannot cap its specular response, and `SpecularTest`'s seventh row exists to check
> it — its last sphere must read as a mirror ball. Through a `Color` the row was **flat**:
> 9.33 / 9.35 / 9.05 / 8.87 out of 255. Carried as a `Math::Vector< 3, float >` it becomes
> **10.22 / 26.42 / 47.13 / 70.95**, a monotone ramp, while the six other rows stay identical to
> the hundredth — the control.
>
> **The rule:** a value the format calls a *factor*, *weight*, *gain* or *scale* goes in a
> `Math::Vector`, never a `PixelFactory::Color`, however colour-shaped its name. Do NOT widen
> `Color` to fix a case like this: it is a foundation type whose [0,1] contract is relied on
> everywhere. Add the overload that takes the vector, as `setSpecularColor()` does.
>
> ⚠️ Energy conservation is not what is at stake, and checking it is not enough to clear this:
> the shader already clamps the RESULT (`F0 = min(dielectricF0 * specularColor * specularFactor, 1)`).
> It was the TRANSPORT of the authored value that was lossy.

### A shared-UBO bank that fills up takes a material's MESH out of the scene — FIXED 2026-09-14

> **Symptom:** an asset with many distinct materials renders with **geometry missing**. No VUID, no
> black frame, no missing-resource warning; the object simply is not there, and a grid of test cells
> just looks sparser than the reference. The log carries `Unable to add the PBR material to the
> shared uniform buffer !`, one line per lost material and with no total, which reads like a
> cosmetic complaint.
>
> **Root cause:** `SharedUniformBuffer::computeBlockAlignment()` sizes the bank from a **hard-coded
> 65536** (`src/Graphics/SharedUniformBuffer.cpp:86` — the device's `maxUniformBufferRange` sits
> commented out on the same line), `addElement()` returns false once every seat is taken, and
> `Material::Interface::getSharedUniformBuffer()` states the consequence in its own comment: the
> material **fails to load entirely, removing its sub-meshes from the scene**.
>
> **The arithmetic, and why it MOVES:** seats = 65536 / (material block aligned to
> `minUniformBufferOffsetAlignment`). The block is **416 bytes** today and alignment is 64 on this
> NVIDIA device ⇒ 448 ⇒ **146 seats**. The block was 320 bytes before `KHR_texture_transform`'s
> rotation added a second block of 6 vec4 in Aug 2026, i.e. **204 seats**. Every future material
> field narrows the ceiling again, and nothing announces it.
>
> **Measured (2026-09-14):** both Khronos iridescence grids declare 344 untextured materials — all
> in the same `MaterialStandardResourceSimple` bank — and **197 of them (indices 146…342) fail**.
> Isolated spheres read 203…216 with a seat and 19.8…31.1 without, against a background of
> 16.7…32.7: **57 % of each grid is absent from the scene.**
>
> ⚠️⚠️ **The trap when you go to verify it:** a sphere whose material failed shows whatever is
> BEHIND it, and in a 7×7×7 grid that is almost always another sphere. A naive per-cell probe
> therefore reports the missing half as *saturated and correlated with its declared parameters* —
> a plausible, publishable, completely wrong reading. Restrict the measurement to cells with
> **nothing in front of or behind them** before concluding anything.
>
> **Do not** diagnose this as an unwired material feature: it looks exactly like one, the same way
> the resource-key collapse did (see *Fixed: an asset NAME was used as a resource identity*).
> Count the `Unable to add` lines and compare with the asset's material count first.
>
> **Fixed 2026-09-14**, two halves:
> 1. `SharedUniformBuffer::bankSize()` is now the single place a bank's size is decided, and it is
>    `min(maxUniformBufferRange, 65536)`. ⚠️ **The device limit is a CEILING, never the size to
>    allocate** — desktop AMD reports it in the gigabytes, and *allocating that* is the defect the
>    old `TODO: this doesn't work with desktop AMD graphics card` note was about, which is why the
>    limit had been commented out in favour of a bare constant. Honour a device that offers less;
>    cap one that offers absurdly more.
> 2. `addElement()` allocates **another bank** instead of returning false, which is what this
>    class's own sizing comment always claimed it did. Capacity comes from more banks, never from a
>    bigger one.
>
> ⚠️ **Why both bank vectors are `reserve()`d to `MaxBankCount` at construction, and why that is
> load-bearing:** a material writes its own block through `writeElementData()` from the logic
> thread while another material is still loading and may be adding a bank. Reserved, a `push_back`
> can never reallocate, so a reader indexing a bank that already existed when it got its seat is
> safe. Remove the reservation and the growth becomes a use-after-free that only shows under
> concurrent loading.
>
> **Measured after the fix**, same scene: zero `Unable to add` lines, zero VUID, and the isolated
> seat-less spheres go from 19.8…31.1 (the backdrop) to **30.1…217.2, mean 187.9** — the grid is
> complete, and its pastel iridescence is visible for the first time.

### A colour AND a texture cannot both be the same material component — the second `emplace()` is rejected SILENTLY

> **Symptom:** a material is given both a colour and a texture for the same component (albedo being
> the usual case); only one of them shows up, and **nothing is logged**.
>
> **Root cause:** `Material::Interface`'s component map is filled with `m_components.emplace(...)`.
> `emplace` on an existing key is a no-op that returns `false` — **first call wins**, second call
> discarded without a trace. Two setters that both create the same `ComponentType` therefore
> silently conflict.
>
> **The correct authoring pattern:** the **texture** is the component, the **colour** is the TINT —
> `setAlbedoComponent(texture)` + `setAlbedoColor(colour)`. That is what the shader multiplies
> anyway, so nothing is lost.
>
> **Two ordering rules of the same family** (also silent when violated):
> - Unlit content needs its emissive **in this order**: `setAutoIlluminationComponent(1.0F)` FIRST
>   (it CREATES the component), then `setEmissiveStrength(<nits>)`, then `enableUnlit()`.
>   `setAutoIlluminationAmount()` alone is a post-creation setter and does **nothing** if the
>   component does not exist yet.
> - Dropping an `enableVertexColor()` call loses the vertex colours with **no log at all**
>   (`VertexBufferFormatManager` emits `declareJump(VertexColor)` and discards the attribute).
>
> **Measured during:** the app-side migration of the material merge (Aug 2026), where each of these
> cost a debugging round with an empty log.

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
> (`MaterialStandardResource2Textures`), so distinct materials share one identifier **by design** — and
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
> [Info] There is no shared uniform buffer named 'MaterialStandardResource2Textures' !   <- TWICE in a row
> [Error] A shared uniform buffer named 'MaterialStandardResource2Textures' already exists !
> [Error][MaterialStandardResource] Unable to get the shared uniform buffer !
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

Since the material merge, `StandardResource` **IS** the PBR (metallic-roughness) material, and its
float array is **80 floats / 320 bytes** (std140 aligned). The authoritative layout is the comment
sitting above the `*Offset` constants in `Graphics/Material/StandardResource.hpp`; the offsets most
often traced:

| Offset | Property | Range | Notes |
|--------|----------|-------|-------|
| 0-3 | albedoColor | vec4 | RGBA, also the TINT factor over the albedo texture |
| 4 | roughness | float | 0-1 |
| 5 | metalness | float | 0-1 |
| 6 | normalScale | float | 0-1 |
| 8 | ior | float | 1.0-3.0 (clamped by `setIOR()`) |
| 9 | iblIntensity | float | 0-1 |
| 10 | autoIlluminationAmount | float | 0-1 (emissive MASK, not a brightness) |
| 12-15 | autoIlluminationColor | vec4 | RGBA |
| 48 | emissiveStrength | float | nits — the actual emissive brightness |
| 50 | opacity | float | 0-1 |
| 51 | alphaThreshold | float | 0-1, glTF `alphaCutoff` |
| **52** | **reflectionAmount** | float | 0-1, artistic mix (texture/probe modes) |
| **53** | **refractionAmount** | float | 0-1, artistic mix (texture mode) |
| 56-79 | per-component UVW transforms | 6 × vec4 | `KHR_texture_transform`, neutral `(1,1,0,0)` |

> [!CAUTION]
> **There is no `ambientColor` / `diffuseColor` / `specularColor` / `shininess` float any more** —
> that layout died with the legacy Blinn-Phong material. `BasicResource` keeps its own, much smaller
> block (`DiffuseColorOffset`, `SpecularColorOffset`, `ShininessOffset`); never read one material's
> offsets against the other.

**Debugging tip:** If reflection/refraction amounts seem wrong, trace:
1. C++ side: Are values written to correct offsets?
2. Shader side: Is the UBO struct layout matching?
3. Descriptor: Is the correct byte offset used?

### An alpha-tested material IS opaque — never teach `isOpaque()` about `AlphaTestEnabled` (Aug 2026)

> [!CAUTION]
> **`MaterialFlagBits::AlphaTestEnabled` (`1U << 16`, set by `BasicResource::enableAlphaTest()`) is a
> binary CUTOUT: the fragment shader discards below a cutoff and the material STAYS OPAQUE.** The
> intuitive "fix" — making `Material::Interface::isOpaque()` return `false` for it — breaks the feature
> instead of completing it, in two places at once:
>
> 1. The Scene dispatches the layer into the **distance-sorted translucent list**: a per-frame sort and
>    the loss of state-sorted batching, for a mask that has nothing to sort.
> 2. `Vulkan::GraphicsPipeline::configureColorBlendState()` keys its default branch on
>    `material.isOpaque()`. A `false` there flips `blendEnable` to `VK_TRUE` and installs the factors of
>    `blendingMode()`, so the already-binary alpha gets **colour-blended** on top of the discard.
>
> The flag adds a discard and changes **NOTHING ELSE** about how the material is classified. That is the
> whole point: `enableBlending()` is what a genuine gradient needs; a coverage mask must not pay for it.
> Gating the discard on the blending mode was the original defect — it left no way to obtain a cutout
> without also leaving the opaque list.
>
> **The cutoff is FIXED at 0.5 and deliberately not configurable.** Two structural blockers: the shader
> **program cache keys on the DESCRIPTOR LAYOUT hash**, not on flags or values, so a per-material cutoff
> literal baked into the generated GLSL could serve one material's program to another with the same
> layout; and all twelve floats of `BasicResource`'s material-properties buffer are already claimed, so a
> uniform-borne cutoff would require growing the block. The three paths now agree at 0.5 — colour
> discard, shadow discard, and `GPURTMaterialData::alphaCutoff`. **Fix the cache key before making it
> configurable.**
>
> **Related:** `isAlphaTest()` returns `true` for the flag (RT hit-time alpha test) and
> `BasicResource::requiresAlphaTestedShadows()` does too — a cutout must cast a cutout shadow, not a
> solid rectangle.
>
> **Full contract:** [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) § 5, "Alpha Test — the Binary
> Cutout Contract".
>
> **Files involved:** `Graphics/Material/Interface.hpp` (flag, `isOpaque()`, `isAlphaTest()`),
> `Graphics/Material/BasicResource.{hpp,cpp}` (`enableAlphaTest()`, discard generation,
> `requiresAlphaTestedShadows()`), `Vulkan/GraphicsPipeline.cpp:configureColorBlendState()`.

### Fresnel Effect (Reflection + Refraction)

When both reflection AND refraction components are present:

1. **`fresnelFactor` is auto-generated** by `LightGenerator.cpp` (the PBR glass IBL block) during
   shader generation — the legacy material used to emit it itself, and no longer exists
2. It's computed with the Schlick approximation from the dielectric F0 (0.04)
3. The lighting code in `LightGenerator.cpp` uses it to blend between reflected and refracted colors
4. **`ior`** (offset 8) **is clamped** to [1.0, 3.0] - values below 1.0 (like 0.33) become 1.0

### Material Types Registration

> [!CRITICAL]
> **All material types must be registered in `Material::Types` array!**
>
> The `Material::Types` array in `Materials.hpp` is used by `FastJSON::getValidatedStringValue()`
> to validate material type strings from JSON. If a type is missing, it falls back to `BasicResource`.
>
> **Bug pattern (fixed Jan 2026):** the lit material's `ClassId` was missing from `Material::Types`,
> so a mesh JSON declaring it silently fell back to Basic — a lit material loaded as a cheap one.
>
> ⚠️ **Live trap since the material merge:** the array holds exactly TWO entries,
> `MaterialBasicResource` and `MaterialStandardResource` — the ClassId `"MaterialPBRResource"` no
> longer exists. Any manifest still carrying it fails validation and falls back to **Basic**, in
> silence. (The scene-definition JSON is a separate path: there the `"Type"` strings `"Standard"`
> and `"PBR"` are accepted as synonyms — `Scenes/DefinitionResource.cpp`.)
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

if ( materialType == StandardResource::ClassId )
    return loadMaterial.operator() < StandardResource > ();
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
> - `Saphir/AbstractVertexStage.cpp:synthesizeTangentToWorldMatrix()`

---

## Ray Tracing / Acceleration Structures

### CDLOD terrain — what the adaptive grid taught, and the traps of its replacement (2026-09-22)

Every `TerrainResource` floor is `Geometry::CDLODTerrainResource` since 2026-09-22: one shared patch
displaced by a height clipmap, Strugar's selection and geomorph, per-pixel normals, a dedicated
ray-tracing proxy (`src/Graphics/AGENTS.md` § "Adaptive geometries"). It replaced
`AdaptiveVertexGridResource` (deleted). What that grid's day of fixes taught still holds:

- ⚠️⚠️ **A terrain's rendering buffers are not traceable.** The adaptive grid's IBO stacked every LOD
  of every sector (converting it put the terrain in the BLAS eight times over), and it did not even
  override the strip conversion, so every terrain was ABSENT from the TLAS while the log repeated
  `uses TriangleStrip but generateTriangleListIndicesForRT() returned empty indices` (2237 times in two
  minutes: `SceneMetaData` retries on every TLAS refresh). The CDLOD patch is flatter still — so the
  terrain traces a DEDICATED proxy built from the CPU grid (`Interface::dedicatedRTVertexBufferObject()`,
  read by the BLAS build AND the hit-shading metadata through `rtVertexBufferObject()` / `rtVertexFlags()`;
  a geometry that gave the BLAS one buffer and the hit shaders another would shade the wrong triangles).
- ⚠️⚠️ **The full-resolution surface is quadratic and unbounded**: 4096 m at 1 m is 33.5 M triangles,
  **7.5 GiB against 5.05 GiB** of VRAM measured on an 8 GiB card. Hence `Core/Graphics/RayTracing/TerrainBLASMaxTriangles`
  (2 000 000, owner decision): the finest power-of-two step that fits — step 8, 524 288 triangles, for the
  4096 m proxy.
- ⚠️⚠️ **A geometry that replaces what it traces must say so, LAST**: the proxy re-centres as the camera
  travels, and the BLAS keeps the previous surface without a word — no error, no VUID. The swap calls
  `markAccelerationStructureStale()` as its last statement; the old buffers leave through the
  `DeferredDestructor`. Proving it is a LOG measurement (one rebuild per re-centre), never two images:
  two runs do not move the same way.
- ⚠️⚠️ **A move decision is a DISTANCE between the wanted centre and the held one, never an inequality**
  (`Grid::subGridCenter()` clamps at the grid border, where the camera stays past any threshold for
  ever): the grid once regenerated its whole 896 MiB window six times in eight seconds while the camera
  drifted along the border. The proxy uses the same test.
- ⚠️⚠️ **Two cameras, two questions.** The LEVEL comes from the camera whose picture it is — for a shadow
  map the MAIN camera (`Scene::castShadows()`), never the light, or the shadow swims at every boundary —
  and the FRUSTUM is the pass's own. The shadow pass once drew the adaptive grid's whole IBO (46 M indices
  per shadow target) because it did not take the adaptive branch; both paths go through
  `drawAdaptiveGeometry()`.
- ⚠️ **A level of detail is chosen from the distance to the node's BOX SURFACE** (`AACuboid::distanceTo()`,
  with each node's own Y range): from the sector CENTRE, the sector under the camera sat at 8 m quads and
  the 1 m data was never drawn — a 0.6 ms `ScenePass` that looked fast because it drew nothing.
- **Two measurement traps**: the `terrain` sun is ANIMATED (11.5 % of mean luminance between two runs
  80 s apart at a pinned exposure) — bench with `--demo-options 100000,25`; and `forest`'s wind moves
  31.6 % of the pixels within one run — attribute on ground-only blocks.

Traps of the CDLOD work itself:

- ⚠️⚠️⚠️ **`PushConstantBlock::bytes()` counted a `vec2` as 16 bytes** (it summed `size_bytes()`, the
  PADDED size) while std430 packs it in 8. The range only grew, so nothing ever failed — until the
  heightfield computed its node offset from it: the CPU pushed the node at **96** while the shader read it
  at **80**, every terrain vertex went to garbage, and the frame showed **no terrain and zero validation
  messages** (the push landed inside the range). Diagnosed by tracing the pushed offset against the
  generated GLSL. `bytes()` now uses the exact std430 sizes (vec2 8, vec3 12, vec3 arrays on 16).
  ⚠️ The exact size then revealed a second, older lie: the post-processing block declared 24 B while
  `PostProcessor::PushConstants` pushes 28 (`deltaTime` was missing from the GLSL) —
  `VUID-vkCmdPushConstants-offset-01795`, now declared. **A push struct and its GLSL block are mirrors:
  change both.**
- ⚠️⚠️ **Saphir emitted the functions BEFORE the uniform blocks, samplers and push constants**: a
  function reading any of them did not compile. They are emitted last now, right before `main()`.
- ⚠️⚠️ **A vertex-stage preparation asked after `generateMainUniqueInstructions()` is never emitted**
  (the normal matrix of the pixel-frame outputs named a variable nothing declared): the heightfield's
  pixel-frame outputs are declared before it.
- ⚠️ **The clipmap is written on the GRAPHICS queue ahead of the frame, never through the transfer
  queue's region upload**: `Image::writeDataRegion()` transitions the whole image on another queue while
  frames in flight sample it. A barrier recorded on the graphics queue has every earlier command in its
  first scope, and the frame's fence covers the submission.
- ⚠️ **R16 heights are 6 cm of quantum over the `terrain` range (-2000 m + [0, 4000] m)**: the normals are
  baked from those 16-bit heights. No banding was seen on the first captures; if contouring ever appears
  on gentle lit slopes, the culprit is this quantum, not the bake (owner decision R16, 2026-09-22).

### Measured: the diamond-square's finest level is white noise at the vertex frequency (2026-09-22)

`Algorithms::DiamondSquare` displaced level k by `roughness × halfSize` — a fixed 2⁻¹ decay per level
(Brownian, H = 1) with `roughness` only weighing the whole noise against the corner values before a
normalisation that erased even that. On a 1 m grid the last level deposits ±0.4 m per vertex, which a
lit mesh shades as a **regular woven lattice** aligned on the grid (the `forest` note that "the
division count is a look, not a quality knob" was this). `DiamondSquareParams::hurst` (default 1 =
the historical relief; declared LAST so a positional `{factor, roughness, seed}` keeps its meaning;
JSON key `Hurst`) sets the per-level decay `2^-H`. Measured on a 4096 × 1 m grid with the `terrain`
slopes: vertex-frequency curvature **0.44 m → 0.083 (1.25) → 0.017 (1.5) → 0.004 (1.75)**, 64 m slope
0.25 → 0.19 → 0.16 → 0.15. `terrain` runs at 1.25: 1.5 read as a flat sheet up close. ⚠️ The
generator's other signature, straight subdivision creases along the grid axes, remains at every H —
it is the algorithm's, not the exponent's.

**Files**: `src/Graphics/Geometry/{Interface,CDLODTerrainResource}.{hpp,cpp}`, `src/Graphics/Geometry/HeightfieldSurface.hpp`, `src/Saphir/Generator/HeightfieldSurfaceHelper.{hpp,cpp}`, `src/Saphir/{VertexShader,FragmentShader}.{hpp,cpp}`, `src/Saphir/Declaration/PushConstantBlock.cpp`, `src/Graphics/Renderable/TerrainResource.{hpp,cpp}`, `src/Graphics/RenderableInstance/Abstract.{hpp,cpp}`, `src/Graphics/Renderer.{hpp,cpp}`, `src/Scenes/Scene.rendering.cpp`, `src/SettingKeys.hpp`; emeraude-base `Math/Space3D/AACuboid.hpp`, `Algorithms/DiamondSquare.hpp`, `VertexFactory/Grid.hpp`.

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
> **Old check** (`Graphics/Effects/Lighting/RTR.cpp` ~line 372):
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
> - `Graphics/Effects/Lighting/RTR.cpp` — trace shader self-rejection check

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
> **Data layout:** `GPUMeshMetaData` grew from 32 B to 48 B (3 `uvec4` instead of 2).
> RTR/RTGI shader indexing uses `* 3u` stride. The third `uvec4` held
> `materialIndices[4]` inline until Sep 2026; it now holds the offset of the
> instance's first row in a shared **sub-geometry table** — see the section below,
> which also fixes what that inline array was silently getting wrong.

### Fixed: the RT hit shader read the WRONG TRIANGLE of a multi-layer mesh (Sep 2026)

> [!CRITICAL]
> **The BLAS partition above gives each sub-geometry its own `primitiveOffset` into
> ONE SHARED index buffer. `rayQueryGetIntersectionPrimitiveIndexEXT` therefore counts
> from the start of the HIT geometry, while the shader indexed the shared buffer as if
> the count were absolute — so every hit on a sub-geometry other than the first read
> another sub-geometry's triangle.** Right material, wrong vertices: wrong UVs, wrong
> normals.
>
> **Symptom, owner-reported 2026-09-13 on `light-and-shadow-debug --demo-options 0,1`:**
> in the mirror floor's ray-traced reflection the palm TRUNK's bark was stretched, with a
> hard, sharply bounded dark band at its base. The trunk rendered directly was correct —
> the raster passes `subGeometryRange(i)[0]` as `firstIndex` to `vkCmdDrawIndexed`, so
> only the RT path was affected. The palm's two layers are leaves (sub-geometry 0, 596
> faces) then bark (sub-geometry 1), so each of the 80 trunk triangles read one of the
> first 80 LEAF triangles: the bark texture landed on leaf UVs (the stretch) and the
> borrowed normals faced away from the lights wherever a run of them agreed (the band).
> The leaves looked fine because they ARE sub-geometry 0.
>
> **Fix:** a per-sub-geometry indirection table, `GPUSubGeometryData` (set 0, **binding 4**),
> one `uvec2` row per BLAS geometry — `.x` = first index in the shared buffer, `.y` =
> material index. `GPUMeshMetaData` carries `subGeometryTableOffset` and the row count, so
> a hit reads row `offset + geometryIndex`. The shared GLSL is `EMEN_RT_SUBGEOMETRY_GLSL`
> (`rtHitFirstIndex()`, `rtHitMaterialIndex()`), spliced before every `getMeshAccessor()`,
> whose signature now takes the geometry index.
>
> ⚠️ **The table is filled from the partition the BLAS was ACTUALLY built with**, never
> re-derived: `Geometry::Interface::BLASGeometryFirstIndices()` publishes it, and the
> skinned path reads `RenderableInstance::Abstract::rtRefitInputs()`. Two sites deciding
> "do we partition?" separately would drift the day one of the do-not-partition cases
> changes (a single sub-geometry, or the TriangleStrip CPU-index fallback whose converted
> indices no longer map to the original ranges).
>
> **The inline `materialIndices[4]` is gone with the same change.** It capped a mesh at 4
> sub-geometries while the BLAS partitioned without a cap, so groups 5 and beyond silently
> took material 0 — `Humans/OldMan` carries seven. The table has no per-instance ceiling;
> only the scene total is bounded (`SceneMetaData::MaxRTSubGeometries`).
>
> **Caveat — animated sprites:** the procedural sprite quad builder emits one group per
> animation frame slot (`MaxFrames = 120`), so the BLAS carries many more geometries than
> the renderable has materials (1). Each still gets a row, with the correct first index;
> a row past the renderable's last material slot takes material 0.
>
> **Files involved:**
> - `Vulkan/AccelerationStructureBuilder.{hpp,cpp}` — `buildBLAS` takes
>   `std::vector<BLASGeometryInput>`, each input has a `firstIndex` field
> - `Graphics/Geometry/Interface.{hpp,cpp}` — partitions by `subGeometryRange(i)` and
>   publishes the result through `BLASGeometryFirstIndices()`
> - `Scenes/GPUMeshMetaData.hpp` — 48 B instance entry + the 8 B `GPUSubGeometryData` row
> - `Scenes/SceneMetaData.{hpp,cpp}` — builds and uploads the table per frame-in-flight
> - `Graphics/Renderer.cpp` — RT descriptor set binding 4
> - `Graphics/Effects/Shared/RTAlphaTestGLSL.hpp` — `EMEN_RT_SUBGEOMETRY_GLSL`, and
>   `getMeshAccessor(instance, geomIdx, primitive)`
> - `Graphics/Effects/Lighting/RTR.cpp`, `RTGI.cpp`, `Graphics/IrradianceProbeVolume.cpp`
>   — call sites; RTAO and RTContactShadows inherit it through `EMEN_RT_SCENE_DATA_GLSL`

### Fixed: every ray-traced reflection was PI times too bright (Sep 2026)

> [!CRITICAL]
> **RTR shaded its hit points without the Lambert `1/PI` normalisation, on both the direct
> diffuse lobe and the flat scene ambient. A reflected surface came out PI = 3.14 times
> brighter than the same surface rendered directly — a passive mirror outshining its
> source.**
>
> **Measured 2026-09-13 on `light-and-shadow-debug --demo-options 0,1`** (mirror floor:
> albedo 1, roughness 0, metal — a PERFECT mirror, so the reflection must carry
> essentially the full luminance of what it reflects). Reflected/direct luminance of the
> same object inside ONE frame, linearised through gamma and the ACES curve — a ratio
> taken inside one frame is exposure-independent, which is what makes two auto-exposed
> captures comparable:
>
> | Object | Screen-space | Ray-traced, before | Ray-traced, after |
> |---|---|---|---|
> | Brick cube | 0.40 | **2.26** | 0.82 |
> | Sphere | 0.40 | **1.73** | 0.83 |
> | Palm trunk | 0.52 | **1.82** | 0.80 |
>
> **Why RTR alone had it:** the raster emits `kD * albedo / 3.14159265`
> (`Saphir::LightGenerator::generatePBRFragmentShader`) and `iblBaseColor * 0.3183098862`
> for the scalar ambient (`LightGenerator.cpp`); RTGI emits `albedo / PI`; the irradiance
> probe volume divides its gathered irradiance by PI. RTR was the only member of the
> family without it. ⚠️ The two other irradiance sources it adds — the probe query and the
> bindless irradiance cube — already store `E/PI`, so they must NOT be divided again.
>
> **Same change, second divergence:** the radius falloff. The raster uses
> `max(1 - dot(d/r, d/r), 0)` and no attenuation at all when the radius is 0; RTR, RTGI and
> the probe volume used `clamp(1 - d/r, 0, 1)²` and an inverse square. The two curves return
> 0.25 against 0.75 at half the radius, so a traced surface was lit by a third of what the
> rendered one received at mid-range. All three now use the raster curve verbatim.
>
> ⚠️ **What this reveals about the screen-space lane:** on a PERFECT mirror the ray-traced
> ratio is now ~0.8 (the effect's own `intensity` 0.8 times the distance fade), which is
> the physically expected figure, while the screen-space lane sits at 0.38-0.50 — it is the
> LOW one. Its `facingFade = 1 - pow(dot(viewDir, reflDir), 5)` (`SSR.cpp`) costs roughly
> half the reflection on a floor seen at a shallow angle. That term hides a real screen-space
> limitation (a ray coming back toward the camera cannot be traced), so it is not a defect to
> delete — but "screen-space is the correct level" no longer holds, and the two lanes are
> expected to disagree here.
>
> **Still divergent, deliberately left open** (owner scoped the fix to the free
> divergences, 2026-09-13): no ambient occlusion at the hit, no Fdez-Agüera multi-scatter
> deduction on the hit's IBL diffuse, the material's own `IBLIntensity` dropped, no colour
> projection (gobo), binary shadow ray against a filtered shadow map. Each makes a
> reflection brighter than the rendered surface and each needs new data in the RT SSBOs.

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
> - `Graphics/Effects/Lighting/RTGI.cpp` — `shadowRayVisibility()` + gated
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
> the ambient/simple pass from `LightGenerator::albedoShaderExpression()` (Standard albedo /
> Basic diffuse / white fallback), and consumed by SSGI's apply pass
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
> `Saphir/Generator/SceneRendering.{hpp,cpp}`, `Graphics/Effects/*/*.{hpp,cpp}`
> (signature), `SSGI.{hpp,cpp}` (consumer).
>
> **Update (Aug 2026):** RTGI now follows the SAME convention as SSGI — see the next section.

### Fixed: RTGI Applied Receiver Albedo Before the Blur — Texture Detail Destroyed in Dark Areas (Aug 2026)

> [!WARNING]
> **Symptom:** in GI-dominated areas (dark zones, direct light ≈ 0) the RTGI contribution
> visually REPLACES the pixel, and because the trace multiplied the receiver albedo at
> HALF resolution BEFORE the bilateral blur + temporal accumulation, the texture detail
> embedded in the GI term was blurred away — mushy stone/floor textures in shadowed Sponza.
>
> **Rule — albedo demodulation (applies to ANY future GI/denoise path):** the
> denoise/temporal chain must carry **demodulated irradiance only**; the receiver albedo is
> re-applied at FULL resolution in the combine pass (`CombineContribution::needsAlbedo` +
> `gi *= texture(emAlbedo, vUV).rgb`). Standard practice: SVGF (Schied et al. 2017, HPG),
> NVIDIA NRD. Both SSGI and RTGI now share this convention.
>
> **Energy coupling — multi-bounce feedback:** with a demodulated cache (the screen history until
> 2026-09-12, the irradiance probe volume since), the feedback read at bounce hits is irradiance,
> NOT outgoing radiance: it MUST be multiplied by the HIT surface's albedo at consumption
> (`albedo * probeFeedback(hitPos, hitNormal, sampleDir)` in the trace shader). Forgetting that factor makes the geometric series undamped (`1/(1-strength)`,
> ×5+ energy runaway on bright walls); double-applying it kills the bounce fill.
>
> **Validated A/B (2026-08-05):** Sponza dark corridor — global luminance ratio 0.996,
> floor texture gradient ×1.64; Cornell GI demo — uniform ≤2% run-to-run drift, colour
> bleed hue preserved, no runaway.
>
> **Files:** `Graphics/Effects/Lighting/RTGI.{hpp,cpp}` (trace shader, descriptor set 1
> renumbered — albedo binding removed, history=2, frame UBO=3; `combineContribution()`;
> `readsChainColorUpstream()` now `false`).

### Fixed: Undithered Radial God Rays = Flickering "Dash Train" Under TAA (Aug 2026)

> [!WARNING]
> **Symptom:** a receding line of small bright dashes on the floor (Sponza dark corridor,
> toward the lit doorway), flipping violently with the TAA jitter phase — the single most
> unstable feature of the whole frame on the temporal-stability map (peak-to-peak flips of
> 220/255 while the GI mottle peaked at 12).
>
> **Root cause (VolumetricLight, radial-blur god rays):** the radial march took `numSamples`
> UNIFORM steps from the pixel toward the light's screen position with no dither. A bright
> source SMALLER than one step (a door slit, a sky gap in the occlusion mask) falls between
> taps: each tap that does catch it paints a discrete ghost copy of the source along the
> radial direction — banding, not noise. Under TAA the occlusion mask is cut from the
> JITTERED depth buffer, so every sub-pixel wiggle of the source silhouette replicates onto
> the entire dash train at once.
>
> **Fix:** offset the march start by a per-pixel fraction of ONE step using interleaved
> gradient noise (J. Jimenez, "Next Generation Post Processing in Call of Duty: Advanced
> Warfare", SIGGRAPH 2014). The banding dissolves into fine sub-step speckle that the radial
> accumulation and TAA smooth. Measured (corridor bench, 8-shot series ×2): trail-region
> pixels flipping >64/255 went 1979-2707 → 210-350 (÷9), full-frame p99.9 halved (50-59 → 26-32).
>
> **The dither is deliberately STATIC per pixel** — same rule as the RTGI noise (see
> "Animated GI Noise" below): TAA integrates over its own jitter; a frame-varying dither
> fights the history and regresses.
>
> **Rule for ANY stepped screen-space march** (god rays, SSR, SSGI, volumetric fog): never
> ship uniform steps without a per-pixel dither of the march origin — undersampling shows up
> as coherent, TAA-hostile banding on any source smaller than the step.
>
> **RESIDUAL RESOLVED (2026-08-06) — the streaks themselves still vibrated ("le filet").**
> Attribution A/B (effect removed): ~90% of the door+trail region's mid-tail instability
> (pixels flipping >16/255: 4200–8500 → ~450) was still VolumetricLight. Three
> SAMPLING-side hypotheses were implemented and measured NEUTRAL: a fractional 2×2-gather
> occlusion mask, jitter-compensated depth sampling (vUV + jitterUV), and an unjittered
> light-position projection. The actual mechanism: **a source narrower than a pixel
> RASTERIZES differently at every TAA jitter offset** — the door slit's footprint in the
> depth buffer genuinely changes with the Halton phase, so the occlusion mask flux truly
> oscillates and the radial march integrates that into a streak-scale vibration. No stable
> sampling can fix a source that really changes; **averaging over the jitter cycle can**:
> a temporal EMA on the occlusion mask (ping-pong pair, blended in the occlusion pass,
> `Parameters::temporalAlpha` default 0.2 ≈ 8 frames, 1 = off, no reprojection — the mask
> is soft and view-anchored). Measured: the region returned exactly to the
> effect-removed floor (px>16 ≈ 450, crop p99.9 40-53 → 11-12, full-frame p99.9 18-26 →
> 6.6), energy preserved, owner-validated live (static + fast camera rotation, no visible
> mask ghosting). The neutral sampling corrections were KEPT (principled, zero-cost:
> the gather anti-aliases the half-res mask, the compensation stabilises the silhouette
> position); the EMA is the active ingredient.
>
> **Lesson:** when a post-effect amplifies a tiny bright source (radial blur, bloom
> streaks), check whether the source's RASTERIZED footprint is jitter-stable before
> blaming the effect's sampling — a sub-pixel source under TAA is a genuinely oscillating
> signal, and the only cure at the effect level is temporal.

### Measured: Animated GI Noise Cannot Beat a Frozen Pattern Under a Fixed-Alpha EMA (Aug 2026)

> [!WARNING]
> **Context:** the RTGI noise seed is purely spatial (`hash2(gl_FragCoord)`, no frame index) —
> the pattern is frozen by design. With TAA active, the Halton jitter makes that static pattern
> shimmer (owner-isolated: switching TAA → FXAA freezes it — the jitter only exists when the
> stack requires it, `PostProcessStack::requiresJitter()`), through two paths: the re-rasterized
> G-buffer wiggles the trace inputs, and the TAA resolve resamples a high-frequency static
> pattern at sub-pixel offsets. NOT matrix-related: swapping the effect's projection matrix for
> the unjittered form had no measurable effect (2026-08-05, note in RTGI.cpp).
>
> **The measured lesson:** animating the seed (R2 sequence per frame) to "let the temporal EMA
> average the error" REGRESSED the temporal peak-to-peak ×2.4 (Sponza corridor bench: mean
> 0.67 → 1.65, >4/255 area ×9, both runs concordant) with no spatial gain. A fixed-alpha EMA
> retains `α/(2−α)` of the input variance (≈23% at α=0.1) — and a frozen pattern's temporal
> variance is ~zero by construction, so ANY seed animation loses on that metric. An NRD-style
> per-pixel 1/N accumulation counter only reaches parity (computed at N=32-64).
>
> **The rule:** seed animation is only viable AFTER the per-frame estimator noise is cut ahead
> of the resolve — a variance-guided à-trous filter chain (SVGF, Schied et al. 2017) or more
> samples. Measurement protocol: projet-alpha `docs/temporal-stability-measurement.md`
> (8-shot series ×2 runs — single runs differ by up to ×1.85 and prove nothing).
>
> **RESOLVED (2026-08-06):** the SVGF chain landed in `Graphics::GIDenoiser` (variance-guided
> à-trous + per-pixel 1/N accumulation counter) and `Temporal/AnimatedNoise` flipped to
> **default ON** (owner decision, owner-validated live). Measured with the full chain: ptp
> 0.55–0.57 vs the 0.67–0.83 marbled baseline, energy restored, fireflies dissolved. The rule
> above STANDS for any new temporal estimator: a frozen pattern also poisons the SPATIAL
> filter — stable bright outliers have near-zero temporal variance, so a variance-guided
> luminance weight protects them as "converged signal" (visible fireflies). Animation and
> the denoiser are a package: neither works alone.

### Fixed: the screen-space lane lit enclosed spaces with the UNOCCLUDED sky (Sep 2026)

> [!CAUTION]
> **Measured on `sponza`, upper gallery, exposure PINNED (f/11 · 1/250 s · ISO 100), 2880×1620,
> RTX 3070 Ti, 0 VUID: the same frame read 11.2/255 mean in the RayTracing lane and 52.8 in the
> ScreenSpace lane.** The galleries were lit as if the courtyard had no roof.
>
> **The cause was the indirect-diffuse OWNERSHIP contract, not a knob.** `RTGI` claimed the indirect
> diffuse, so `Scene::updateIBLDiffuseOwnership()` switched the raster ambient pass' diffuse IBL leg
> off and the sky reached every surface through rays that measure its visibility. `SSGI` claimed
> nothing and had no sky term, so the weight stayed at 1 and the ambient pass applied the **whole
> irradiance cubemap** to every surface — attenuated only by the short-range SSAO, with the SSGI
> bounce added on top.
>
> **The fix**: SSGI runs a **GTAO horizon search** (Jimenez et al., SIGGRAPH 2016; Intel XeGTAO as
> the implementation reference) and composites `irradianceCube(bentNormal) * skyLuminance * V` inside
> its trace, so the sky rides its existing SVGF denoiser. It then claims the ownership like RTGI.
> Full description, settings and acceptance table: `src/Graphics/AGENTS.md` § "The screen-space sky
> visibility".

**Three traps this defect and its measurement have already sprung:**

- ⚠️⚠️⚠️ **Identify WHAT the crop holds before measuring it — geometry, or the background?** The
  first acceptance table of this fix reported "open sky: ratio 1.0000, bit-identical", measured on
  "the grass outside Sponza". **That grass is the SKYBOX**: the demo builds no ground (no
  `enableBasicGround()`) and the Intel asset is the building alone, so everything below the edge of
  its platform is the Kloppenheim05 cubemap, meadow and rocks and flowers included. A background is
  drawn unlit, writes no albedo and no material-properties G-buffer: NO lighting term can move it,
  in any lane, ever — so an A/B on it returns a perfect ratio for every term, and reads as a proof.
  The valid open-sky surface on that scene is the ROOF (real geometry, V ≈ 1). Two rules, in order:
  name the surface, then check that it MOVES with the term under test. **A ratio of exactly 1.0000
  is not a result, it is a warning.**
- ⚠️⚠️⚠️ **Measure a surface's sky visibility, never reason about it from the floor plan.** Put the
  camera at the point and look straight up (`setPosition(x, y, z)` + `lookAt(x, y+10, z+0.01)`), then
  integrate the sky pixels of the capture weighted by `cos⁴θ` (the cosine-weighted solid angle of a
  rectilinear image plane; an 85° vertical FOV covers 62 % of the cosine-weighted hemisphere). It
  takes 30 seconds and it is exact. Reasoning instead — "a courtyard is open, so `V = sin(atan(W/2H))`
  ≈ 0.45" — put a 12× error into a published table and opened an item against the wrong lane: that
  Sponza floor is under the arcade and really sees **V ≥ 0.036**, which is what the traced lane
  delivers. See `src/Graphics/AGENTS.md` § "The screen-space sky visibility".
- ⚠️⚠️ **`global-illumination` is NOT a bench for a sky term.** The demo declares no background and
  `setAmbientLightIntensity(0)`, so `FrameContext::skyLuminance` is 0 AND the reserved irradiance
  cube slot holds the engine's default **black** 16² cubemap: every sky term, raster or effect, is
  exactly zero there and an A/B measures nothing. The item's plan named it as the "no double count"
  test; it took a look at `onSetupLighting()` to see it could not be. A sky bench needs a scene with
  a background AND a lit surface that sees the sky — on `sponza` that is the ROOF
  (`setPosition(-9, 22, 0)` + `lookAt(-9, 0, 2)`), never the meadow under the horizon, which belongs
  to the cubemap (see the rule above).
- ⚠️⚠️ **A pinned exposure is pinned to A SCENE.** `f/11 · 1/250 s · ISO 100` is the daylight triad
  the Sponza measurements use; on the (indoor, one-omni) `global-illumination` bench the same triad
  renders a mean of 0.0/255 — a black capture that looks exactly like a broken lane. Bracket the
  triad on the scene at hand before concluding anything (that bench sits at `f/5.6 · 1/60 s ·
  ISO 800`, mean 123).
- ⚠️ **Comparing two lanes in DISPLAY space understates everything.** The tone mapper is ACES plus a
  gamma: on the gallery pose the display-space gap reads 4.6×, the linearised one 8.4×. Undo the
  gamma, then invert the ACES fit `(x(2.51x+0.03))/(x(2.43x+0.59)+0.14)` — a quadratic — before any
  ratio.

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
> - `Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/` gained `Intensity` (1.0), `Bias` (0.005),
>   `MaxDistance` (2.0), `BlurRadius` (4), `NormalSigma` (0.5).
> - **First screen-space group** `Core/Graphics/PostProcessing/AmbientOcclusion/ScreenSpace/`:
>   `Radius` (0.5), `Intensity` (1.0), `Bias` (0.025), `SampleCount` (32) — read by
>   `SSAO::create()` (which previously read no settings at all).
> - `Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/` (same day): `MaxDistance` (5.0),
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
> **Files involved:** `Scenes/LightSet.cpp` (flag), `Graphics/Effects/Lighting/RTR.{hpp,cpp}`
> (shadowRayVisibility + gating + ambient push constants), `RTGI.cpp` (gating).

> **Addendum (Aug 2026) — two more ways the reflection diverged from the raster, both fixed:**
> - RTR shaded its hit points with `LightSet::ambientLightIntensity()`, which is NOT what the
>   raster shades with: under a sky-driven scene the raster's scalar ambient is ZERO (the
>   irradiance cubemap replaces it) while the LightSet keeps the manifest's 17 000 lx. Every
>   reflection carried that flat ambient on top of its IBL. `Scene::effectiveAmbientIlluminance()`
>   is now the single rule, delivered as `FrameContext::ambientIlluminance`.
> - RTR's SHADOW ray (and both RTGI rays) used `gl_RayFlagsOpaqueEXT`, which accepts every
>   triangle of a cutout instance whole — a leaf shadowed as a solid quad. Every scene ray now
>   applies the ONE shared alpha-test rule of `Effects/Shared/RTAlphaTestGLSL.hpp`.
>   RTAO and ContactShadows apply it too — ⚠️ at a price that scales with the ray count: RTAO at
>   full resolution × 8 spp doubled (12.3 → 26.3 ms on Sponza's ivy), at half resolution it costs
>   7.2 ms, i.e. less than the uncorrected full-res effect. The resolution is the owner's setting.

### Fixed: every "dynamic" material property was DEAD after creation (Sep 2026)

> [!WARNING]
> **Symptom:** `SkyBoxResource` dimmed its material's emissive strength for the night
> (`StandardResource::setEmissiveStrengthValue()`, documented "a dynamic property") and the drawn
> sky stayed at full daylight while the IBL, fed by the same luminance, went dark. Not specific to
> the sky: 39 setters of `StandardResource` (`setRoughness`, `setOpacity`, `setAlbedoColor`,
> `setIBLIntensity`, `setEmissiveStrengthValue`, …) wrote the CPU copy and raised
> `m_videoMemoryUpdated`, and **nothing consumed that flag** — `updateVideoMemory()` had exactly one
> caller, `create()`. A material changed once on the GPU, at creation, ever.
>
> **Fix (contract):** the setters go through `markVideoMemoryDirty()`, which raises the flag ONCE
> per flush and registers the material with `Renderer::requestMaterialVideoMemoryUpdate()`
> (thread-safe, any thread); `Renderer::flushMaterialVideoMemoryUpdates()` drains the list on the
> render thread once per frame, right after the in-flight fence and before the scene's own
> uploads, calling the now-virtual `Material::Interface::updateVideoMemory()`. ⚠️ A material element
> lives in ONE frame region (the material UBO is not frame-partitioned): the write may land while a
> previous frame still reads it — for a property VALUE that is "one frame early at worst", never a
> structural hazard. Frame-partitioning the material UBO is the day a property must be exact per
> frame. **Rule:** a "dynamic property" is only dynamic if something uploads it — grep the consumer
> of a dirty flag before trusting the word in a doc comment.

### Fixed: a component moving its OWN entity from processLogics() deadlocked the logic thread (Sep 2026)

> [!CAUTION]
> **Symptom:** the whole engine froze on a BLACK frame the moment `sponza --demo-options 1` (the
> animated sun) started — the remote console still answered (main thread alive), nothing else did,
> no validation error, no log line after the scene load. `Core.shutdown()` was accepted and never
> completed. Both the owner's build and the AI's reproduced it identically.
>
> **Root cause:** `AbstractEntity::processLogics()` walks `m_components` UNDER `m_componentsMutex`
> and calls each `processLogics()`. `Component::SunCourse` aims its pivot there through
> `Node::setPosition()` → `onLocationDataUpdate()` → `AbstractEntity::onContainerMove()`, which
> took the SAME mutex to dispatch `move()` to the siblings. A `std::mutex` is not recursive:
> the logic thread waited for itself, forever. The scene never published a state again.
>
> **Fix (contract, two layers — owner rule: if a deadlock is easy to write, make it impossible):**
> 1. `m_componentsMutex` is a **`std::recursive_mutex`**: any same-thread re-entry from a
>    component's `processLogics()` — a move, `getComponent()`, `forEachComponent()`,
>    `containsComponent()` — is legal by construction. The cost is an owner check per lock,
>    once per entity per cycle.
> 2. `onContainerMove()` requested while the component loop runs is still **DEFERRED** — the
>    coordinates are kept (`m_deferredMoveCoordinates`) and dispatched right after the loop, in
>    the same cycle (the last request of a cycle wins) — so the siblings are never moved in the
>    middle of their own iteration. Same pattern as the deferred collision-boundaries refresh.
>
> What the recursive mutex would have let through is refused instead: `linkComponent()`,
> `removeComponent()` and `clearComponents()` called from inside the component loop trace an
> error and do nothing (they would invalidate the running iteration). A component that wants to
> go returns `true` from `shouldBeRemoved()`. **A component may move or query its entity from
> `processLogics()`**; it is the entity's job to make that safe, never the component's to know.
>
> **Second defect found on the way — a DISABLED light stopped tracking its frame.** The three
> `move()` overrides (`DirectionalLight`, `PointLight`, `SpotLight`) returned early on
> `!isEnabled()`, so a light moved while off and switched on later kept a STALE direction, shadow
> frame and light-space matrix (whatever its last enabled `move()` had staged). Removed: a light
> tracks its frame whether it emits or not — emitting is the render passes' decision, and they
> skip a disabled light themselves. And `DirectionalLight::resolveDirection()` (the one expression
> behind the uniform lane and the shadow camera) falls back to the frame's forward axis for a light
> AT the origin in position-to-origin mode: normalising a null position wrote NaN into the light
> buffer, which the vertex shader normalises again.
>
> **Rules:** anything a component triggers from `processLogics()` that reaches back into its
> entity (a move, a notification, a boundaries change) goes through a DEFERRED path consumed
> after the lock — never a direct re-entry. A freeze with a live console and a black frame is a
> logic-thread deadlock first: look for a lock taken twice on the same thread before anything GPU.

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
> **Verification:** the check needs the two shading families in one scene × high/low quality. Since
> the material merge, `StandardResource` IS the PBR material, so the non-PBR half must be a
> **`BasicResource`** material (WaterWorld's floor and sea are both Standard now). Toggle
> `EnableHighQuality` and confirm zero shader errors in both.

### Fixed: Direct diffuse was missing the Lambertian 1/pi — legacy Blinn-Phong vs PBR disagreed by a factor of pi (Jul 2026)

**Symptom.** Sunlit ground rendered markedly brighter than the sky that lit it, and brighter than a
PBR surface beside it under the same light. Owner's words: "flashy as hell". On `water-world`, sand
under a 100000 lx sun reached ~28000 nits against an 8000-nit sky dome.

**Root cause.** Light intensities are ILLUMINANCE in lux, so a Lambertian surface emits
`albedo * E * cos(theta) / pi`. That `1/pi` existed in exactly ONE place in the whole Saphir
generator — the ambient pass (`LightGenerator.cpp`, `albedo * 0.3183098862`). The PBR generator had
its own (`LightGenerator.PBR.cpp`: `kD * albedo / 3.14159265`). The **legacy Blinn-Phong direct
diffuse had none** — that path serves `BasicResource` since the material merge — so the two material
models differed by pi (~3.14x) on direct lighting.

**Fix.** `LightGenerator::generateFinalFragmentOutput()` now builds a `diffuseIlluminance`
expression (`lightIntensity * 0.3183098862`) and uses it at all four diffuse emission sites (plain,
reflection, refraction, reflection+refraction).

> [!WARNING]
> The normalisation is deliberately **NOT** folded into `finaleDiffuseFactor`: that same expression
> is reused as a raw geometric N.L term inside the PBR low-quality specular `pow()` further down, and
> scaling it there would change the highlight exponent's input rather than its energy.

**Moot since the material merge (Aug 2026):** the legacy Blinn-Phong material was deleted, so
"the legacy specular is not normalised either" no longer describes anything that exists.

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
`m_isUsingGrabPassForTransmission` from `StandardResource`. When set, the transmission term skips the
luminance scale; the reflection term always keeps it.

> [!IMPORTANT]
> **The rule: scale by the environment luminance only what came out of the environment cubemap.**
> Anything read back from the rendered scene (grab pass, and by extension any screen-space capture)
> is already an absolute luminance.

### Fixed: Beer's law absorbed over the MESH thickness, not the world one (Sep 2026)

**Symptom:** a transmissive material whose volume thickness was authored for its mesh turned a strong
colour it does not have once the mesh was scaled — a near-white `Parametrics/Diamond` on a 40-unit
model shown 5 m tall came out navy blue with `Thickness` = 20 mesh units. At scale 1 (every bench),
nothing shows.

**Cause:** `KHR_materials_volume` gives `thicknessFactor` in MESH space and `attenuationDistance` in
WORLD space. The screen-space refraction scaled the factor by `svModelScale`; the five Beer sites of
`LightGenerator.cpp` did not, and divided a mesh length by a world length.

**Fix:** the light generator receives `StandardResource::volumeThicknessWorldExpression()`, and the
model scale is synthesized for every transmissive material (`src/Saphir/AGENTS.md` § *One volume
thickness for both consumers*). ⚠️ The same rule makes a LIBRARY material scale-dependent twice over:
its thickness is in the units of the mesh it was tuned on, its attenuation distance in metres — a
parametric gem on a model of another size needs both re-derived (`forest` does it for its chick).

### Fixed: the grab pass was recreated IN PLACE while frames still used it (Sep 2026)

**Symptom:** at the act teardown of a scene holding ONE refracting material (a `Parametrics/Diamond`
chick on `forest`), `VUID-vkDestroyImage-image-01000` twice, `VUID-vkFreeMemory-memory-00677`, then the
same image/memory handles reported LEAKED by `vkDestroyDevice`. Nothing visible, and intermittent from
the outside: it fired on every run whose teardown recreated the scene render target (the log shows
`Scene render target created` twice), never on the others.

**Cause:** `Renderer::refreshGrabPass()` — called by every scene-target recreation — used
`GrabPass::recreate()`, i.e. `destroy()` then `create()` on the spot, while the grab's blit (it runs
on every frame whose scene holds a TranslucentGB object) was still in flight. `recreateSceneTarget()`,
three lines above, already retired the target it replaced through the `DeferredDestructor`; the grab
pass it then refreshed did not.

**Fix:** the previous `GrabPass` is handed to `DeferredDestructor::retireAction()` and a fresh one is
created; `GrabPass::recreate()` is DELETED so the in-place path cannot come back. Verified: 0 VUID on
the teardowns that recreate the target (3 of 3), against 3 of 3 before.

⚠️ **The rule it restates:** a GPU object a recorded command buffer may still reference is RETIRED,
never destroyed — and a "recreate" helper that calls `destroy()` is a destruction. A pre-allocated
object used only when some content is on screen (here: a refracting material) hides the defect until
that content shows up; the new content is NOT the cause.

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
  is structurally why a `BasicResource` and a `StandardResource` could never agree under one light.

**Changed sites:** `LightGenerator.PerFragment.cpp` (view space, reuses `twoSidedN` / `twoSidedV`),
`LightGenerator.PerFragment.NormalMap.cpp` (**tangent** space — N, V and H all in tangent space),
`LightGenerator.PerVertex.cpp` (view space, computed in the vertex shader). The function names are now
truthful, so nothing was renamed.

> [!CAUTION]
> **Material shininess values are now wrong, in a specific direction.** `dot(N, H) > dot(R, V)` for the
> same geometry, so at an UNCHANGED `shininess` every highlight is **WIDER** than the Phong one it
> replaced. Verified live on `water-world`: the sand went visibly glittery. Rule of thumb — a Blinn
> exponent needs roughly **4x** the Phong one for the same visual width (`Grounds/desert001`'s 192
> wants something nearer 768). Was tracked in the historical root `TODO.md`.
>
> **RESOLVED (Jul 2026)** — both the normalisation and the shininess retune were done in one pass,
> exactly as this note asked. See the next entry, "The legacy specular was not energy-normalised, and
> `Shininess` was authored as a glossiness". The retune did NOT become a per-file sweep: the manifest
> key was re-interpreted at the parse boundary instead.

### The light RADIUS is a culling bound, not a dimmer — and an "artistic" emissive is 1 nit (Aug 2026)

> [!CAUTION]
> **Two independent consequences of the photometric migration, both of which make things
> VANISH rather than look wrong.** Found on the `game-logic` fire and explosions, which had been
> invisible since the migration.

**1. Animating the radius no longer dims anything.** The pre-photometric falloff was
`max(1 - (d/r)^2, 0)`, where the radius genuinely shaped the whole curve — so growing and shrinking
it was the natural way to animate a flash or a dying fire. The photometric falloff is

```glsl
radiusWindow = clamp(1 - (d/r)^4, 0, 1)
lightFactor *= (radiusWindow * radiusWindow) / (d * d + 1.0)
```

The window sits at **1.0 over almost the entire range** and only bites near `d == r`; the whole
falloff is carried by `1 / (d^2 + 1)`, which depends on the distance alone. So the radius is now a
**culling bound**: set it where the contribution stops mattering and leave it there. An effect that
used to breathe by scaling its radius now either does nothing visible or pops its hard edge in and
out. **Move the envelope to the intensity** — see `EffectsToolkit::FX::createFlashEffect()`, whose
keyframes are the reference shape for a detonation (instant peak, fast decay; holding near-peak for
half the duration reads as a lamp switching on, and floods the scene).

**2. An emissive authored artistically emits exactly 1 nit.** The emitted quantity is
`autoIlluminationColor * autoIlluminationAmount * emissiveStrength`, the AMOUNT is clamped to [0,1]
because it is the emissive MASK, and `emissiveStrength` defaults to 1. So the conventional
`"AutoIllumination": 1.0` — the artistic maximum — is **one candela per square metre**, which is
invisible next to anything real. The brightness belongs in `EmissiveStrength`, in nits.

⚠️ **Sprites could not express this at all until Aug 2026**: `SpriteResource::load()` parsed
`AutoIllumination` but no strength key, so a flame sprite was structurally stuck at 1 nit. It now
reads `EmissiveStrength`, the same key and contract as `StandardResource` and
`KHR_materials_emissive_strength`.

⚠️ **Reference luminances, because the intuition is wrong here**: a candle flame is ~**10 000**
nits, not 5-10 — a flame is SMALL, not dim, and its luminance must not be confused with the
illuminance it casts. Clear sky ~8000, SDR monitor 200-300, explosion fireball an order of
magnitude above a flame, sun 1.6e9.

⚠️ **Symptom shape to recognise**: both failures are SILENT. Nothing logs, nothing warns, the
geometry is drawn and the light is enabled — the contribution is simply five orders of magnitude
below what the auto-exposure is metering, so it reads as "the asset disappeared". When something
vanishes after a photometric change, check the UNITS before suspecting the renderer.

### Fixed: a radius of ZERO meant "no attenuation" to the GPU and "cull me everywhere" to the CPU (Aug 2026)

> [!CAUTION]
> **The CPU and the GPU disagreed about what a null light radius means, and the CPU won.** A light
> correctly placed, correctly valued, enabled, and listed in the light set lit **nothing at all** —
> no log line, no validation error, no warning. Found on `WorldLobby.usdz`, whose 29 fixtures were
> every one of them in this state.

The two readings, before the fix:

| Side | Code | Reading of `radius == 0` |
|------|------|--------------------------|
| GPU | `if ( lightRadius > 0.0 )` gates the distance attenuation | **unbounded reach**, no attenuation window |
| CPU | `touch()` built `Sphere{m_radius, position}` | **degenerate sphere** |

The CPU reading was fatal because of two lines nobody reads together:

- `Space3D::Sphere::isValid()` is `m_radius > 0`;
- `isColliding(Sphere, Sphere)` returns **false immediately** when either sphere is invalid.

So `touch()` answered false **unconditionally** — not "point outside sphere", not even true for geometry
sitting on the emitter. And `Scene.rendering.cpp` culls on exactly that answer, for point lights and
spotlights alike:

```cpp
if ( instance->isLightDistanceCheckEnabled() && batchCoordinates != nullptr && !light->touch(instanceWorldSphere) )
{
    continue;
}
```

⚠️ **That check is ON by default** — the flag is `DisableLightDistanceCheck`, opt-in to disable. A
zero-radius light was therefore dropped from **every draw of the scene**.

**The fix, in two halves — the second is not optional.**

1. `SpotLight::touch()` and `PointLight::touch()` (both overloads each) now return `true` when
   `m_radius <= 0`, so a null radius means unbounded reach on both sides. `DirectionalLight::touch()`
   already did exactly this.
2. `SceneDataConsumer::attachLight()` now DERIVES a culling bound when the asset declares no range,
   via `Graphics::Photometry::cullingRadiusFromIntensity()` — `r = sqrt(I / E)` with `E` =
   `Photometry::CullingIlluminance` (1 lux). Without this half, half one turns every rangeless asset
   light into an unbounded one: bound to every draw, one light pass each. **USD declares no range on
   any light type**, so this is the branch every USD fixture takes — 3751 cd gives ~61 m, against a
   lobby some 20 m across.

⚠️ **Why 1 lux**: it is far below anything an interior scene grades against (a lit room reads
200-500 lx), so the cut cannot produce a visible boundary, while still bounding reach to something the
culling can reject. The 0.05 lx photopic floor gives ~274 m and culls nothing; 5 lx gives ~27 m and
can clip a dim far surface.

⚠️ **The measurement that isolated it** was a KNOWN-GOOD CONTROL, not a code read: the player's
flashlight lit the same scene perfectly. The only relevant difference was `setRadius(30.0F)` in
`Player.cpp`. When asset lights fail and an engine-made light succeeds in the same frame, compare the
two **setters**, not the two shaders — the lit path was never the suspect.

⚠️ **Do not restate "a null radius disables the attenuation, so the symptom is an OVER-lit room"**
without this correction. That was true of the shader in isolation and false of the engine, because the
light never reached the shader. It is exactly the kind of half-truth that sends the next session
hunting an exposure bug.

### Fixed: the legacy specular was not energy-normalised, and `Shininess` was authored as a glossiness (Jul 2026)

**Two independent defects that compounded into one symptom** — every sunlit legacy Blinn-Phong
surface read as a uniform bright sheet, and the sky it stood under looked washed out by comparison.
The material that carried them was the legacy `StandardResource`, deleted by the material merge; the
shading path survives for `BasicResource`, and so does the trap. Diagnosed on
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
| Glossiness -> exponent | `specularExponentFromGlossiness()` on the legacy material, called ONLY from the two JSON parse sites | `exp2(1 + 10 * gloss)` (UE3 convention): `0.0 -> 2`, `0.1 -> 4`, `0.2 -> 8`, `0.4 -> 32`, `0.5 -> 45`, `0.9 -> 1024`. **Gone with the legacy material** — see the caution below for what consumes the key today. |

> [!CAUTION]
> **The conversion belongs to the JSON boundary and NOWHERE else — and it is a ROUGHNESS conversion
> now.** `specularExponentFromGlossiness()` died with the legacy material. The merged
> `StandardResource` is PBR and reads `"Shininess"` as the glossiness it is, storing
> `roughness = 1 - glossiness` (`parseRoughnessComponent()`). Nothing downstream re-interprets it: a
> value held by a `StandardResource` is a roughness in `[0,1]`, full stop.
>
> ⚠️ **`BasicResource` did NOT follow, and that is the live trap.** It still consumes `"Shininess"`
> (or `"Value"`) as a RAW Blinn-Phong exponent — `BasicResource::DefaultShininess` is `200`, there is
> no remap anywhere — while the data store holds glossiness values (`0.1` in 3834 of 3917 files). A
> legacy manifest loaded as a Basic material therefore lands on an exponent of `0.1`, which the three
> legacy generators clamp to `max(shininess, 1.0)`: the widest possible lobe, i.e. exactly the
> uniform-sheet look Defect 2 describes. Convert at the call site when authoring a Basic material
> from that data.
>
> The 13 files that legitimately carried exponents were converted to glossiness in the data store
> (`gloss = (log2(n) - 1) / 10`), so the manifest key has exactly ONE meaning: a glossiness in `[0,1]`.

**Verified.** `basic-scenery`, ground + sky only, controlled camera. The rendered ground/sky mean
luminance ratio landed at **1.64**, against **1.65** computed independently from the manifest
(ground 3 070 nits = 3 011 diffuse + 59 specular; sky mean = linear(`AverageColor` 0.31) x 6000 =
1 860 nits). The ground stopped clipping (band max 238 -> 194) and the sky mean ROSE 53 -> 79,
because the exposure no longer had to absorb a 22 000-nit ground.

> [!NOTE]
> Still unnormalised, deliberately out of scope here: the **PBR low-quality specular approximation**
> in `LightGenerator.cpp` (`lqSpecPower`), which multiplies the raw illuminance and reuses `N.L`
> inside its own `pow()`. ⚠️ VOID since Aug 2026: `lqSpecPower` lived in
> `generateFinalFragmentOutput()`, written for the Gouraud path, and was deleted with it.

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

## Resources / Loaders

### Two files with the same NAME shared one resource namespace — fixed Sep 2026

> [!CAUTION]
> **A defect whose only symptom is a wrong NUMBER is the hardest kind to catch.** `GLTFLoader`'s
> per-asset resource prefix was the file **stem**, so
> `SunglassesKhronos/glTF-Binary/SunglassesKhronos.glb` and
> `SunglassesKhronos/glTF-Draco/SunglassesKhronos.gltf` — different geometry *and* texture encodings
> — shared `glTF:SunglassesKhronos/`, and the second load quietly served the first's cached
> resources. Nothing crashed, nothing looked wrong: the conformance bench simply reported **98.16 %
> of pixels differing, mean 39.28/255** where a clean session read **6.35 % / 0.10**.
>
> Fixed by keying on the whole path (`lexically_normal().generic_string()`, extension dropped). The
> contaminated scenario now reproduces 6.3451 %, bit-identical to the clean one.
>
> ⚠️ It hid because replacing a viewer scene unloads its resources first, so the ordinary path never
> collides. Reproducing it takes loading variant A then variant B **without the first scene being
> torn down** — which is what a manual `Core.openFiles()` followed by an automated run does.
>
> ⚠️ The key is the path **as given**, NOT canonicalised: a caller must be able to predict it without
> loading (projet-alpha's `Fox` skips a reload that way). The price is that two spellings of one file
> are two keys — pass an asset's path consistently. Details:
> `src/Scenes/Loaders/AGENTS.md` § *The resource PREFIX*.

### A bufferView-less glTF accessor reads as ZEROS, silently — the Draco trap (Sep 2026)

> [!CAUTION]
> **Adding support for a compression extension can turn a LOUD failure into a SILENT one.**
> Before `KHR_draco_mesh_compression` was declared on the fastgltf `Parser`, a Draco asset was
> refused outright (`Error::MissingExtensions`, one error line, zero nodes). Declaring it makes the
> file parse — and **every accessor of a Draco primitive carries no `bufferView` at all**.
> glTF § 5.1.1 then mandates that such an accessor "MUST be initialized with zeros; extensions MAY
> override", and `fastgltf::iterateAccessor` implements exactly that (`tools.hpp:746`): it emits
> `count` zero-initialised elements with **no error and no log**.
>
> A read site that is not routed through the decoder therefore yields positions all at the origin,
> normals at zero, indices all zero — a mesh collapsed to a point, with nothing in the log.
>
> **The fix is a hard guard, not care.** `readDracoAwareAttribute()` / `readDracoAwareIndices()` in
> `GLTFLoader.cpp` are the single gate; a bufferView-less accessor that Draco did not claim **fails
> the mesh** with an error naming the attribute. Never soften that into a warning, and never add a
> tenth read site that calls `fastgltf::iterateAccessor` directly.

> [!WARNING]
> **The meshopt design does not transpose to Draco.** `EXT_meshopt_compression` intercepts at the
> **buffer view** level, so a `BufferDataAdapter` serves decoded bytes transparently. Draco
> intercepts at the **primitive** level, and fastgltf only calls the adapter *after* resolving a
> bufferView — which a Draco accessor does not have. Writing a `DracoBufferAdapter` cannot work;
> it will simply never be called.

> [!WARNING]
> **`Attribute::accessorIndex` is NOT an accessor index inside `Primitive::dracoCompression`.**
> fastgltf reuses its generic `Attribute` struct for the extension's map, whose values are Draco
> **attribute unique ids**. Indexing `asset.accessors` with one reads an unrelated accessor — and
> the ids differ from primitive to primitive within the same mesh.

> [!NOTE]
> **Draco is lossy: do not use bit-equality as the pass criterion, and do not panic at a high
> local maximum.** Measured 2026-09-18 against the uncompressed variants at identical framing:
> `Box` usually comes out bit-identical (0 of 2 073 600 pixels) — but not always: the same run
> repeated shows an intermittent single-LSB residual on ~0.44 % of its pixels (max **1**/255),
> so treat one LSB as the CAPTURE's noise floor, never as a codec signal. `Avocado` differs on 10.3 % of pixels
> (mean **0.18/255**) and `CesiumMan` on 3.3 % (mean **0.11/255**) — a sparse scatter along
> silhouettes, background strictly identical. Tiny mean + high local maxima confined to edges is
> the quantisation signature. A **region** or a **block** of difference is not, and means a defect.
> ⚠️ Before believing any such comparison, check the comparator discriminates at all: two
> *different* models must come out far apart (58.5 % of pixels, mean 95.6/255 here). And make sure
> the second asset has actually finished loading — a screenshot taken mid-swap compares a frame to
> itself and reports a perfect match for the wrong reason.

### Critical: Resource getOrCreateResource Lambdas Run on Loading Threads — Capture By VALUE

> [!CRITICAL]
> The loader lambdas passed to `Resources::Container::getOrCreateResource()` may execute
> **asynchronously on the resource manager's loading threads**, after the calling scope has
> returned. Capturing a local buffer by reference is a use-after-free: symptoms range from
> "The manual loading function has return an error !" spam (garbage data failing validation)
> to hard segfaults. **Move buffers into the lambda** (`[pixels = std::move(rgba)]`,
> `[shape]`, `[geometry, materialList, ...]`). Caught while writing `Scenes::Loaders::WADLoader`
> (Jul 2026); GLTFLoader/FBXLoader already follow the rule — keep it that way.

### Fixed: an asset NAME was used as a resource identity — 18 meshes, 1 renderable (Aug 2026)

> [!CRITICAL]
> **The identity of a mesh, a material, a texture or an image inside an asset is its INDEX, never
> its name.** Neither glTF nor FBX imposes uniqueness on names. `GLTFLoader` and `FBXLoader` keyed
> every resource on `{prefix}{Category}/{name}`, so `getOrCreateResource()` handed the **first
> homonym to every later caller** — the second mesh named `Sphere` got the first one's geometry
> **and its material**. Silent: no error path, no warning, and the result is
> indistinguishable from an un-wired material feature.

**Measured on the Khronos conformance assets (2026-08-28).** `ClearCoatTest` ships **eighteen
meshes all named `ClearCoatSampleMesh`** bound to eighteen different materials; all eighteen cells
rendered material 0's red (linear ratio `1 : 0.045 : 0.029` against material 0's declared
`1 : 0.04 : 0.02`), and the `Base layer` / `Coated` / `Coating Only` columns showed no separation
because they were **the same renderable**. Also: `MetalRoughSpheresNoTextures` `Sphere` ×98,
`SpecularTest` `OneSample` ×20, `TransmissionTest` `Sphere` ×12 plus three genuinely different
materials all named `BlueTransWithMask`, `TransmissionRoughnessTest` two different **images** both
named `RoughnessGrid`.

**Fix:** every key goes through `Scenes::Loaders::buildResourceKey()` →
`{prefix}{Category}/{name}-{index}`, the bare `{index}` when the asset declares no name.
`USDLoader` already used that convention; the two others were aligned onto it.

⚠️ **Traps this one carried:**
- **It looks exactly like an un-wired extension.** The failures of `ClearCoatTest`,
  `TransmissionTest` and `SpecularTest` were charged to `KHR_materials_*` for three bench runs.
  Read what the asset DECLARES before charging a defect to a feature.
- **A control only exonerates what it exercises.** `MetalRoughSpheres` cleared "the IBL and the
  exposure" across two runs and could never have caught this: it carries exactly **one** material.
- **An unnamed item was already safe**, because the loaders fell back to the index. That is why
  `AnisotropyStrengthTest` and both iridescence models — whose meshes and materials are unnamed —
  are genuinely un-wired extensions and must NOT be re-attributed to this defect.
- **The colour space belongs in the key, the addressing does not.** `sRGB` comes from the USAGE
  (one image = an sRGB albedo here, a linear roughness map there), so one asset texture yields up
  to two engine resources (`-srgb` / `-data`) and the loaders' texture cache needs **two slots per
  index**. The wrap modes belong to the asset texture itself, so the old `-<U><V>` name suffix
  became redundant once the index was in the key.

**How it was verified**, because a loader-wide change needs a control and not just a nice capture:
a full-frame pixel diff of all 44 bench captures against the previous run, partitioned by whether
the asset carries duplicate names. Models with **unique** names: 32 captures, max delta
**≤ 2 / 255** — the change is a **bit-exact no-op** where it must be. Models with **duplicate**
names: 12 captures, max delta **243 / 255**, up to 15.1 % of pixels. `SpecularTest` has duplicate
names and came out at delta **0**, because its 24 materials declare identical
baseColour/metallic/roughness and differ only in an unread extension — change if and only if
predicted. Runtime consumers re-verified in `animation-debug` (zero VUID): the Fox through the
renamed `glTF:Fox/Mesh/fox1-0`, both Paladins through the FBX path.

Details: [`src/Scenes/Loaders/AGENTS.md`](../src/Scenes/Loaders/AGENTS.md) § *The resource key —
an asset name is not an identity*, and the numbers per model in
[`docs/todo/gltf-conformance-loader-gaps.md`](todo/gltf-conformance-loader-gaps.md).

### An IDENTITY default makes an unwired feature indistinguishable from a disabled one (Aug 2026)

> [!CAUTION]
> `KHR_materials_specular` and `KHR_materials_ior` were implemented **completely and spec-exactly
> on the GPU** — `LightGenerator.PBR.cpp:589-590` computes `dielectricF0 = ((ior-1)/(ior+1))²` and
> `F0 = mix(min(dielectricF0 · specularColor · specularFactor, 1), albedo, metalness)`, the material
> UBO carries the three slots, and `StandardResource` declares them for every material. The glTF
> loader never filled them. Because the defaults are the **identity** (factor 1, white, IOR 1.5 =
> glTF's own default), nothing looked broken: no warning, no black frame, no validation error —
> just a feature that quietly did nothing on every asset, for an unknown number of sessions.

**Why it hid so well.** An identity default is the right engineering choice — it makes an
un-declared extension a no-op — but it also removes every symptom of the wiring being absent. The
only way to see it is to read the two ends and check they meet: a setter that exists and is called
from nowhere is a signal, not a curiosity. `grep -rn setSpecularComponent src/` returned only the
declaration and its own definition, and that was the whole diagnosis.

**How it was confirmed** (2026-08-28, after wiring the factors in `GLTFLoader`): `SpecularTest`'s
four *factor* rows went from flat (axis spread ≤ 0.17 / 255) to monotone (2.36 to 3.35), the sphere
declaring `specularFactor 0` now renders **exactly 0**, and its three *texture* rows — which were
deliberately left unwired — came out **bit-identical**. That last part is the control: a change
that also moved the rows it must not touch would have been a different bug.

⚠️ **The corollary for auditing.** "The extension is enabled on the parser" and "the shader has the
formula" are each worth nothing alone. Check the full path — asset → loader → material UBO → codegen
— before recording a feature as present OR as missing. The bench item had guessed "wiring, not
implementation" for this one; nobody had verified the GPU end, so the estimate could have been off
by a full BRDF.

### A material component's sRGB decoding is decided by its VARIABLE NAME (Aug 2026)

> [!CAUTION]
> `Component::Texture`'s constructor does
> `m_textureResource->enableSRGB(m_variableName.ends_with("Color"))`. The colour space of a
> material texture is therefore chosen by **string suffix**, not by an argument — so renaming a
> `Surface*` key, or picking a new one, silently changes how its texture is decoded.

It lines up correctly for `KHR_materials_specular`, which needs both behaviours from one extension:
`specularColorTexture` is sRGB-encoded per spec and lands on `SurfaceSpecularColor` (decoded);
`specularTexture` carries a linear factor in its **A** channel and lands on `SurfaceSpecularFactor`
(not decoded). That is luck reinforced by convention, not a mechanism anyone declared — treat the
suffix as load-bearing and re-read this rule before renaming either key.

⚠️ Related, and also decided elsewhere than where you would look: `Component::Texture::isOpaque()`
returns `!alphaEnabled()`, but **no material consults it** — `Material::Interface::isOpaque()` only
checks `requiresGrabPass()` and the `BlendingEnabled` flag. So passing `enableAlpha = true` to read
a data channel out of a texture's alpha (which `specularTexture` requires) cannot accidentally turn
the material translucent. `alphaEnabled()` is read nowhere outside the component today, and its
documented promise to "request a 4-channel texture" is **not implemented** — the sampled `.a` is
whatever the loaded image happens to carry (1.0 when it has no alpha, which is the identity here).

### Fixed: the bitangent handedness did not exist, so mirrored UVs lit backwards (Aug 2026)

> [!CRITICAL]
> `ShapeVertex::biNormal()` was a bare `cross(normal, tangent)`, and
> `setTangent(Vector<4>)` **silently dropped its W**. That W is the **bitangent handedness** (±1),
> not a homogeneous coordinate: the bitangent is `cross(normal, tangent) * w`, and the sign is the
> only thing that distinguishes a **mirrored UV island** from a plain one. Every mirrored island in
> every asset therefore lit its normal map backwards. `GLTFLoader` compounded it by never reading
> the `TANGENT` accessor at all and always recomputing.

**Why it is so easy to miss.** Mirroring is invisible on the half of a model that is not mirrored,
and the defect never produces an error, a black frame or a validation warning — it produces a
*plausible* shading that is wrong only where the UVs fold. Mirrored UVs are ubiquitous in
production art (any symmetric character, vehicle or prop), so the blast radius is large and the
symptom is diffuse.

**Measured** on the Khronos `NormalTangentMirrorTest`, highlight angle per column over five
roughnesses: the two mirrored columns had a circular spread of **107.7°** and **102.9°**, deviating
−115.3° and −110.8° from the `Geometry` reference column (spread 0.3°). After reading the accessor
and carrying the handedness: spread **1.8°** and **1.3°**, at −7.8° and −12.1° — the same family as
the `Normal` column (−6.3°) that already passed.

**The controls that make it a proof, not a change:** the `Geometry` and `Normal` columns come out
**identical to the decimal** (they carry no mirrored UVs and must not move); `NormalTangentTest`,
whose asset declares no `TANGENT`, is **bit-identical on all three views**; the handedness defaults
to **+1**, so every other loader and every generated shape is a bit-exact no-op. On the three other
tangent-authoring assets there was no regression — `BoomBox` reads visibly crisper, its normal map
finally matching the frame it was authored against.

⚠️ **Two side effects worth knowing.**
- The read is **all-or-nothing per mesh**: the tangent computation runs over the whole shape, so a
  mesh where only some primitives declare `TANGENT` recomputes every one and logs it. Half authored
  and half computed would disagree at the seam.
- `sizeof(ShapeVertex)` went **80 → 84**, and `FileFormatNative` writes vertices as a **raw blob**
  of that size, so the native format version was bumped to 2 with no version-1 read path (the
  format had no users yet — owner decision). ⚠️⚠️ **A size change to `ShapeVertex` or
  `ShapeTriangle` with an unchanged format version is silent data corruption**: the reader's count
  validation can pass on a wrong stride. The size is pinned by a unit test so the failure is a red
  test rather than a corrupt file.

### `Shape::build()` CLEARS the shape, and its callback is not handed the vertex colours (Aug 2026)

> [!CAUTION]
> `VertexFactory::Shape::build(callback)` begins with `this->clear()`, and the callback receives
> only `(groups, vertices, triangles)` — **not** `m_vertexColors`. So anything written into
> `shape->vertexColors()` **before** `build()` is silently wiped, and the colour vector is left
> EMPTY. The first write into it from inside the callback then indexes an empty vector: **segfault**,
> hit exactly this way while wiring glTF `COLOR_0`.

Size and fill the colours from **inside** the callback, through the captured shape, after the clear
has run. The trap is that the callback's parameters look like the complete set of what a shape holds,
and they are not: colours (and edges) live outside them.

⚠️ Related, same subsystem: `ShapeTriangle`'s vertex-**colour** indexes are a list separate from its
vertex indexes — a face-varying attribute, which OBJ and FBX genuinely need — and they default to
`{0, 0, 0}`. A loader that fills `m_vertexColors` but forgets `setVertexColorIndex()` paints every
triangle with colour 0, which renders as a plausible flat tint rather than as an error. Formats whose
colours share the position indexing (glTF) must still set them, to the same values.

### Fixed: the near plane was a CONSTANT, in four copies, and wrong in both directions (Aug 2026)

> [!CAUTION]
> `nearPlane = nearestObjectDistance / sqrt(1 + tan²(fov/2) · (aspectRatio² + 1))`. The formula is
> right; `nearestObjectDistance` was the **literal `0.1F`**, in **four hand-maintained copies** —
> `ViewMatrices2DUBO`, `ViewMatrices3DUBO`, `ViewMatricesCascadedUBO`, `SpotLight`. The last one
> even carried a comment claiming consistency with the first. Every scene got a near plane sized
> for a decimetre subject, ≈ 0.089 m, with no link to the camera or the content.

**Both directions are broken, and the one nobody looks at is the large end.**

- **Small subjects render NOTHING.** They sit inside the near plane. Measured: the Khronos
  `MetalRoughSpheresNoTextures` has a radius of 0.00035 m — **89 times inside it** — and the glTF
  bench could only get it to 5.5 % of frame height by pushing the camera away; `BoomBox` 14.1 %.
- **Large scenes lose depth precision.** The depth test is conventional
  (`VK_COMPARE_OP_LESS_OR_EQUAL`, `D32_SFLOAT`, **no reversed-Z anywhere**), so a float depth buffer
  spends its precision near the near plane. 0.089 m against a kilometre-scale far throws that
  precision into the first ten centimetres. **Distant z-fighting should be investigated here
  first** — this was never a "small assets" bug, it was a scale-blindness bug.

The derivation is now one function, `ViewMatricesInterface::computeNearPlane()`, and the distance is
a camera property pushed through a defaulted `AbstractVirtualDevice::updateNearestObjectDistance()`
hook. Anything that says nothing keeps 0.1 m and is unchanged.

⚠️⚠️ **THE LESSON IS THAT ONE SCALE FLOOR IS NEVER ALONE.** Fixing the near plane did NOT fix
`+ModelViewer`, which held two more decimetre assumptions: the framing radius floored at `0.01F`
(re-framing every sub-centimetre asset as if it were a centimetre across — 28× too far for the model
above) and the orbit controller's lower distance limit at `max(radius * 0.05F, 0.01F)`, which
`OrbitController::setDistance()` clamps against. A magic 0.01 or 0.1 in a viewer, a controller or a
projection is a scale assumption; grep for the others before declaring a scale bug fixed.

⚠️⚠️ **THE VIEW MATRICES HAVE FIVE OWNERS AND NO COMMON HOLDER** — `Vulkan::SwapChain`,
`Graphics::SceneRenderTarget`, and the templated `RenderTarget::View`, `Texture` and `ShadowMap` each
hold their own `ViewMatrices2DUBO`. A property that belongs to a projection has to be forwarded by
every one of them that carries a perspective. **And the one a scene camera actually connects to is
the SWAP CHAIN**, not `SceneRenderTarget`: implementing the forward on the latter alone left the near
plane at its default while the caller believed it proportional, and the two small conformance models
rendered ENTIRELY EMPTY. Only `ShadowMap` legitimately ignores it — a light's frustum has no subject.

⚠️⚠️ **`Camera::onOutputDeviceConnected()` IS THE FIRST PUSH A TARGET EVER RECEIVES.** Every camera
setter is guarded by `hasOutputConnected()`, and a viewer sets its camera up while building the
scene, i.e. while nothing is connected — so a value that is not also pushed from
`onOutputDeviceConnected()` is stored and never sent. That was the second cause of the same empty
frames, found only after instrumenting the near-plane computation and reading `nearestObject=0.1`
straight out of the log.

⚠️⚠️⚠️ **AND THE METHOD MISTAKE THAT LET BOTH THROUGH: a safety net was loosened in the same change
that needed it.** The glTF bench's `clamp_distance()` existed precisely to turn "this model would
render nothing" into a reported number — and it was made subject-relative in the SAME commit as the
engine change it was supposed to police. So it had nothing left to catch, and the failure arrived as
two silently empty captures instead of a warning. **Keep a guard absolute until the change it guards
is measured**, then relax it.

### A UNIT that differs silently: glTF anisotropy rotation is radians, the engine's is turns (Aug 2026)

> [!CAUTION]
> `KHR_materials_anisotropy`'s `anisotropyRotation` is in **RADIANS**. The engine's is in **TURNS** —
> the shader computes `rotation * 2.0 * PI` — and `StandardResource::setAnisotropyRotation()`
> **clamps to [0, 1]**. Passing the glTF value straight through is therefore wrong by 2π *and*
> flattened to 1.0 for anything above one radian. Both failures are silent and produce a rotation of
> a plausible magnitude in the wrong direction, which is the hardest kind of defect to see.

The same shape appeared twice more the same week and is worth generalising: **a scalar crossing a
format boundary needs its unit checked, not its type.** `KHR_materials_iridescence`'s film
thicknesses happen to be nanometres on both sides — so they must NOT be converted, and "helpfully"
scaling them would break what currently works. The only way to know which case you are in is to read
both ends.

⚠️ **A spec default that was a placeholder.** While wiring iridescence, the film thickness in
`LightGenerator.PBR.cpp` read `mix(min, max, 0.5)` — the midpoint. The extension says the thickness
comes from the thickness texture's G channel and that **without that texture it is the MAXIMUM**. A
midpoint is a different interference colour at every angle and can never match a reference.

### ⚠️ Four metrics in a row can be confounded — say so instead of shipping the fourth (Aug 2026)

Wiring anisotropy and iridescence changed 6.6 % to 9.6 % of the pixels of the three models that
declare them, with clean controls at delta 0. What could NOT be produced was a numeric conformance
criterion, and each attempt failed differently:

- a **principal-axis aspect ratio** of the bright set cannot measure elongation when the highlight is
  a curved arc — a wrapped arc reads as compact;
- a **percentile threshold** (top 5 %) cannot survive the brightness change anisotropy itself causes:
  on a dark sphere the top 5 % is a large dim region, on a bright one a tight highlight;
- a **saturation average** over a crop that includes lawn measures the lawn.

⚠️⚠️ The 2026-08-27 anisotropy figures recorded in the bench item came from the first of those and are
**not trustworthy either** — a number in a document is not a measurement unless its metric was
validated against a case where the answer is known. When four attempts are confounded, the honest
deliverable is "the feature demonstrably acts, by this diff and this control; the criterion is still
missing", not the fourth number.

### ⚠️⚠️ Inside `load()`, a dependency is NOT loaded — never ask one about its content (Sept 2026)

`ResourceTrait::load()` implementations register dependencies with `addDependency()` and end on
`setLoadSuccess(true)`. At that point the dependencies are typically still loading: resource
creation goes through the thread pool, and `ResourceGenerator::shape()` says so in its own comment.
So anything `load()` reads *from* a dependency is a race.

It cost a whole row of a bench to learn. `MultiLayerMeshResource::load(geometryLODs, …)` validated
that every level of detail exposed the same number of sub-geometries, by calling
`subGeometryCount()` on each geometry it had just been handed. For a shape carrying two groups the
answer came back **1**, the load was refused, and four perfectly valid trees never appeared —
while the four built later in the same frame, having had more time, loaded fine. A race that
depends on how busy the pool is looks exactly like a random defect.

**The check belongs in `onDependenciesLoaded()`**, which the contract guarantees is called with
every dependency in the `Loaded` state. What `load()` may legitimately check is what it holds
itself: null pointers, list sizes, ceilings.

⚠️ And when overriding `onDependenciesLoaded()`, **chain to the implementation you override** —
`MultiLayerMeshResource`'s already did real work (sub-geometry / layer coherence, LOD generation),
so a fresh override that forgot to call it would have silently disabled all of it.

⚠️ Related: check whether the hook is ALREADY overridden before adding one. A truncated
`grep | head` said it was not, and the duplicate only surfaced as a compile error. The grep that
answers the question is the one without `head`.


## Animation

### Fixed: `play()` keys on the CLIP name, the loaders hand out RESOURCE names — a silent, total no-op (Aug 2026)

**Symptom.** The `asset-loader` demo's KeyPad2 cycled its animation index, printed the clip name it
believed it was starting, and **nothing moved**. No error, no warning, no log line.

**Cause.** `SkeletalAnimator::addClip()` indexes on the clip's own name:

```cpp
m_clips[clip->clip().name()] = clip;          // "walk_2"
```

while the caller collected `clip->name()` — the **resource** name, which every loader prefixes:

| Loader | Resource key | Clip's own name |
|---|---|---|
| FBX (sibling clips) | `FBX:walk_2/Animation/walk_2` | `walk_2` |
| FBX (embedded stack) | `<prefix>Animation/<stack>` | `<stack>` |
| glTF | `<prefix>/animation/<name>` | `<name>` |

`play()` looked up a key that never existed and **returned `false`** — which the caller discarded.

**The rule.** `play()` takes `clip->clip().name()`, never `clip->name()`, and **its bool is never
discarded**. ⚠️ The two names are equal often enough (a clip whose loader adds no prefix) that a
spot-check on one asset proves nothing.

### Fixed: `stop()` cleared the pose instead of restoring it — the model froze mid-animation (Aug 2026)

`SkeletalAnimator::stop()` did `m_skinningMatrices.clear()`, so `hasPose()` went false,
`Component::Visual` stopped calling `updateSkinningMatrices()` — and the `RenderableInstance`'s
**staging buffer still held the last animated pose**, which `flushSkinningMatrices()` kept uploading
every frame. "No animation" rendered as "frozen on the last frame".

⚠️ **A stop that stops WRITING is not a stop.** Wherever a consumer pushes state only while a
producer reports having some, stopping the producer freezes the last value. `stop()` now *evaluates*
the bind pose; `Scenes::Component::NodeAnimation::stop()` writes back each target's captured rest
frame. (The skinning SSBO is separately initialised to identity at creation, so an asset that never
animated at all was correct — the defect only bit assets that had animated at least once.)

### The animator does not exist yet when the scene is built — say it on the CONTENT (Aug 2026)

`Component::Visual` creates its `SkeletalAnimator` **lazily**, on its first logic cycle, and
auto-plays clip 0. A consumer building a scene has nothing to call `play()` or `stop()` on, so:

- `SkeletalDataTrait::enableAutoPlayFirstClip(false)` → the asset appears in its bind pose;
- `SkeletalDataTrait::setAutoPlayClipName(name)` → the asset appears already playing that clip.

⚠️ Both mutate a **cached resource** — the setting outlives the instance that asked for it.

⚠️ A corollary for diagnostics: right after a load, `visual->skeletalAnimator()` is legitimately
`nullptr`. Code warning on "nothing answered" must distinguish *nothing was asked* (no animator yet,
correct) from *everything refused* (a real miss) — `AssetLoader::applyAnimation()` counts both.

### A glTF animation is not necessarily skeletal — and may be BOTH (Aug 2026)

glTF drives skin joints and plain nodes through the same construct. Measured on the bench:
`ChronographWatch.glb` and `IridescentDishWithOlives.glb` each declare **1 animation and 0 skins**;
`Dragon.glb` declares **27 animations, 1 skin**. The loader used to `continue` on any channel whose
target node was not a skin joint, so a 0-skin asset produced **zero clips** and read as "carries no
animation".

Channels are now sorted into `SceneData::animationClips` (joints) and
`SceneData::nodeAnimationClips` (nodes), evaluated by `SkeletalAnimator` and
`Scenes::Component::NodeAnimation` respectively. An animation touching both comes out **split into
two clips sharing one name**, in two resource key spaces. See
[`src/Animations/AGENTS.md`](../src/Animations/AGENTS.md).

⚠️ **An animated node must be exempted from hierarchy flattening.** `SceneDataConsumer` drops a node
with no mesh and an identity transform — exactly the shape of a pivot waiting to be rotated. Kept
flattened, the clip drives the PARENT and swings the whole asset.

### An anonymous clip's fallback name must reach the CLIP, not just its resource key (Aug 2026)

`GLTFLoader` built `"clip_N"` for an unnamed animation but constructed the `AnimationClip` with the
raw (empty) name. Two anonymous animations therefore collapsed onto the empty key inside every
animator's clip map, and only one stayed reachable. `CesiumMan.glb` is the asset that exhibits it.

### Pointer priority — the overlay is served FIRST (fixed 2026-09-18)

`Input::Manager::addPointerListener()` inserts every listener **at the front**, so the newest one is
dispatched first. That is deliberate between scenes, and it was silently wrong for the UI: the
`Overlay::Manager` registers once at engine start-up, so **every scene created afterwards outranked
the interface drawn on top of it**.

> [!CAUTION]
> **Symptom: the application menu is perfectly drawn and completely dead.** Not frozen, not hidden —
> rendered, hover states and all, while every click falls through to the scene. Measured: clicking a
> menu entry changed **0.0000 %** of the pixels.
>
> It needs a scene whose `OrbitController` has a node to drive. `OrbitController::onButtonPress()`
> returns `false` when `m_controlledNode == nullptr` and `true` otherwise — so a **demo** scene, whose
> controller drives nothing, declines and the menu works, while the **model viewer**, which gives it
> the subject to orbit, consumes the click. That asymmetry is what hid the defect for so long: it
> only appeared after opening a viewer, which until 2026-09-18 meant dragging a file onto the window
> — never twice in a row.

`Overlay::Manager` now registers with `addPointerListener(this, true)`. The priority listener is
kept at **index 0 of the same vector**, so the five dispatch loops are untouched and none of them can
be forgotten; everything else keeps its newest-first order.

> [!NOTE]
> **No regression on the scene controls — MEASURED, 2026-09-18.** `Overlay::Manager::onButtonPress()`
> returns `false` for any screen that is empty, not visible or not listening, so with the menu closed
> the event reaches the orbit controller exactly as before. A held drag across the viewport of a
> model viewer moves **99.82 %** of the pixels after the change.
>
> ⚠️ That measurement only became possible with `mousePress`/`mouseRelease`, added the same day:
> `mouseClick` presses **and releases**, so every `mouseMove` sent afterwards arrives with the button
> already up and a drag-driven control sees nothing — the same drag through `mouseClick` reports
> **0.0000 %**. This note first shipped saying "verified by reading the code rather than by
> exercising it", which was true for about an hour. **A control that can only be driven by a held
> button is untestable until the console can hold one.**

> [!WARNING]
> `Scenes::Editor::Manager` also registers a pointer listener and had the same exposure. It is fixed
> by the same change, since the overlay now outranks every scene-side listener rather than just that
> one.

⚠️ `addPointerListener()` guarded against duplicates with `std::ranges::binary_search` over a vector
that is **never sorted** — front-insertion guarantees it is not. The guard could miss a real
duplicate or invent one. Now `std::ranges::find`.

## Scene Rendering

### Fixed: a post-process occupant filed AFTER the stack's creation was never created (Sep 2026)

**Symptom:** the scene-driven cloud pass was listed by `getStatus()` as `Clouds: VolumetricCloudsEffect`
— selected, effective — and drew nothing, with no error and no VUID, for three launches.

**Cause:** `PostProcessStack::addEffect()` files an effect ENABLED (every effect is enabled at
construction) and creates nothing; `createAll()` creates the selected occupants, but only for a stack
built before the scene starts. An occupant filed later (`syncSceneEffects()`, or any runtime
`addEffect()`) reached `syncSlotSelection()` as "selected == enabled" and took its "nothing changes"
early-out every frame — the materialization below it never ran — while the executor's `isCreated()`
gate skipped it in silence (a gate that is silent ON PURPOSE, to avoid per-frame spam).

**Fix:** the early-out also requires `isCreated()`; an enabled but un-created occupant is now
materialized the next frame, then enabled.

**Rule:** "listed as running" is not "running". A pass that can legitimately draw nothing must say
WHY it drew nothing (the cloud pass traces a census when it changes); the absence of that line is
itself the diagnosis (`execute()` never ran).


### Fixed: RushMaker paced on the last capture, not on the slot grid — 1 frame in 5 duplicated at 48 FPS (Sep 2026)

> **Symptom (owner):** hiccups in every RushMaker video of `forest`, though the demo ran at ~48 FPS
> with or without the recording (measured: recording costs no frame rate).

**Cause.** `shouldCaptureFrame()` waited one frame duration after the LAST capture, and the PTS was the
wall clock × FPS. The capture lagged the slot grid by up to one render interval each time: at 48 FPS a
capture every ~41.6 ms against 33.3 ms slots, so every fourth slot had no frame and received a CFR
filler (132 out of 626 — `Hardware encoding session finalized: … (132 CFR filler frames …)`). **Fix:** the
gate captures the first frame of every slot not served yet (`Recorder::cfrSlotAt()`, shared with the
PTS): 0 fillers at the same load. Details: `src/Graphics/AGENTS.md` § RushMaker pipeline.

⚠️ A filler is still legitimate when the renderer is below the target FPS (owner's CFR model, by design).
Read the finalisation counter, not a hash of decoded frames: the hardware rate control re-encodes a
duplicate slightly differently, so identical-frame hashing found only 22 of the 132.

### Fixed: `MaxLODLevels` means TWO things, and the unqualified name picked the wrong one (Sep 2026)

`Graphics::Geometry::MaxLODLevels` = 8 (what a mesh can HOLD) and `Graphics::Renderable::MaxLODLevels`
= 4 (the ladder the VIEW selects from). Inside `namespace EmEn::Graphics::Renderable` the unqualified name
resolves to the second one — even in a file with `using namespace Graphics::Geometry`. So
`MultiLayerMeshResource` stored at most 4 levels and `MeshResource` (storage sized 8) refused its 5th:
a tree grown with 6 levels failed to load with *"was given 6 levels of detail, the ceiling is 4"* and the
forest was silently empty (0 VUID — only the log says it). Both now qualify: storage and validation by
`Geometry::MaxLODLevels`, the automatic LOD generation (for the view) by `Renderable::MaxLODLevels`.
⚠️ Always qualify `MaxLODLevels`.

### Fixed: a multiview CSM drew every caster into every cascade — and culling by cascade volume does not fix it (Sep 2026)

> **Symptom:** switching `terrain` from its classic 4096 px map to 4 × 4096 px cascades took the shadow
> pass from 15.1 to 57.8 ms of GPU (validation ON, RTX 3070 Ti) for the same 74 M caster triangles.

**Cause.** One multiview pass for all the cascades: multiview broadcasts every draw to every view.

**What did NOT work, measured.** One pass per cascade with casters culled to the cascade's light
volume: 91.6 ms. The cascades are sphere-fitted, so their volumes overlap and each kept 3.9 cascades'
worth of casters, and four passes cost ~1.6× one multiview pass at equal work.

**Fix.** One pass per cascade, casters kept only if their shadow can land on the cascade's RECEIVERS
(the camera-frustum slice between the cascade's splits, cross-fade band included), a terrain culled to
the cascade's caster volume: **34.7 ms**, 7 024 instances instead of 32 882. Details, the bounded-sweep
trade-off and the profiler lines: `docs/shadow-mapping.md` § *One pass per cascade*.

⚠️ **Traps.** Never cull shadow casters with the light's full frustum: the cast pass clamps depth, the
near plane must go (`Frustum::shadowCasterVolume()`). The cascades are geometry-bound — a lower
resolution saves 15 %, fewer caster triangles is the lever.

### Fixed: two state slots let the logic LAP the renderer — heavy content slid on the image while the camera turned (Sep 2026)

> **Symptom (owner, 2026-09-24):** on heavy scenes only, some objects seem to *slide* against the rest
> while the camera turns — the `terrain` forest, Sponza's central tree. No VUID, and a still camera
> looks perfect.

**Cause.** The logic → render hand-off had TWO slots. The logic publishes at a fixed 60 Hz and
alternated between them; the render thread latched one slot per frame and read it until the end of
the frame — each draw re-reads the view matrix and holds a POINTER to its node's published frame, so
the reads are spread over the whole recording. The logic's second publication inside one frame
therefore landed on the latched slot: every frame whose CPU recording outlasted a tick drew its early
batches with the camera of tick N and its late ones with the camera of tick N+2 (plus torn matrices:
the copy was a plain data race). The code documented the opposite — "stable for the whole frame, the
logic thread publishes to the other index".

**Measured** with the permanent instrument `Core.SceneManagerService.getStateSyncStatistics(true)`
(write generation per slot, bumped before the logic writes it, compared at the latch and at the
frame's end): **41-66 % of the frames overwritten on `terrain` (latch-to-end 23-29 ms), 11-16 % on
`sponza` (19 ms)**. After the fix: **0 %** on both, at the same load (1.6 publications per frame on `terrain`, 1.2 on `sponza`). Owner-validated on screen the same day, on both scenes: no object slides
on the others any more while the camera turns or moves under heavy load.

**Fix.** `RenderStateSlotCount` = 3 and a lock-free triple buffer: the logic writes its own slot and
EXCHANGES it with the middle one; `beginRenderFrame()` takes a fresh publication by exchanging its
slot with the middle one. See `src/Scenes/AGENTS.md` → Frame Synchronization.

⚠️ **Traps.**
- A frame rate above 60 FPS hides it completely; the symptom needs a CPU recording longer than one
  tick, which is why only the heaviest scenes showed it.
- "Camera in motion" is still a CLASS of causes: this one is a pure timing defect and is measured as
  such — the counter needs no camera motion at all. It also explained the owner's older report of
  shading "popping in and out" on Sponza while the camera moved (2026-09-13, "only on overloaded
  scenes"): gone with the fix, owner-confirmed, item deleted. Two RT-lane mechanisms remain
  camera-dependent BY CONSTRUCTION and are the next suspects if popping ever returns: the
  `IrradianceProbeVolume` is anchored on the camera's 1.5 m grid cell and RESETS one plane of probes
  at every cell crossing (re-converging over ~1 s at hysteresis 0.97, `m_resetPlanes`), and the RT
  denoisers (RTGI/RTAO/RTR) show the raw estimate on disoccluded pixels for a few frames.
- A console teleport produces ONE frame of velocity, and a screenshot takes ~1.5 s: a capture series
  after a move measures convergence, never a sub-second event. Measured on Sponza: from +3.3 s on,
  both lanes matched their settled frame within the noise floor.
- Never take the published index with a plain load again, and never size a state array with a
  literal `2`: use `RenderStateSlotCount`.

### A duplicate ENTITY NAME cost the whole node — `emplace` discarded it and the caller never knew

> **Symptom:** a loaded model renders with objects missing. No VUID, no warning, no failed resource
> — the geometry is simply absent, and every remaining object looks right.
>
> **Root cause:** `Scene::createStaticEntity()` did `m_staticEntities.emplace(name, entity)` and
> returned the entity **unconditionally**. `emplace` on an existing key is a no-op that DISCARDS its
> argument, so the caller received a live pointer to an entity that is not in the scene, attached
> its components to it, and the node never rendered. The scene's registry is keyed by name, and
> **neither glTF nor FBX makes node names unique** — so a collision is the normal case, not an edge
> one.
>
> **Measured (2026-09-14, owner-reported):** the Khronos `TransmissionTest` carries 22 mesh-bearing
> nodes under only **16 distinct names** — three of them are called `BlueTransWithMask`. The scene
> built **18** static entities (16 + the viewer's own two) and **two of the three blue spheres were
> missing from the grid**, along with one of the four column labels. After the fix: **24** entities,
> the 3×4 grid complete. The arithmetic is the diagnosis — count the entities against the asset's
> mesh-bearing nodes before looking at anything else.
>
> **Fixed in two halves:**
> 1. `createStaticEntity()` reads the `emplace` result, traces an error and **returns nullptr** on a
>    collision. It can no longer hand back an orphan.
> 2. `SceneDataConsumer` renames only the DUPLICATES (`<name>-<nodeIndex>`, the remedy
>    `buildResourceKey()` uses one level down). The first holder of a name keeps it verbatim, so
>    every entity that already resolved by name still does.
>
> ⚠️ **Same family as two defects already in this file** — the resource key built on an asset NAME,
> and `Material::Interface`'s component map where the second `emplace()` is rejected in silence.
> When something is missing and nothing is logged, look for a map keyed on a name the format does
> not make unique.
>
> ⚠️ **The node-hierarchy import path is NOT covered by this fix.** `SceneDataConsumer` also builds
> children with `createChild(nodeDesc.name)` (the `+ModelViewer` file path, `AssetRoot`); whether
> that registry tolerates a duplicate is unverified. Measure it the same way — entity count against
> the asset's node count — before assuming either answer.

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
the TAA sections of this document for the state of the art applied to the resolve (measurement-neutral at the
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
the program cache key. (The historical root `TODO.md` entry "Infinity-view renderables wrote a
garbage velocity" was its origin; that file is gone — open work now lives in `docs/todo/`.)

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

### Fixed: the NodeCrawler stale-local trap — it compiled with ZERO warnings (Aug 2026)

`Scenes::NodeCrawler< node_t >` lost its returning accessor: the API is now
`bool fetchNextNode()` + `const std::shared_ptr< node_t > & currentNode()`, where it used to be
`std::shared_ptr< node_t > nextNode()`. Every one of the **12 call sites**
(`Scene.cpp` ×2, `Scene.entities.cpp` ×5, `Scene.rendering.cpp` ×5) kept the local it used to
assign from the old form:

```cpp
// BROKEN — compiles, zero warnings, wrong on EVERY iteration
const auto currentNode = m_rootNode;      // leftover of: while ( (currentNode = crawler.nextNode()) != nullptr )
while ( crawler.fetchNextNode() )
{
    currentNode->doSomething();           // ALWAYS THE ROOT NODE
}
```

The loops ran the correct NUMBER of times and operated on the root node every time:
`processLogics()` ran N times on the root, `findNode()` could only match the root, the statistics
counted the root's children N times, camera/microphone detection inspected the root only, and the
five rendering crawlers would have rendered nothing node-attached.

**Two rules that follow:**
1. **Never cache the crawler's node in a local.** Call `currentNode()` in the loop body.
2. **The iteration NEVER yields the base node** — `while ( crawler.fetchNextNode() ) { ... }` walks
   DESCENDANTS ONLY. `currentNode()` is the base node before the first fetch and `nullptr` after
   the last, so a caller needing the base node processes it BEFORE the loop (the two converted
   `do/while` sites in `Scene.entities.cpp` are the reference pattern).

**Validated at runtime on the `animation-debug` demo (Aug 2026):** node-attached visuals render and
animate correctly. That witness is decisive, not merely reassuring — with the stale local the
crawlers only ever saw the ROOT node, so node-attached meshes rendered **not at all**. Seeing them
render *and* animate therefore proves the five rendering crawlers (`Scene.rendering.cpp`) **and** the
`processLogics()` crawler are repaired.

⚠️ `doom-loader` remains a **weak target** for this and must not be used as the witness: the level is
a `StaticEntity` (the `m_staticEntities` path does not use the crawler at all) and its only `Node` is
the player, which carries no visible mesh. Any future regression check needs a demo with
node-attached visuals. Full contract: [`src/Scenes/AGENTS.md`](../src/Scenes/AGENTS.md)
§ "Node Tree Iteration — NodeCrawler Contract".

### Fixed: the physics broad phase re-hashed every inherited pair once per leaf — 23 ms logic tick on 121 nodes (Sep 2026)

**Symptom:** `logicsTask : 22.8 ms` printed on virtually every cycle of projet-alpha `game-logic`
(121 nodes, 138 static entities). The owner attributed it to the Aug 26 two-state synchronisation
commits. `perf` said otherwise: the logic thread was CPU-saturated, **99 % in
`Scene::resolveCollisions()` phase 2**, `unordered_set< uint64_t >::insert` alone ≈ 57 %;
`publishStateForRendering()`, the node logic and the actors were under 1 % together.

**Cause:** since the one-sector storage invariant (`9a95a50f`, Aug 2026), `forLeafSectors()` handed
each LEAF the full ancestor chain, and phase 2 paired ALL candidates in EVERY leaf, relying on a
pair hash set to deduplicate. An inherited × inherited pair (anything straddling a split plane:
player, walls, crate at the origin) was re-hashed once per leaf below it — 7 260 unique pairs at
most, an estimated ~200 000 hash inserts per tick.

**Fix:** `OctreeSector::forEachSector()` visits every sector that OWNS elements (inner nodes too)
and the callers apply the pairing contract — `owned × owned`, `owned × inherited`, nothing else —
which produces each geometrically possible pair exactly once. The hash set, `createEntityPairKey()`
and the never-called `detectCollisionInSector()` are gone. Phase 1 handles each movable once at its
owning sector and reaches the subtree statics with `forTouchedSector(aabb)`, closing a hole where a
body straddling two leaves ignored the statics of the second one. **After:** ≈ 1.3 ms of CPU per
tick (12× less), zero warnings, zero VUID, base suite 2045/2045.

⚠️ **Traps for the next profiler:** on a hybrid CPU (P-cores + E-cores) `perf report` percentages
are weighted by *cycles* across two PMUs and misled here (they showed the render thread at 40 % and
the saturated logic thread at 7 %) — count **samples per thread** (`perf script --tid` folded), a
thread at 999 Hz × 15 s = 15 000 samples is saturated. `perf report --children` on a DWARF capture
of a 170 MB binary takes >10 min; `perf script --tid <tid> -F ip,sym,dso` folded with a 20-line
script is the fast path. `perf_event_paranoid` is 3 on this workstation by default (the owner
lowers it to 1 on request).

### Fixed: `[Warning][OctreeSector] Element 'ACTOR_…' is not part of the octree !` on every explosion (Sep 2026)

**Symptom:** the warning on every runtime-created actor that died (explosions, fires) on
`game-logic` — 60 to 90 per run. Visually everything was correct: the actors appeared, lived and
left the scene. The message names the actor's BASE NODE (it carries the actor's `ACTOR_…` name), not
the actor itself: the warning never came from the `Act` actor octree, whose add/remove traces all
found their element (measured by instrumentation before touching anything).

**Cause:** `Scene::onNotification(SubNodeDeleting)` and `Scene::removeStaticEntity()` erased the
dying entity from the PHYSICS octree unconditionally, while `checkEntityLocationInOctrees()` only
inserts an entity there when it is collidable WITH a collision model and a valid AABB. An explosion
or a fire has no collision model — never inserted, hence "not part of the octree" at removal.
`OctreeSector::erase()` warns by default when the root finds nothing.

**Fix:** both removal paths erase from BOTH octrees unconditionally (a membership predicate can have
changed since insertion, and a stale membership keeps the entity alive through the octree's
`shared_ptr`) and pass `warnWhenMissing = false`: absence is legitimate there. The warning stays
where the caller asserts membership (`Act::removeActor()`).

⚠️ **Method trap:** two octrees print the same `[OctreeSector]` tag and the element names collide
(actor ↔ its base node). Before reasoning about a "double removal", instrument the SUSPECTED path
and check the warned id appears in its traces — here it never did, which pointed at the other octree
in one run.

### Fixed: a removed light destroyed its hardware on the logic thread while the render thread iterated the set (Sep 2026)

**Symptom (of the fix in progress):** `pure virtual method called` on the render thread,
`renderLightedSelection()` → `PointLight::touch()` → `Component::Abstract::getWorldCoordinates()`,
the moment a barrel exploded on `game-logic`.

**Cause:** `renderLightedSelection()` locked the light-set mutex **per render batch**, around the
ambient and every light pass — i.e. while recording command buffers — and the logic thread took
the same mutex every tick in `updateCSMCascades()` (one futex block per tick, measured). Replacing
the per-batch lock by a once-per-call SNAPSHOT exposed what the lock had been hiding by accident:
`Node::destroyTree()` → `LightSet::remove()` → `destroyFromHardware()` ran synchronously on the
LOGIC thread, so a frame holding the snapshot dereferenced a light whose node was destroyed, and
whose shadow descriptor set and shared-UBO element were freed while frames in flight still read
them — that second half predates the snapshot and was never protected by the lock.

**Fix:** (1) `AbstractLightEmitter::touch(sphere, readStateIndex)` reads position and radius from
the published block of the latched slot — the render thread never reaches a light's parent entity.
(2) `LightSet::remove()` RETIRES the light (erased from the sets, stamped with the render frame
counter); `destroyRetiredLights()` runs from `Scene::beginRenderFrame()` behind the in-flight fence
and destroys, on the render thread, the lights retired more than `framesInFlight()` frames ago.
(3) `updateCSMCascades()` copies the CSM lights under the mutex and refits outside it.
**Rules:** never hold a shared mutex while recording; a snapshot keeps a COMPONENT alive, not its
entity; GPU resources die behind the fence, on the render thread. Full contract:
[`src/Scenes/AGENTS.md`](../src/Scenes/AGENTS.md) § "LightSet & Background-Derived Lighting".

### A lazily-created resource MATERIALIZES ITSELF at the first resize (Sep 2026)

> [!CAUTION]
> **Symptom:** the post-process stack keeps both lighting lanes resident but creates only the
> selected one. It works — until the window is resized, after which every alternative reports
> `created` and the memory saving is gone. Nothing in the resize path mentions laziness.
>
> **Cause:** `PostProcessStack::resizeAll()` iterated every resident effect and did
> `effect->setCreatedFlag(effect->resize(w, h))`. `resize()` **destroys then creates**, so calling
> it on a never-created effect *is* a creation, and the line right after raises its flag. The
> laziness evaporated on a gesture that has nothing to do with it.
>
> **Rule:** any per-object lifecycle sweep — resize, reload, revalidate — must skip what was never
> created, and the "created" flag must be the one thing that decides. Fixed by an
> `if ( !effect->isCreated() ) { continue; }` guard, with the symmetrical fixes in `createAll()`
> (creates only what is selected) and `destroyAll()` (destroys only what exists — `destroy()` on
> an effect that never ran `create()` releases members it never assigned).

### An automatic fallback on a WARM-UP condition defeats the laziness it was paired with (Sep 2026)

> [!CAUTION]
> **Symptom:** the ray-traced lane runs, exactly as selected — and yet all three screen-space
> effects report `created`. Measured on Sponza the day the lazy residency landed.
>
> **Cause:** `Renderer::isRayTracingReady()` is false for the first frames of **every** scene (the
> TLAS is built asynchronously) and it **cannot distinguish "not yet" from "never"** — it is a
> null/created test on the current TLAS plus a non-empty descriptor-set check, nothing more. A
> fallback that fires on the first stalled frame therefore materialized the whole alternative lane
> on every single launch, seconds before the traced one took over.
>
> **Rule:** a fallback keyed on a condition that is *transiently* false at startup needs a grace
> period, not a debounce — `PostProcessStack::FallbackGraceFrames` (120, ~2 s) — chosen long enough
> that a warm-up never trips it and short enough that a scene genuinely without ray-traced geometry
> still recovers. And ⚠️ **verify it by reading the state, not the picture**: the frame looked
> perfect in both cases, because the correct lane *was* the one running. Only `listEffects()`
> showed the wasted allocation.

### A "nothing changed" early-out that also skips the REPORTING makes a correct system look broken (Sep 2026)

> [!CAUTION]
> **Symptom:** `listEffects()` marked TAA as selected but not running, on a frame that was visibly
> anti-aliased; `getStatus()` printed `ToneMapping: off` on a visibly tone-mapped frame.
>
> **Cause, twice over.** (1) `syncSlotSelection()` recorded the effective occupant *after* its
> `if ( effective == enabledEffect(slot) ) continue;` early-out, so a slot whose selection was
> already correct on the very first frame never reached the store and reported "nothing here" for
> the whole session. (2) The four camera-owned slots carry no selection at all — the camera
> materializes them — and reading them from the selection indices reported them off.
>
> **Rule:** state an observation on **every** path, including the one where nothing changed; and
> when a subsystem has two ownership regimes, the reporting needs two readers, not one. A report
> that contradicts the image is worse than no report — it is the thing a future session will trust
> over its own eyes.

### A setting that another setting DOMINATES should not exist — and until it goes, default to "Auto" and speak (Sep 2026)

> [!CAUTION]
> **Symptom:** the settings file reads
> `Core/Graphics/PostProcessing/LightingLane: "RayTracing"` and the frame is rendered in screen
> space. Nothing in the log, nothing on screen. Owner report, 2026-09-10, on the first settings
> reset after the post-processing tree was reorganised.
>
> **Cause:** `Core/Graphics/RayTracing/Enabled` — a *different* key, in a *different* section
> (since **deleted**, see the epilogue) — defaulted to `false` and dominated the lane choice: with
> it off the ray-traced lane was never made resident, so `installLightingFamily()` started on
> screen space. A settings reset took it back to `false` while the post-processing key kept saying
> `RayTracing`, so **a freshly written file was self-contradictory out of the box**.
>
> **Two rules, both learned here:**
> 1. **A key that another key can override must not default to a concrete value.** Default it to
>    `"Auto"` — resolve against what the machine actually offers — so a generated file can never
>    contradict itself on any machine. A fixed default is a claim about a state you do not control.
> 2. **An EXPLICIT request that cannot be honoured must be traced**, naming the dominating key and
>    its current value. And only an explicit one: `"Auto"` is a policy, not a request, and warning
>    on the default trains the reader to ignore the warning.
>
> ⚠️ Naming the key well is not a substitute for either. This key was *deliberately* named
> `LightingLane` rather than `EnableRayTracing` precisely to avoid being confused with the device
> switch — the confusion happened anyway, on the very next reset, because the failure was silent.
> A good name helps someone who is already looking; a trace is what makes them look.
>
> **Epilogue, same day: the dominating key was DELETED** (owner decision). Two settings that
> express the same intent, one silently overruling the other, is a defect of the *schema*, and the
> two rules above are only the mitigation. What made deletion possible was checking what the key
> actually gated:
> - ⚠️⚠️ **The documentation of it — written the same morning, by the same hands — was wrong.** It
>   claimed the key gated the `SHADER_DEVICE_ADDRESS | ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY`
>   usage flags on every VBO/IBO, `createRTDescriptorSet()` and the descriptor-pool AS entry. All
>   three are gated on `Device::rayTracingEnabled()` — **pure hardware**, since the RT extensions
>   are requested from `physicalDevice->supportsRayTracing()` without consulting any setting
>   (`Instance.cpp`). That false claim was then used to argue against changing the key's default.
>   **Grep the consumers of the accessor before describing what a setting controls**; a settings
>   key and a capability query that AND together at every call site look identical from the call
>   site and are not.
> - Its one real job — creating the `AccelerationStructureBuilder` — moved onto the key that was
>   already expressing the intent. ⚠️ That makes the lane a **launch** decision: a BLAS is built
>   when a geometry loads and cannot be added later, so a runtime switch to the traced lane is
>   refused rather than silently rendering nothing.

## Shader/GLSL Pitfalls

### Read the GENERATED GLSL — a variable computed and never used is two subsystems never connected (Aug 2026)

The engine writes its shaders. When a rendering feature looks half-present, the fastest instrument
is the generated source, not the C++ that emits it:

```jsonc
// ~/.config/LNIsle/projet-alpha/settings.json
"Core/Graphics/Shader/EnableSourceCodeDump": true,
"Core/Graphics/Shader/EnableBinaryCache":    false,   // ⚠️ MANDATORY
"Core/Graphics/Shader/EnablePipelineCache":  false    // ⚠️ MANDATORY
```

⚠️ **Both caches must be off.** A cache hit skips generation entirely, so the dump directory stays
empty or stale and you conclude the feature was never generated. The GLSL lands in
`~/.cache/LNIsle/projet-alpha/generated-shaders/<domain>/`. Restore the three keys afterwards.

**What it caught.** Transmissive glass rendered milky white. The tier was the obvious suspect —
grab pass or cubemap fallback? The dump answered in one grep: **28** scene-rendering shaders
contained `gpRefractedUV`, so the high-quality grab pass was running everywhere. But
`SurfaceTransmissionColor` was *used* in only **2** of them — the ambient passes. The other 26
sampled the grab pass, assigned the result to a `const`, and never read it again, adding an
albedo-based term instead. A dangling computation of that size is not a micro-optimisation
oversight: it is the signature of **two subsystems written against each other and never wired
together** — here the grab-pass generation in `Graphics/Material/StandardResource.cpp` and the
lighting composition in `Saphir/LightGenerator.PBR.cpp`. Grep the dump for a declared-but-unread
variable whenever a feature is "implemented" but invisible.

⚠️ Do not expect a warning from anywhere: the GLSL compiler dead-strips a pure texture fetch in
silence, glslang emits nothing, and the validation layers see a perfectly legal pipeline.

### `Location::Top` orders by EMISSION, so a generator that CONSUMES must run after the one that DECLARES (Aug 2026)

Two blocks of `StandardResource::generatePBRFragmentShader()` both emit at `Location::Top`, and the
order there is simply the order the C++ ran. Wiring `KHR_materials_volume`'s thickness map into the
grab-pass ray length put the consumer (the transmission block) **above** the producer (the texture
component block) and produced, at runtime:

```
ERROR: 0:132: 'SurfaceVolumeThickness' : undeclared identifier
```

⚠️ **The C++ compiles either way.** Nothing in the build, and nothing in the generator, notices that
a texture component is read before it is declared — only the launched engine says so, and only
because it prints the erroneous GLSL in full. Whenever a component's variable is consumed by another
generation block, check which of the two runs first, and leave a comment at the site saying so;
`ComponentType::VolumeThickness` now carries one.

### The fragment stage has NO view matrix — plan the space you work in before writing the maths (Aug 2026)

Writing a screen-space refraction in world space compiles fine in C++ and dies at GLSL compile time
with `'viewMatrix' : no such field in structure 'ubView'`. Three facts, all of them easy to assume
away:

- The regular View UBO (`Generator::Abstract::declareViewUniformBlock()`) carries
  **`projectionMatrix` but no view matrix**. For regular rendering the view matrix is a push
  constant.
- That matrices push constant is declared **`VK_SHADER_STAGE_VERTEX_BIT | GEOMETRY`** only.
  A fragment shader cannot read it.
- The **cubemap** View UBO does carry view matrices, but inside a per-face `instance[]` array indexed
  by `gl_ViewIndex`, a **vertex** input — so `ViewUB(..., true)` is a vertex-stage-only expression.

⚠️ The failure surfaces as a **runtime shader-compile error**, not a build error: the generator emits
the text happily. It is caught only by launching and reading the log — one more reason the engine
prints the erroneous GLSL in full.

The way out is usually to change space rather than to widen a push-constant range or grow the UBO
(both have engine-wide blast radius): **view space costs nothing extra** — the camera is the origin,
so the incident direction is `normalize(positionViewSpace)`, `projectionMatrix` alone finishes the
job, and the transform is rigid so world-space LENGTHS carry over untouched. See
`src/Saphir/AGENTS.md` § "Screen-space refraction is done in VIEW space".

### ⚠️⚠️ Editing an asset while the engine is running gives you the OLD one, silently (Aug 2026)

`Core.openFiles()` goes through the resource manager, which serves a **cached** material and
geometry for an asset it has already loaded this process. Regenerating
`VolumeAbsorptionProbe.glb` with a five-fold different thickness and re-capturing produced a
**byte-identical** image — max diff 0 — and the natural reading of that is "the change did nothing",
which sent the diagnosis down a wrong path.

**Restart the engine after touching a `.glb`**, and make the first check a diff against the previous
capture: if it is exactly zero, suspect the cache before suspecting the change. A real no-op is
usually zero for a *structural* reason you can state in advance (a material that does not declare
the feature, a factor of zero); a zero you cannot explain that way is a stale load.

### ⚠️⚠️ A test can lose its discriminating power when you remove the defect it was measuring through

`VolumeAbsorptionProbe` (bench asset, three glass spheres: no volume / attenuation colour without
distance / colour **and** distance) used to separate its third sphere cleanly — green cap, measured
`(161,175,155)` against `(219,220,220)` for the two controls. After the transmission composition was
corrected the three came out **identical to within 0.5 of a code value**, and the probe now proves
nothing.

That is not necessarily a regression in the absorption. The green came from the *additive light-pass
term* — `albedo × beerAbsorption × radiance`, driven by a 100 000 lux key light — which is precisely
the defect that was removed. With it gone, the pixel is dominated by
`reflectedColor * fresnelDielectric`, and the transmitted term it should be discriminating is
sampled from a **near-black background** (the probe floats against dark trees), so absorbing 99 % of
almost nothing is invisible.

⚠️ The lesson generalises past this probe: **a control that discriminates through the defect stops
discriminating when the defect is fixed, and its silence then reads as a regression.** Before
declaring one, check what the criterion was actually measuring.

**Resolved 2026-08-29, and the second cause was in the asset, not the engine** — see the next entry.
The probe got the bright backdrop it needed AND a winding fix, and now reads
G/R = 0.993 / 0.993 / 14.717.

### ⚠️⚠️ Inverted triangle winding is INVISIBLE on a closed mesh — it shows up as Fresnel = 1 (Aug 2026)

`VolumeAbsorptionProbe`'s generated spheres had their two triangles wound the wrong way round. A
closed convex mesh looks **exactly the same**: back-face culling keeps the far side instead of the
near one, and a sphere is a sphere either way. Nothing in the geometry, the validation layers or the
log says a word.

What betrays it is the **shading**: the outward normal then points away from the camera, so

```glsl
NdotV = max(dot(reflectionNormal, -reflectionI), 0.0)   // clamps to 0 everywhere
F     = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0)          // = 1 everywhere
```

A Fresnel term pinned at 1 means **total reflection and zero transmission**: the glass renders as a
dark mirror and no amount of work on the transmission path changes anything. Three separate
hypotheses were burned on the engine before the geometry was suspected — the grab pass, the render
lists, the composition — and all three were wrong.

⚠️ **Suspect the winding whenever a transmissive or refractive surface refuses to transmit while its
grab-pass sample is provably correct.** The check is one line: output `1.0 - F` and see whether it is
zero.

**The method that found it, worth reusing.** Bisect the shading expression by writing intermediate
factors straight to the fragment colour, packed three at a time into R/G/B and scaled into the
tone-mapper's range (`* 20000.0` here, since these are linear radiance values and an un-scaled
[0,1] factor tone-maps to black):

1. the raw sample — `fragmentColor.rgb = SurfaceTransmissionColor;` → read the panel exactly, so the
   grab pass was innocent;
2. the whole term — `transmittedLight * transmissionFactor * (1 - F)` → exactly 0;
3. its factors — `vec3(transmissionFactor, albedo.g, absorption.g) * 20000.0` → all non-zero, and the
   absorption already discriminated correctly;
4. what was left — `vec3(1.0 - F, ior / 3.0, F0) * 20000.0` → `1 - F` **zero** with a small F0, which
   is only possible if `NdotV` is zero.

Four builds, each answering one question. Guessing at the engine cost more than that.




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
  for temporal reprojection forced the migration to a **per-frame UBO** (`FrameUBOData`,
  owned by `Graphics::GIDenoiser` since the Aug 2026 extraction).
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

### Critical: World-Space Y Reconstruction from Depth (Y-UP)

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
> **Code reference:** `Effects/Atmosphere/AtmosphericFog.cpp` — shader `cameraUp` computation

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
> **Code reference:** `Effects/Atmosphere/AtmosphericFog.cpp` — shader inscattering section

### Critical: Environment Cubemap Sampling Convention (RAW direction since Y-up)

> [!CRITICAL]
> **A world direction `D` samples any environment cubemap RAW — `D` itself, never a
> negated component.** The world is Y-UP and cubemaps are stored Y-up: nothing to
> compensate.
>
> ⚠️ **The old rule `vec3(D.x, -D.y, D.z)` is DEAD.** It was correct only while the world
> was Y-down (UP = -Y). Do not restore it from an old capture, an old comment, or this
> paragraph's history. Sites that must all agree: the skybox
> (`Material/Helpers.cpp` `checkPrimaryTextureCoordinates`), the material reflections
> (`StandardResource` bindless reflection GLSL), `LightGenerator`, SSR, RTGI, RTR,
> `IBLBaker`.
>
> **Symptom of a stray negation:** the sky is read upside-down — GI bounces tinted by the
> ground where the sky should be, ray-miss reflections showing the wrong hemisphere,
> and on the skybox itself the +Y/-Y faces swapped with the four side faces mirrored
> vertically. **Invisible on a near-uniform sky, and no assertion can see it.** The only
> detector is an axis-labelled cubemap on screen: `coordinates-debug` +
> `AxisDebug` (bright R=X+, G=Y+, B=Z+, Cyan=X-, Magenta=Y-, Yellow=Z-, same palette as
> the compass) — the face colour must equal the compass sphere colour in that direction,
> for all six. ⚠️ Looking at the nadir needs the camera BELOW the ground
> (`Act.setPosition(0,-30,0)`) or the ground plane masks the `-Y` face entirely.
>
> **What is NOT a defect:** each face displays **mirrored horizontally** relative to its
> stored pixels. Said properly: **a cubemap face stores the view of that wall taken from
> OUTSIDE the cube, looking inward** — the standard Vulkan cube-face mapping is left-handed
> while the world is right-handed (`screen-right = look × up`, measured). The engine does
> NOTHING at load: `CubemapResource::load(Pixmap)` cuts the 3×2 cross into six pixmaps and
> uploads them as-is. The equirectangular loader already bakes the convention in
> (`u = atan2(dz,dx)/2π + 0.5` reads non-mirrored on screen). Do not "fix" it with a
> negation at a sampling site — that would desynchronise the display from the IBL, which
> samples the same cubemaps.
>
> **Seam-continuity test — decides an asset's authoring convention numerically, no launch.**
> Under the hardware convention the four side faces form a closed ring whose seams are
> pixel-exact:
>
>     +X col 0 ≡ +Z col N-1    +Z col 0 ≡ -X col N-1
>     -X col 0 ≡ -Z col N-1    -Z col 0 ≡ +X col N-1     (rows correspond directly)
>
> An "authored from inside" (mirrored) set matches the SAME column pattern against the
> OPPOSITE neighbour (`+X col 0 ≡ -Z col N-1`, …). Compare the mean absolute difference of
> both pairings. Measured Aug 2026 over the 11 packed assets: 9 give **0.17–1.87** for the
> outside pairing against **6.22–56.85** for the inside one — the engine convention is
> confirmed by the content itself. Two are mute, and knowing why matters: `DNCity` reuses
> ONE image for both `+X` and `-X` (MAD 0.0), which makes the two hypotheses symmetric, and
> `AxisDebug` is flat colour per face. **A tie is a degenerate asset, not a failed test.**
>
> **Correcting an inside-authored asset:** flip each face tile horizontally **in place**.
> ⚠️ Never mirror the whole packed cross — that also exchanges the columns, moving faces onto
> the wrong axes. All six tiles take the same flip, poles included (`+Y`/`-Y` keep their top
> edge on the same axis, `-Z` and `+Z` respectively). `AxisDebug.Packed.png` was re-authored
> this way (Aug 2026) and is now correct: **its labels read BACKWARDS in an image editor and
> upright on screen. That is the correct state — do not "fix" the file.**
>
> ⚠️⚠️ **Reading a POLE capture: a pole face has no absolute on-screen orientation.** Its
> apparent rotation is set by the camera YAW, because looking straight up/down leaves the
> screen-up direction entirely to the body heading. A zenith shot taken while facing `-Z`
> shows `Y+` rotated 180° (`+⅄`) on a perfectly correct asset. **Never conclude from a pole
> capture without declaring its yaw** — shoot `lookAt(0,2,±100)` first, then the pole.
> Discriminator when in doubt, and it is exact: a 180° ROTATION preserves orientation
> (`Y+` → `+⅄`), a horizontal MIRROR does not (`Y+` → `+Y`, the `Y` staying upright).
> Getting this backwards turns a yaw artefact into a phantom asset defect.

### POM GPU Stress on Large Surfaces

Parallax Occlusion Mapping ray-marching is expensive at far distances, especially on large surfaces. The engine implements distance-based fade (default 8-18 m, per material through `setParallaxFadeDistances()`) and skips the march beyond it. See: `Graphics/AGENTS.md` (POM section).

### IRIDESCENCE TOOK THE BASE'S REFLECTANCE AGAINST AIR AS THE FILM-TO-BASE TERM (fixed 2026-09-22)

`evalIridescence()` existed twice (direct light, ambient pass) with `R23 = baseF0`, the base against AIR:
- a base with F0 = 0 gave R23 = 0 and an achromatic film;
- a base whose IOR equals the film's still reflected;
- there was no interface phase shift.

On both Khronos iridescence grids the colour sat in the wrong rows. The reference has it at LOW base IOR
and dark metal; ours had it at the top. Found by the macOS bench.

It is now ONE definition, `LightGenerator::declareIridescenceFunctions()`: Belcour & Barla 2017, as in the
Khronos glTF Sample Viewer (Apache-2.0). The base IOR is recovered from F0, the film-to-base Fresnel is
taken at the refracted angle, the π phase shifts are applied, and the spectral sensitivity is evaluated in
XYZ, then converted to Rec.709. Re-captured on Linux: the trend now matches the Khronos screenshots, with
0 VUID. ⚠️ The earlier bench verdict "the film sweep reads" judged the whole frame; judge a grid ROW by ROW
against the reference.

### A DOUBLE-SIDED BACK FACE REFLECTED AS AN UNTINTED MIRROR (fixed 2026-09-22)

Only the direct light turned the shading normal toward the viewer (`N = dot(N, V) < 0.0 ? -N : N`,
two-sided lighting, `5bec23db`). Every other normal kept pointing away on a back face:
- the environment normal: NdotV clamped to 0, the Fresnel pinned at 1, and F0 was lost. The back of
  `NormalTangentTest`'s gold plates reflected the sky with no gold, and the pink ones became mirrors;
- the ambient pass's IBL diffuse normal: the irradiance was read BEHIND the face;
- the G-buffer normal the post-process effects read.

It was found by the macOS bench and reproduced on Linux. Now every normal a term uses faces the viewer,
by the same rule (`faceforward(N, I, N)`, view vector `-I`). The environment normal is built ONCE by
`StandardResource::declareEnvironmentFrame()`: it was redeclared by seven sites, and it also mixed spaces
(`TangentToWorldMatrix[0..1]` is a VIEW-space frame, `NormalWorldSpace` a world one). It is now
`WorldTBNMatrix · n`, in world space end to end.

Measured on the glTF bench (Linux, ScreenSpace lane, 16 captures): only the two back views of the
double-sided tests move (12 % and 9 % of their pixels). Every front view is unchanged to 0.01/255. That
proves the old space mix was invisible from those FRONT poses (view ≈ world rotation), and bench poses
cannot vouch for it elsewhere.

### A "BROKEN" POM WAS THREE DEFECTS AND ONE BAD TEXTURE (fixed 2026-09-22)

Owner reports, 2026-09-08 and 2026-09-22: the parallax "looks blown out" (*éclaté*). Three shader
defects and one data defect, each enough to produce it on its own:

1. **Wrong space.** The tangent-space view vector was `transpose(TangentToWorldMatrix) · (camera − P)world`.
   `TangentToWorldMatrix` is `NormalMatrix · (T, B, N)`, a VIEW-space frame outside MDI: the march
   direction turned with the camera. Now `WorldTBNMatrix`. ⚠️ The name lies; read the synthesis
   (`AbstractVertexStage::synthesizeTangentToWorldMatrix()`) before using it on a world vector — the
   reflection-normal code still does, unattributed.
2. **The v sign.** `B` is the image's +Y (Khronos), i.e. DECREASING v: a tangent `(x, y)` is `(x, −y)` in UV.
   Verify with the relief read along ±X AND ±Z — a sign error on one axis shows raised along one and
   sunken along the other.
3. **Implicit derivatives in a per-pixel loop** — undefined; every march sample is a `textureGrad()` with
   the gradients of the undisplaced coordinates. Plus a bisection refinement: one linear step left the
   layers as visible slices.
4. **The texture.** ⚠️⚠️ `Grounds/Pavement005-height` is the photo's LUMINANCE (correlation 0.927 with the
   albedo): the correct shader extruded its granite grain into spikes. Measure a height map BEFORE judging
   the technique: correlation with the albedo luminance (a height is near 0), and the Frankot-Chellappa
   integral of the normal map (a coherent pair > 0.95, and its range IS the right `heightScale` in UV).
   `Pavement006` passes both (−0.001, 0.993, 0.021 UV).

Also removed: the layer count was a GLSL literal outside the program cache key (served stale through the
on-disk SPIR-V cache). It is a material UBO value. Bench: projet-alpha `relief`.

### A HARD-EDGED SPOT DIVIDED BY ZERO IN EVERY RASTER SHADER (fixed 2026-08-10)

The spot cone factor is generated as:

```glsl
const float epsilon = innerCos - outerCos;
const float spotFactor = clamp((theta - outerCos) / epsilon, 0.0, 1.0);
```

> [!CAUTION]
> **`inner == outer` is not a degenerate case — it is how a HARD CONE EDGE is expressed**, and it is
> exactly what USD's `shaping:cone:softness = 0` means, which is what **every fixture of an
> Omniverse Kit export declares**. `epsilon` is then **zero**: the division is `0/0` for fragments
> on the edge and `x/0` elsewhere, the result is **driver-dependent**, and it came back as a **pure
> black frame** on 25 correctly-placed 3750 cd ceiling spots — with no error, no validation message
> and a light set truthfully reporting all 25.

**The ray-traced path had always guarded it** (`RTR.cpp`, `RTGI.cpp`:
`max(innerCos - outerCos, 0.0001)`); the four raster generators had not. Fixed identically in all
four, which is the rule for this codegen — **a shading change must cover every generator, not just
PBR**:

- `Saphir/LightGenerator.PerVertex.cpp`
- `Saphir/LightGenerator.PerFragment.cpp`
- `Saphir/LightGenerator.PerFragment.NormalMap.cpp`
- `Saphir/LightGenerator.PBR.cpp`

**Diagnostic value**: this defect is invisible to every tool the project normally reaches for. No
Vulkan validation error, no shader compile warning, no log line — only pixels. When a light is
provably present in the light set and provably placed, and the frame is still black, **suspect the
generated cone/attenuation arithmetic before suspecting the light**.

### A radius of 0 DISABLES attenuation, it does not disable the light

`AbstractLightEmitter::DefaultRadius` is `0.0F`, and the codegen guards on it:
`if ( lightRadius > 0.0 )`. So a point or spot light with no radius has **no distance falloff at
all** — it is not dimmed to nothing, it reaches everywhere in its cone. `SceneDataConsumer` only
calls `setRadius()` when a loader declared a range (`LightDescriptor::range > 0`), so asset-imported
lights land in exactly that state. Worth knowing before blaming a radius for a dark scene: the
symptom of a zero radius is an **over**-lit room, never an under-lit one.

### The shader caches are ON by default — and what keeps that safe (Aug 2026)

Two caches and one dump sit on the shader path. Their defaults live in `SettingKeys.hpp`:

| Setting key | Default | What it actually is |
|---|---|---|
| `Core/Graphics/Shader/EnableBinaryCache` | **`true`** — flipped from `false` in Aug 2026 | the SPIR-V blob cache; skips glslang on a hit — **383 ms** on the demo below |
| `Core/Graphics/Shader/EnablePipelineCache` | `true` (unchanged) | the engine-side `VkPipelineCache` blob; the **bigger** win of the two — 5702 ms → 31 ms |
| `Core/Graphics/Shader/EnableSourceCodeDump` | `false` | **not a cache — a DUMP** (see below); renamed from `EnableSourceCodeCache` in Aug 2026 |

**Measured (2026-08-13, demo `material-debug` with all 10 options, RTX 3070 Ti, Release).** The
instrumented envelope is *source dump + cache lookup + glslang compile + `vkCreateShaderModule`*,
placed AFTER the in-memory hash lookup so it counts **cache misses only** — 232 shader modules:

| Binary cache | Total | Per module |
|---|---|---|
| OFF | 393 ms | 1.69 ms |
| ON, cold (writes the 232 blobs) | 391 ms | 1.68 ms |
| ON, warm (reads) | **10.3 ms** | **0.044 ms** |

**38× faster, 383 ms saved.** Writing the cache on a cold run is **free** (391 vs 393 ms = noise),
so there is no first-launch penalty to weigh against it. 0 residual `.tmp` files.

> [!CAUTION]
> **This cache is only safe because of its file HEADER. Do not "simplify" that header away.** A blob
> is valid only for the exact source AND the exact toolchain that produced it; hand a stale one to
> `vkCreateShaderModule` and you get a driver-level fault or garbage pixels, with no compile error
> anywhere to point at. That is why the cache shipped disabled until the header existed.
>
> Every field is validated **in full before a single byte reaches Vulkan**
> (`Saphir/ShaderManager.cpp`, `ShaderBinaryFileHeader` + `toolchainIdentity()`): magic, format
> version, source hash, shader stage, data size, FNV-1a content hash, and above all a **toolchain
> identity hash** — glslang version + SPIR-V generator version + client/target environment pair +
> engine version. Two structural checks follow: size a multiple of 4, and the SPIR-V magic word
> `0x07230203` in front. A rejected file is **deleted and the shader recompiled** — never repaired,
> never partially trusted. Writes land on a temporary path and are **renamed** into place, so a
> SIGKILL cannot leave a half-written blob behind.
>
> The toolchain hash is what makes a glslang upgrade *invalidate* the cache instead of silently
> poisoning it. It also carries the platform target pair: **macOS targets Vulkan 1.2 / SPIR-V 1.5
> while the other platforms target 1.3 / 1.6**, so a cache directory that travels between platforms
> rejects itself rather than mixing generations.

**The source code dump is NOT a cache — which is why it was renamed in Aug 2026.** Nothing ever
reads it back: `AbstractShader::loadSourceCode()` has **zero callers**, and the key it stores under
is a hash of the source itself, so it structurally cannot be one. It writes
`~/.cache/<app>/generated-shaders/`, one lazily-created subdirectory per generator
(`SceneRendering/`, `ShadowCasting/`, `PostProcessing/`, `OverlayRendering/`, `GizmoRendering/`,
`TBNSpaceRendering/`) so you can inspect what the generators produced. It is written BEFORE the
binary-cache lookup, so a cache hit does not suppress it. Enable it to *read* generated GLSL, never
to speed anything up — it stays OFF by default.

> [!CAUTION]
> **The Aug 2026 rename breaks existing settings, silently and by design.**
> `EnableSourceCodeCache` → `EnableSourceCodeDump`, and the directory `shader-sources/` →
> `generated-shaders/`. `Settings` has **no key-migration mechanism at all**: an existing
> `settings.json` keeps the old key as dead JSON, it is ignored without a warning, and anyone who
> had the dump enabled finds it **OFF** until they set the new key. The owner accepted the break
> because this is a debug facility defaulting to `false` — a key rename touching anything a user
> actually relies on would need a migration path first.

**Pipeline cache context** (`Vulkan::Device` owns the `VkPipelineCache`, `Graphics::Renderer` does
the disk I/O): 294 graphics pipelines on the same demo — driver cache active **33 ms**, driver cache
OFF **5702 ms**, driver cache OFF but the engine blob restored from disk **31 ms**. Factor **182×**,
for a 7.4 MB blob. The engine cache is what protects a cold machine, a driver update or a wiped
driver cache from a 5.7-second stall.

**What really drives load time is the VARIANT COUNT**, not the per-shader compile speed: a single
`material-debug` load generates **336 distinct SceneRendering sources** (265 fragment, 71 vertex).
Attack that number before micro-optimising the compiler path.

`--clear-renderer-cache` — renamed from `--clear-shader-cache` in Aug 2026 — wipes **three** on-disk
renderer caches: the SPIR-V binary cache, the pipeline cache, and the BC7 texture cache (see below).
An unknown switch is simply *absent*, so a script still passing the old name clears **nothing**, and
says nothing.

**Files:** `SettingKeys.hpp` (the three defaults), `Saphir/ShaderManager.cpp` (header struct,
`toolchainIdentity()`, write-then-rename), `Vulkan/Device.{hpp,cpp}` + `Graphics/Renderer.cpp`
(pipeline cache). Engine commits `56fabc9a` (binary cache hardening), `e583df40` (pipeline cache).
Contracts: [`src/Saphir/AGENTS.md`](../src/Saphir/AGENTS.md),
[`src/Vulkan/AGENTS.md`](../src/Vulkan/AGENTS.md).

### `disableOptimizer` is a SILENT NO-OP — glslang is built with `ENABLE_OPT=OFF` (Aug 2026)

> [!CAUTION]
> `Saphir/ShaderManager.cpp` sets `glslang::SpvOptions::disableOptimizer = true`, which reads like a
> deliberate performance decision somebody could flip to gain something. **It does nothing.** glslang
> in this build is compiled with `ENABLE_OPT=OFF`: `libSPIRV.a` contains **ZERO SPIRV-Tools symbols**
> (verified 2026-08-13), so the optimizer that flag would disable is not linked in at all. Setting it
> to `false` changes not one byte of the emitted SPIR-V — and emits no warning saying so.
>
> Making the flag meaningful would mean adding **SPIRV-Tools to the dependency cascade**, for no
> gain: desktop NVIDIA/AMD drivers fully re-optimise the SPIR-V they receive. Do not spend a session
> chasing a shader-optimisation win through this flag; measure the driver-side result instead.

### Flipping a default to ON runs a path nobody had ever run (Aug 2026)

Turning `EnableBinaryCache` on by default did not break anything — but it made the engine execute,
on every machine, code that until then only ran for whoever manually set the flag. Four defects
surfaced in a single fresh-install run, all of the same family: **an absent file or an empty path
is the NOMINAL first-launch state, and every one of these sites treated it as a failure.**

| Site | What it did on a first launch |
|---|---|
| `ShaderManager::readBinaryCache()` (then named `readCache()`) | Scanned the source-dump directory too. The dump is OFF by default, so its path is empty ⇒ `IO::directoryEntries("")` logged an error at **every** startup. It also indexed into `m_cachedShaderSourceCodes`, a member nothing ever read back — the loop was pure dead work. Both are gone; the function now returns early when the binaries directory is empty. |
| `ShaderManager::clearCache()` | Same empty-path scan, and `--clear-renderer-cache` runs whatever the settings say. Both loops are now guarded. |
| `Renderer::loadPipelineCache()` | Read `pipeline.cache` unconditionally ⇒ `IO::fileGetContents` logged an error on the one launch where the file is *supposed* to be missing. Now checks existence first and starts empty, silently. |
| `--clear-renderer-cache` | Erased the blob and the `.loading` marker unconditionally ⇒ an `IO::eraseFile` error per absent file. Guarded. |

> [!CAUTION]
> **A feature that ships disabled has an untested first-run path, and flipping the default is what
> executes it.** None of these were caught by the build (`-Werror` clean), by the unit suite
> (1967/1967), or by any warm run — only by deleting the cache directories and watching a genuinely
> cold start. When you enable something by default, **delete its state and run it cold**, then read
> the log for errors that are really just "nothing here yet".

Verified after the fixes: a fresh install and a `--clear-renderer-cache` run on a completely absent
cache both log **zero** errors (only the pre-existing, unrelated GLFW Wayland gamma-ramp one).

### `std::stoull` in a `-fno-exceptions` build terminates the process (fixed Aug 2026)

`ShaderManager::extractHashFromFilepath()` parsed the hash out of a cache filename with
`std::stoull`. The cascade builds with `EMERAUDE_DISABLE_EXCEPTIONS` **On** by default, so the
`std::invalid_argument` it throws on a malformed name had nowhere to go: a stray file named
`foo_bar.vert` dropped into the **user-writable** cache directory called `std::terminate` **at
startup**. Replaced with `std::from_chars`, which reports failure through a return value.

> [!CAUTION]
> The throwing `std::sto*` family is a **crash primitive** in this cascade, and the danger is
> proportional to how untrusted the input is — a cache directory the user can write to is about as
> untrusted as it gets. Prefer `std::from_chars` for every parse of external data.

### A cache key that LOOKED content-sensitive and was not — the BC7 texture cache (fixed Aug 2026)

The BC7 disk cache keyed its entries on `SHA256(resourceName | sourceFileSize | sourceModTime)`.
Nothing in that signature looks wrong, and the class documentation duly claimed "file size and
modification time". But the **caller** passed the **decoded pixel byte count** as `sourceFileSize`,
and `width * 1000000 + height` as `sourceModTime`. The key therefore reduced to **name +
dimensions**: repaint a texture without changing its size and the stale blob was served forever, no
error anywhere.

Fixed by making it content-addressed — `Base::Hash::FNV1a` over the **decoded pixels**, folded with
width, height and `colorCount` (`TextureCache::cacheKey()`). Correct by construction, and it needs no
plumbing through `ResourceTrait`. File format `Version` **1 → 2**.

> [!CAUTION]
> **Check what a cache key is FED at the call site, not what its parameters are NAMED.** A hash of
> the wrong inputs is indistinguishable from a hash of the right ones — same length, same
> distribution, no validation can catch it — and the only symptom is an edit that never takes effect.
> The parameter names, and the doc comment repeating them, actively hid the defect. Same discipline
> as the binary-cache header above: a cache is only as trustworthy as the identity it keys on.

**No migration concern:** the engine is pre-release, so there is no deployed version whose caches
would need converting. Worth keeping for whoever touches `cacheKey()` NEXT, though: changing a
content-addressed key scheme does not *invalidate* existing entries, it **orphans** them — their
filenames simply stop being produced, so they sit there unreachable. `--clear-renderer-cache` is the
remedy (**40** stale entries erased here when the key changed).

**What the cache is worth** (`material-debug`, all 10 options, RTX 3070 Ti, Release): cold cache
**231** mip-level compressions for **7705 ms** of BC7 compression; warm cache **0** compressions,
**0 ms**. That is ~7.7 s of load time — more than the `VkPipelineCache` (5702 → 31 ms) and about
twenty times the SPIR-V binary cache (393 → 10.3 ms). Zero compressions on the warm run is also the
proof that the content-addressed key is deterministic: every texture found its entry.

**On disk:** `~/.cache/<app>/texture-cache/`, extension `.bc7cache`. Documents claiming
`~/.cache/AppName/TextureCache/` are **wrong** — the code has always used `texture-cache`.

**Only one of the two BC7 paths touches this cache**, and which one runs depends on the **source**,
not on a setting: an `ImageResource` (PNG, JPEG, procedural) is CPU-encoded at load time and cached;
a `CompressedImageResource` (KTX2 / `KHR_texture_basisu`) arrives already block-compressed, is
uploaded verbatim, and touches neither bc7enc nor the cache.

**Files:** `Graphics/TextureCache.{hpp,cpp}` (`TextureCacheService`) and
`Graphics/TextureCompressor.{hpp,cpp}` (`TextureCompressorService`). Both were "a grouping of
statics" and are now real `ServiceInterface` sub-services: value members of `Graphics::Renderer`,
enrolled by `initializeSubServices()` (compressor **first** — the cache holds a reference to it),
terminated in reverse order with the rest, reachable as `renderer.textureCompressor()` /
`renderer.textureCache()`, both **const** references. All mutable static state is gone —
`bc7enc_compress_block_init()` now runs in `onInitialize()`, so no caller can reach a compression
method before the encoder is ready, and the static `initialize()` one had to remember to call (whose
omission only logged a runtime error) is deleted. `TextureCache::getOrCompress()` owns
lookup → compress → store, so `Texture2D::createFromPixelData()` no longer orchestrates
try/compress/store itself. The second migrated call site, `CompressedImageResource::load()`, does
**not** go through the cache: it calls `compressSingle()` on the compressor sub-service directly, for
its own procedural default payload only — which is why the "touches neither bc7enc nor the cache"
statement above still holds for every KTX2 texture that actually comes from disk.

> [!WARNING]
> **BC7 compression is NOT parallelised across blocks.** `compressLevel()` received a
> `Base::ThreadPool` and never used it; `compress()` and `compressSingle()` no longer take one.
> Compression is sequential per texture — the parallelism comes from the resource manager loading
> several textures concurrently on different workers. Any document claiming otherwise is false.

---

### A sound with no buffer used to kill the engine (fixed 2026-08-27)

`Audio::Source::play()` called `m_currentPlayableInterface->buffer()->identifier()` — and
`PlayableInterface::buffer()` returns **null while the sound resource is not loaded, or failed to
load**. The first collision of a scene whose sound was not ready dereferenced a null pointer inside
`AbstractObject::identifier()` (`this = 0x0`) and took the whole application down.

**Reproducer**: `--load-demo lighten-marbles`, ~10 s in, when the first marble lands
(`Marble::onCollision` → `SoundEmitter::replay` → `Source::play`). The demo is rarely started, which
is why a null-dereference survived there. `basic-scenery` and a launch without a demo were unaffected.

Both buffer paths (streaming queue and single buffer) now check for null and cancel the playback with
a warning. ⚠️ **A resource that is not ready is a normal state in an asynchronous loader — never
dereference what a resource accessor returns without checking it.**

## Build / Compiler

### An ext-deps patch hunk that UPSTREAM absorbs as an OPT-IN option silently re-arms what it disabled

> **Symptom:** a feature that worked for weeks stops working after nothing in the cascade changed,
> and the error names a THIRD-PARTY source file. Measured case (2026-09-14): `world-lobby` refused
> to load on
> `composition.cc:LoadAsset():466 Unsafe asset path in composition: '../Source/Lobby/assets_Lobby.usd'`.
>
> **Root cause:** the behaviour was supplied by a local patch in
> `ext-deps-generator/patches/<lib>.patch`. Upstream later implemented the same thing — but as an
> **option defaulting to the restrictive value** instead of a relaxed default. The patch hunk then
> becomes a genuine conflict against the updated sources and is correctly dropped when the patches
> are re-verified (`ext-deps-generator` commit `1191c0e`, 2026-08-31, for tinyusdz). Upstream's
> feature is present, the consumer never opts in, and the restriction returns.
>
> **⚠️ The trap is that the patch file looks HEALTHY afterwards** — it applies cleanly, the library
> builds, `git status` in `repositories/<lib>` is clean, and the library checkout carries no local
> modification to inspect. Nothing anywhere says a capability was withdrawn.
>
> **How to diagnose, in this order:**
> 1. `git log -p --follow -- patches/<lib>.patch` in `ext-deps-generator` and look for **removed**
>    hunks touching the symbol the error names. A "re-verify the patches" commit is the usual one.
> 2. Grep the updated upstream sources for that symbol — an absorbed feature almost always survives
>    as a struct field or a setter (`allow_parent_relative_paths`, `set_*()`), with the old
>    behaviour reachable by setting it.
> 3. Check the **installed** headers under `ext-deps-generator/output/<triplet>/include/`, not only
>    `repositories/`. If the option is there, the fix is a call-site change and needs **no ext-deps
>    regeneration at all**.
>
> **⚠️ Fix it by SETTING THE OPTION, never by re-patching the library.** A re-applied patch is
> silently reverted by the next update; a call-site flag survives it and is visible in our own code.
> An absorbed feature is upstream telling us the behaviour is supported — take the supported route.
>
> **⚠️ An opt-in restriction can also fail SILENTLY where the patch used to fail loudly**, because
> the option often sits next to `error_when_*` flags that default to `false`. On the case above one
> demo option errored out while the other reported success with an empty stage. When a dependency
> update is suspected, **set the `error_when_*` flags temporarily** — it names the asset the strict
> path is dropping in one run, where three rounds of guessing had named nothing.
>
> **Reference:** [`docs/scene-loaders-usd.md`](scene-loaders-usd.md) § 11.7 (the full case),
> `src/Scenes/Loaders/USDLoader.cpp` (`USDLoader::load()`, the four flags).

### CMake must be RECONFIGURED after a source file is renamed, moved or removed (`GLOB_RECURSE`)

> **Symptom:** after a `git mv` or a deletion, Ninja fails with
> `<OldName>.cpp missing and no known rule to make it`, or keeps compiling a file that no longer
> exists.
>
> **Root cause:** the source lists are built with `GLOB_RECURSE`, which is evaluated at
> **configure** time and cached. A build alone never re-globs.
>
> **Fix:** re-run the configure step on the affected build directory before building
> (`cmake -S . -B <build-dir> …`, then `cmake --build <build-dir> -j$(nproc)`).
>
> ⚠️ The rule is usually remembered for **added** files; renames and removals break the build in a
> way that reads like a corrupted tree, which is why it is written here. Paid during the
> `PBRResource.cpp` → `StandardResource.cpp` rename of the material merge (Aug 2026).

### ⚠️⚠️ Half the engine is NOT globbed — a new file under `src/Net/` (and friends) must be ADDED BY HAND

> **Symptom:** the new `.cpp` compiles nowhere, the build looks clean for a while, then the LINK
> fails with `undefined reference to EmEn::Net::Whatever::method()` coming from
> `libEmeraude.so` — *not* with anything naming the file you added. Reconfiguring does not help,
> and neither does a clean rebuild.
>
> **Root cause:** `cmake/PrepareEngineSourceFiles.cmake` uses **two** mechanisms, and the entry
> above (`GLOB_RECURSE`) only describes one of them:
> - **Globbed** (reconfigure is enough): `src/Animations`, `src/Audio`, `src/Console`,
>   `src/Graphics`, `src/Input`, `src/Overlay`, `src/Physics`, `src/Resources`, `src/Saphir`,
>   `src/Scenes`, `src/Tool`, `src/Vulkan`.
> - **Explicit lists** (the file must be typed into the CMake): `src/Net`, `src/Help`,
>   `src/PlatformSpecific`, and everything else at the top of that file.
>
> **Fix:** add the header to `EMERAUDE_HEADER_FILES` and the source to `EMERAUDE_SOURCE_FILES` in
> `cmake/PrepareEngineSourceFiles.cmake`, then reconfigure.
>
> ⚠️ The reflex "a link error means a missing library" sends you looking in entirely the wrong
> place. Read WHICH symbol is undefined: if it is one you just wrote, the file never got compiled.
> Paid while adding `Net::APIClient` (Aug 2026).

### ⚠️ A third-party archive linked by RAW PATH propagates nothing — the order of `include(Setup*)` IS the link order

> **Symptom:** the engine's own `.so` links fine, then the **first executable** to consume it fails
> with `undefined reference to` symbols of a library nobody in the project calls — e.g.
> `simdjson::internal::to_chars(...)` referenced by `libEmeraude.so`, while no engine source
> includes simdjson at all. On projet-alpha the casualty is `projet-alpha-helper` (the CEF helper
> process), which is linked *before* the main executable — so the build dies at 88% on the target
> that has the least to do with the change.
>
> **Root cause:** most `Setup<Lib>.cmake` scripts in emeraude-base link an archive as a **raw file
> path** (`target_link_libraries(... "${EMERAUDE_EXT_LIBS_PATH}/lib/libfastgltf.a")`), not as an
> imported target. CMake then knows nothing about that archive's own dependencies and cannot order
> them: the static link order is simply **the order in which the `include(Setup*)` lines appear**.
> When an upstream bump moves a bundled dependency out into its own archive — fastgltf 1.x
> externalising simdjson (Aug 2026) — nothing in CMake notices, and the archive is never passed to
> any link.
>
> ⚠️ **The `--exclude-libs` list is NOT evidence that an archive is linked.** It is a generated
> inventory of every archive in the ext-deps directory (`HideThirdPartyExports.cmake`); seeing
> `libsimdjson.a` in it while the symbol is undefined is exactly the trap. Check the link line's
> **inputs**, not the flags.
>
> **Fix:** add the dependency's own `Setup<Dep>.cmake` and `include()` it **after** the library that
> needs it. Done for simdjson: `cmake/SetupSimdjson.cmake` in emeraude-base (it owns every Setup
> script, even for libraries it does not use itself), included right after `include(SetupFastGLTF)`
> in the engine's `CMakeLists.txt`. simdjson ships a real CONFIG package, so the imported target
> `simdjson::simdjson` also carries `SIMDJSON_EXCEPTIONS=0` — consistent with the project's
> `-fno-exceptions` — for free.
>
> ⚠️ Adding a Setup script changes the compile definitions of the whole engine target, so the next
> build recompiles everything. That is normal, not a symptom.

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

The cascade-wide STL precompiled header (`emeraude-base/cmake/STLPrecompiledHeaders.cmake`) force-includes
the whole STL hot-set (`<ranges>`, `<string>`, `<iostream>`, `<sstream>`, …) into every translation
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
  - `Graphics/Effects/Lighting/RTR.cpp` — `RTRTraceFragmentShader` grew to ~18 KB (fixed Jul 2026).
  - `Graphics/Effects/Lighting/RTGI.cpp` — `RTGITraceFragmentShader` grew to ~16.6 KB, split at the
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

### ⚠️ An `EMEN_API` class defined ENTIRELY inline in a header no engine TU includes does not link on MSVC (Sep 2026)

- **Symptom (MSVC only):** the CONSUMER fails to link — `LNK2019` / `LNK2001` on `__imp_` symbols of the class
  (its constructor, destructor and virtuals), then `LNK1120`. Linux and macOS link fine.
- **Cause:** a `dllexport` class is instantiated and exported only by a DLL translation unit that sees its
  definition. When nothing in the engine includes the header, `Emeraude.dll` exports nothing for it, while the
  consumer sees `dllimport` and calls the `__imp_` entries. GCC and Clang emit inline members on the consumer
  side, so they never notice.
- **Seen in:** `Graphics/Geometry/DisplacedGridResource.hpp` (engine `cf16fd26`, caught by the Windows session on
  2026-09-23: projet-alpha's `Relief.cpp` was the only includer).
- **Fix (the engine convention):** a real `.cpp` with the constructor and the virtuals out of line — like
  `VertexGridResource.cpp`. A `.cpp` that only includes the header also links, but hides the reason.
- **Preventive:** every new `EMEN_API` class gets its `.cpp`, even when it is small.

### ⚠️⚠️ `FetchContent`: a `SOURCE_DIR` you point at an existing checkout gets ERASED — and `MakeAvailable` builds the fetched project unless you tell it not to (Sep 2026)

> **Context:** RenderDoc stopped being a submodule (Sep 2026) — 219 MB of sources cloned by everyone
> for a debug option that is `Off` by default. `cmake/SetupRenderDoc.cmake` now fetches them with
> `FetchContent`, and only when `EMERAUDE_ENABLE_RENDERDOC=ON`. Two traps were paid on the way.
>
> **Trap 1 — reusing a local checkout.** Making FetchContent reuse a tree already on disk *looks*
> like `FetchContent_Declare(... SOURCE_DIR <path>)`. It is not: `SOURCE_DIR` hands the directory to
> the underlying ExternalProject **git-clone step, which deletes its source directory whenever the
> stamp file is missing** — that is, from every freshly created build directory. Here it would have
> taken `dependencies/renderdoc/build/` with it (240 MB, several minutes of compilation, the Python
> analysis module `renderdoc.so`). The correct mechanism is the cache variable
> **`FETCHCONTENT_SOURCE_DIR_<UPPERCASE NAME>`**, the only one that makes FetchContent *skip the
> download step altogether*, and the one CMake documents for exactly this use.
>
> **Trap 2 — fetching sources without building them.** `FetchContent_MakeAvailable()` calls
> `add_subdirectory()` on what it populated. For RenderDoc that means building the capture library
> and `qrenderdoc` inside our build tree, when all the engine needs is one header
> (`renderdoc/api/app/renderdoc_app.h`, MIT). The documented download-only idiom is to point
> **`SOURCE_SUBDIR` at a directory that holds no `CMakeLists.txt`**; `MakeAvailable` then populates
> the sources and skips `add_subdirectory()`. Verified with a throwaway repository whose top-level
> `CMakeLists.txt` was nothing but a `message(FATAL_ERROR)`: it never fired.
>
> **Where things are now:** the revision is pinned once in `cmake/RenderDocPin.cmake` (v1.46) and
> read by both consumers — `SetupRenderDoc.cmake` (the header the engine compiles against) and
> `BuildRenderDocPython.cmake` (which clones the same revision on demand to build `renderdoc.so`).
> A checkout at `dependencies/renderdoc` is picked up automatically; `-DRENDERDOC_GIT_TAG=<tag>`
> overrides the pin. Nothing at all is downloaded while the option stays `Off`.

### ⚠️⚠️ A RenderDoc capture NEEDS the validation layers OFF (and X11) — with both layers loaded, nothing is ever presented (Sep 2026)

> **Symptom:** `Core.RendererService.triggerRenderDocCapture()` answers *"frame capture triggered
> (captured on the next present)"*, the app runs, and **no `.rdc` file is ever written**. Reported
> as an unexplained capture-workflow problem since Jun 2026; here is the measurement.
>
> **Two independent causes, both mandatory to fix, measured on RenderDoc 1.43 AND 1.46 (identical):**
>
> 1. **RenderDoc has no Wayland support.** `renderdoccmd version` prints *"Windowing systems
>    supported at compile-time: xlib, XCB, Vulkan KHR_display"*. Under Wayland its layer does not
>    expose `VK_KHR_wayland_surface`; GLFW then reports *"Vulkan: Window surface creation extensions
>    not found"* and `WindowService` dies on `VK_ERROR_EXTENSION_NOT_PRESENT` before frame one.
>    Fix: `Core/Video/Window/GLFW/UsePlatform` = `"X11"` (XWayland).
> 2. **The Khronos validation layer and the RenderDoc layer are mutually exclusive in practice.**
>    Loaded together, every present is rejected: `VUID-vkCmdDraw-None-09600` (a descriptor's image
>    sits in `PRESENT_SRC_KHR` where the descriptor was written expecting `GENERAL`), then
>    `VUID-vkQueuePresentKHR-pWaitSemaphores-03268`, `VUID-VkPresentInfoKHR-pImageIndices-01430`,
>    and `[VulkanQueue] Unable to present an image : VK_ERROR_VALIDATION_FAILED_EXT`. RenderDoc
>    wraps the swapchain with its own usage flags and layouts; validation tracks what it saw.
>    No present ⇒ no capture, silently. Fix: `Core/Video/VulkanInstance/EnableDebug` = `false` for
>    the capture run only.
>
> **Term isolation (same binary, same X11 platform), the reason this entry is trustworthy:**
> | Layers loaded | Result |
> |---|---|
> | validation only | 0 error, frame presented, screenshot fine |
> | RenderDoc only | `.rdc` written (549 MB), 0 error, replayed: 90 draw calls, 18 dispatches, 55 render passes |
> | both | 6 validation errors, `VK_ERROR_VALIDATION_FAILED_EXT`, nothing presented, no capture |
>
> ⚠️ This is the **documented exception** to the project rule *"validation layers ON for any
> rendering verification"*: for a capture run RenderDoc IS the instrumentation, and the `.rdc`
> replays the commands actually submitted. Turn validation back on for every other verification.
>
> ⚠️ Do not conclude "the engine has a layout bug" from those VUIDs without redoing the isolation:
> the same code presents cleanly when the RenderDoc layer is absent.

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

### Fixed: a 3D image uploaded ONE slice, and its mips filtered one slice (Sep 2026)

**Symptom:** the volumetric clouds' shapes (`Graphics::CloudShapeResource`, `VK_IMAGE_TYPE_3D`)
sampled as empty — no cloud, no error, **no VUID**.

**Cause:** `ImageTransferOperation::transfer()` copied `imageExtent.depth = 1` (and offset its layers by
`width × height`), so only slice 0 of a 3D image reached the GPU; `finalizeForGPU()`'s mip blit wrote
`z = 1` on both ends, and its `extent >> level` had no floor at 1 (a non-square 2D image's short axis
reached 0 before the long one finished its chain). A cloud keeps its first slice EMPTY (the growth
margin), so the whole volume read as air. Nothing in the engine had ever uploaded a 3D image: the
`Texture3D` resource depends on a `VolumetricImages` container that is not even registered.

**Fix:** the copy covers the full extent (depth included, layers offset by the whole slice stack);
the blit uses `max(extent >> level, 1)` on all three axes, depth included. A 2D image has a depth of 1:
its path is unchanged.

**Rule:** validation layers do not catch a copy that is merely SMALLER than the image — undefined
texels are legal. Verify a first-of-its-kind upload by reading it back or by seeing it, never by the
absence of VUIDs.


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
`Graphics/Effects/Camera/ToneMapping.cpp:createEffect()`

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

**Files:** `Graphics/SceneRenderTarget.cpp`, `Graphics/Effects/Camera/DepthOfField.cpp`

---

### A screenshot after a fixed `sleep` is not comparable between runs (Sep 2026)

**A freshly loaded scene keeps moving for tens of seconds.** Exposure adaptation, temporal
accumulation (TAA, SVGF) and animation start-up all drift the image well after the first frame.
Measured on `reflexion-debug --demo-options=0,6,0`, mean luminance of one crop:

| capture instant | t~4s | t~8s | t~12s | t~16s | t~20s |
|---|---|---|---|---|---|
| mean luminance | 136.5 | 129.8 | 126.8 | 125.9 | 125.7 |

> [!CAUTION]
> ⚠️⚠️ **Two scripts whose delays differ compare two different moments of the scene.** A bench
> capturing at t~4s against a reference captured at t~8s shows a **uniform +7 offset over the
> whole frame** — which reads exactly like a rendering regression. That happened during the RTR
> UBO port: the "regression" was a capture-timing artefact of the measuring script, and the two
> renders were in fact identical. The reverse mistake is just as available: matching sleeps by
> luck and calling it a verified null result.
>
> **Use `tools/demo-capture-bench.py`**: it probes the image until the peak-to-peak spread of the
> last N probes falls under a threshold, then captures. Two runs then agree to **0.229** mean
> luminance instead of 7.171 — and by construction, not by having copied the same `sleep`.
>
> ⚠️ **Shoot the same-run control before reading any pixel diff** (`--control`). On a scene with
> an animated subject the noise floor is enormous: two captures of the SAME run of `reflexion-debug`
> already differ on **59.3 %** of pixels (mean 1.873). Without that control the noise reads as a
> broken render. Conversely, keep a known-different pair around to prove the metric still
> discriminates — a diff that cannot detect a real change is not evidence of "no change".

**Two traps inside the measurement loop itself:**

> [!CAUTION]
> ⚠️⚠️ **The engine names a capture with a unix timestamp in SECONDS**
> (`~/.local/share/LNIsle/<app>/captures/1788369068.png`). Two screenshots taken within the same
> second write the **same file**, the second silently overwriting the first. Code that identifies
> a capture by its path alone then reports "no capture produced" while the engine did write one —
> measured, and it aborted a bench run. Compare `(path, st_mtime_ns, st_size)`.
>
> ⚠️⚠️ **A convergence test on consecutive deltas latches onto false plateaus.** Exposure
> adaptation moves in steps: the luminance sits perfectly still for one probe, then jumps again.
> A pairwise criterion converged two runs of the SAME binary at 128.40 and 127.19 (a 1.2 gap)
> because one run saw `delta 0.000` followed by `delta 0.483`. Test the **peak-to-peak spread of
> a window** of at least 3 probes instead.

**Files:** `tools/demo-capture-bench.py`, `AGENTS.md` § "When you need to verify a rendering change"

---

### Fixed: RTR was dead on min-spec — 148-byte push constant range (Sep 2026)

`RTR::TracePushConstants` was **148 bytes**, above the 128-byte Vulkan minimum guarantee for
`maxPushConstantsSize` and the only block in the engine over the floor. On a device exposing
exactly 128 — part of the AMD/Intel fleet — `PipelineLayout::create()` returned `false`, so
**RTR was never created and every scene declaring it silently lost its reflections**. It read
144 bytes before `b32f22d8` added `coneScale`, so it had been over the floor for longer than that.

The engine had been printing it all along, on every RTR pipeline layout build:

> `A push constant range ends at 148 bytes, above the 128-byte Vulkan minimum guarantee ! This pipeline layout will fail to create on min-spec devices.`

**Fix:** the trace pass reads `TraceFrameUBOData` from a per-frame UBO at set 1 binding 5, with
the **member list and order unchanged** — the std140 offsets coincide exactly with the C++ ones,
verified member by member against the emitted SPIR-V, and pinned by 13 `static_assert`s. Not one
line of the shader body changed. Details and the offset table:
`src/Graphics/AGENTS.md` § *RTR followed, for a defect that was already live*.

> [!CAUTION]
> ⚠️⚠️ **The failure is invisible on this workstation and on every NVIDIA GPU.** NVIDIA exposes
> 256 bytes, so both of the owner's machines took the `TraceWarning` path and RTR worked. Only a
> min-spec device takes the `TraceError` + failed-layout path. **Verifying this class of fix here
> means reading the ABSENCE of the warning in the log, not looking at the render.**
>
> ⚠️⚠️ **`reflexion-debug` cannot be read with a pixel diff.** It carries an ANIMATED dragon, so
> two runs of the SAME binary already differ on **59.3 %** of pixels (max 221, mean 1.873). The
> pre/post-port delta measured 84.6 % / mean 1.150 — *below* the same-binary control. Shoot the
> control first, or the noise reads as a regression.
>
> ⚠️ **A UBO here costs nothing measurable.** `vkCmdPushConstants` ran once per draw, not per
> pixel, and the values are uniform across the pass, so both forms land in scalar registers.
> `RTREffect/trace` cumulative average over ~2500 frames: 0.521 ms before, 0.519 / 0.522 ms after.
> Untouched passes drifted more between runs (`ScenePass` 1.544 / 1.473 / 1.483). Do not use "it
> would be slower" as a reason to keep a block over the floor.

**Verified:** projet-alpha cascade builds, the 128-byte warning is gone, 0 VUID (RTX 3070 Ti),
emeraude-base 2045/2045.

**Files:** `Graphics/Effects/Lighting/RTR.{hpp,cpp}`

---

### Fixed: ContactShadows drew a FACETED shadow terminator — tMin used as a normal bias (Sep 2026)

**Symptom (owner report):** an "ugly shadowed staircase" across the top of the DamagedHelmet
dome — large, axis-aligned, right-angled steps, not noise. Reproduce with
`--load-demo=asset-loader --demo-options=7,0,1,0,0,0`.

**Attribution, measured — three effects share that stack (RTGI, RTAO, ContactShadows), so it
was A/B'd, not assumed:** removing `ContactShadows` from the stack removes the staircase
entirely and nothing else changes. Then, term by term:

1. **Not the denoiser.** Its PCSS-lite blur radius scales with the hit distance
   (`cshdwRadius = maxBlurRadius * hitDist`) and early-outs below half a texel, so it is a
   pass-through on exactly the shadowed pixels. Forcing the radius to 0 left the staircase
   **unchanged** — the artefact is in the MASK.
2. **Not a screen-space grid.** The mask, visualised directly (`em_Color.rgb = vec3(shadow)`
   in the combine snippet), showed steps of 30–70 px where a half-res texel is 2 px.
3. **Self-intersection with the helmet's own triangles.** Raising the bias ×10 collapsed the
   staircase to a smooth curve.

**Root cause:** the bias was handed to `rayQueryInitializeEXT` as **`tMin`** — a distance along
the LIGHT direction — while the effect's own parameter was named `normalBias`. `normalTex` was
declared in the GLSL, bound by the renderer, written to the descriptor every frame, and
**never sampled**: the normal-offset machinery was never wired. At a terminator the light is
grazing by definition, so advancing along it never leaves the surface and the ray re-hits the
neighbouring facets — the terminator follows the MESH, which is what the staircase is.

Secondary, same pass: the depth was read with `texture()` at half resolution, where `vUV` lands
exactly on the corner of a 2×2 full-res block, so **every** pixel reconstructed its origin from
the average of four non-linear depths — a surface that does not exist.

**Fix (`Graphics/Effects/Lighting/RTContactShadows.{hpp,cpp}`), aligned on RTAO:**
`texelFetch` for depth AND normals; world position reconstructed from the FETCHED texel's centre
(not `vUV` — half a full-res texel apart); view→world normal via the inverse view rotation;
`rayOrigin = worldPos + worldNormal * adaptiveBias` with `tMin` a constant 0.001; adaptive bias
gaining RTAO's grazing term `min(1 / NdotV, 10)`.

**Numbers** (axis-aligned steps ≥ 4 px along the mask terminator, same camera pose):

| Variant | Steps ≥ 4 px | Flat boundary |
|---|---|---|
| before (tMin = bias 0.01) | **19** | 72.3 % |
| tMin raised ×10 — the trap | 2 | 47.8 % |
| **after (normal offset, bias 0.01)** | **0** | 47.9 % |

> [!CAUTION]
> ⚠️⚠️ **Raising `tMin` is not the fix, it is the trap.** It hides the staircase by skipping
> everything within the bias distance — including the near occluders a contact shadow exists to
> draw (peter-panning). The middle row above looks like a fix and costs the effect its purpose.
>
> ⚠️⚠️ **A binding that is declared, bound and written is NOT a binding that is read.** Every
> external sign said the normals were wired. Only grepping for the READ (`normalTex` appears
> once in the file, in its own declaration) showed the two halves were never connected. Compare
> against the sibling that works: `RTAO.cpp` samples it at line ~120.
>
> ⚠️ The push constants could not hold the inverse view rotation (132 bytes > the 128-byte
> minimum guarantee), so the pass moved to a per-frame UBO at set 1 binding 2. See
> `src/Graphics/AGENTS.md` § "ContactShadows reads its per-frame data from a UBO".

**Verified:** projet-alpha cascade builds, 0 VUID with the validation layers on, RTX 3070 Ti;
emeraude-base 2045/2045.

**Files:** `Graphics/Effects/Lighting/RTContactShadows.{hpp,cpp}`

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

### Fixed: the auto-exposure EMA had no sanitisation — one corrupt frame blew the screen white FOREVER (Aug 2026)

**Symptom (macOS only):** the tone mapping saturates to a blown-white frame and **never
recovers**. Toggling HDR off/on restores it, and looking at the sky breaks it again *suddenly*.
The auto-ISO readout in the physical-camera panel (Shift+F2) stays empty ("metering...")
indefinitely. Core validation and **Synchronization Validation are both clean**.

**Measured chain of causality** (do not re-derive it, these numbers are reproducible):

| Probe | Reading | Meaning |
|---|---|---|
| `adaptedLogLum` | `-9.203` = `log(1e-4)` exactly | the luminance chain measures **pure black** |
| resulting multiplier | `0.104 / 1e-4` = 1041, clamped | pinned against `maxExposure` -> white |
| `isnan/isinf(hdrColor)` boolean probe | 0 over the whole frame | the HDR input is **finite**; the scene is innocent |
| `isnan/isinf(adaptedLogLum)` | 1, flat, latched | the pollution is born **inside the chain** |
| `halfBits` of the history | `0x7E00` | a canonical quiet **NaN**, latched forever |
| `textureSize(inputTex,0).x` carried down the chain | **2320** instead of 2560 | a per-draw CONSTANT arrives corrupt: the chain samples **corrupt video memory** |
| `deltaTime` | 0.015-0.035 s throughout | innocent |

**Root cause — two independent defects, one visible failure.**

1. **The EMA is an infinite-impulse filter with no validity check.** A single non-finite sample
   poisons it permanently: `Inf - Inf = NaN`, and no weight ever removes a NaN. `resetHistory`
   guarded only the *first* frame after creation, which is exactly why recreating the effect
   (toggling HDR) "fixed" it every time. So a **transient** glitch became a **permanent** one.
2. **`clamp()` on a NaN is undefined and hardware-dependent.** On this path a NaN came out as the
   ISO **ceiling** — the brightest possible exposure — which is what actually painted the frame
   white. The failure direction was the worst possible one.

**Fixes (`Graphics/Effects/Camera/ToneMapping.cpp`):**

- The adaptation pass validates its measurement against a physical log-luminance window and,
  when it fails, **HOLDS the previous adapted value** instead of mixing it in. The exposure now
  shifts slightly during a glitch instead of being destroyed, and self-heals on the next good
  frame. A poisoned *history* snaps back rather than staying stuck.
- The saturation is a **NaN-deterministic select** instead of `clamp()`: an unusable measurement
  now lands on the ISO **floor** (dark, recoverable, highlights preserved) rather than the ceiling.
- The rejection is **counted and surfaced** (`ToneMapping::meteredRejectedCount()`, shown in the
  Shift+F2 panel) so the guard is a **detector**, not a mask.

> [!CAUTION]
> **Do NOT write this guard with `isnan()` / `isinf()`.** Metal is compiled with fast math, under
> which those intrinsics may be folded away — that is precisely how the NaN survived every
> existing guard on macOS. Use a **RANGE TEST**: every comparison against a NaN is false, so a
> bounds check rejects NaN and Inf *without* depending on finite-math intrinsics, and it
> additionally rejects finite-but-absurd values — which is what sampled corrupt memory actually
> looks like (see the 2320 row above). This applies to every temporal/accumulating buffer in the
> engine, not just this one.

> [!IMPORTANT]
> **A reduction pass must ask for an explicit LOD.** The luminance extract pass minifies (it
> reads a full-resolution HDR buffer into a half-resolution target), so `texture()` derives a
> mip level from derivatives — meaningless for a reduction, and a live hazard on a single-mip
> image. The whole chain now uses `textureLod(..., 0.0)`.

> [!WARNING]
> **This is a MITIGATION, not the root cause.** The `2320` measurement proves the luminance chain
> samples corrupt video memory, and the same corruption shows up as green blocks in the frame
> (reported since the photometry rework). Synchronization Validation is **verified active** — the
> positive control is the still-open `SYNC-HAZARD-WRITE-AFTER-PRESENT` on the screenshot path,
> which it does report — and **clean** on the render path, so the corruption sits below what the
> Vulkan layers can see: MoltenVK/Metal. Hunt it with Metal API Validation
> (`MTL_DEBUG_LAYER=1`, `MTL_SHADER_VALIDATION=1`), `MVK_CONFIG_DEBUG=1`, and an Xcode GPU frame
> capture (Xcode works on Metal, RenderDoc does not). **Do not re-investigate:** resolution and
> the downsample chain (identical at framebuffer 1280x720, the size Windows uses for the same
> window — macOS runs 2x Retina, 2560x1440), the descriptor writes (image/view/**sampler** handles
> and `imageLayout` byte-identical at both write sites), `deltaTime`, the portability subset (it
> IS enabled and its features ARE requested), and explicit-LOD sampling.

> [!IMPORTANT]
> **CONFIRMED MoltenVK-ONLY (Aug 2026).** The rejection counter this fix introduced was read from the
> SAME commit on both platforms: **~2 rejections per second on the Apple M2, exactly 0 on Linux after
> a full rebuild**. The engine's Vulkan usage and CPU-side logic are therefore sound — this is not a
> cross-platform bug wearing a macOS mask. The counter is also the **exit criterion** for the
> remaining work: the corruption is fixed when it stays at 0 on the M2, not when a screenshot happens
> to look clean. See [`docs/troubleshooting.md`](troubleshooting.md) -> "Blocky corruption on macOS".
>
> **CLOSED (Aug 2026):** the root cause the counter was pointing at is found and fixed — the
> post-process chain's reliance on `VK_SUBPASS_EXTERNAL` dependencies alone, a race in MoltenVK's
> inter-encoder translation. See the dedicated entry below ("chained post-process passes relied on
> EXTERNAL subpass dependencies alone"). The counter stays: it is the permanent regression metric
> for the post-process recording path.

**Verified:** `--load-demo=reflexion-debug --demo-options=0,4,0` on an M2 goes from a fully blown
frame with no ISO readout to a correctly exposed one reporting `metered: ISO 199 | scene avg
1902.2 nits`, with `5 metered frame(s) rejected as implausible - held` counted rather than fatal.

**Files:** `Graphics/Effects/Camera/ToneMapping.{hpp,cpp}`, `Core.cpp` (panel readout)

---

### Fixed: the tone mapping never declared requiresHDR — the HDR buffer vanished under it (Aug 2026)

**Symptom (found on macOS, latent everywhere):** un-ticking the LAST enabled effect that happened
to require HDR turns the whole frame into a **uniform grey/white framebuffer**, while HDR (tone
mapping) is still ticked. Which effect triggers it depends on what else is on — it was reported
first as "the bloom does it", then as "no, the motion blur does it", which is the signature of an
aggregate requirement rather than one effect's bug.

**Root cause:** the scene render target's colour format is chosen from the stack's AGGREGATE
requirement (`Renderer::recreateSceneTarget()` via `PostProcessor::cachedRequiresHDR()`), and that
requirement is re-evaluated every time the camera materializes or retires an effect
(`PostProcessStack::syncCameraEffects()`). `ToneMapping` inherited `requiresHDR() == false`, so the
HDR buffer was only ever held up by whichever OTHER effect was enabled — `VeilingGlare`, `MotionBlur`,
`TAA`, `SSR`, `SSGI`, `RTR`, `AtmosphericFog`, `LensFlare` or `VolumetricLight` all declare it.
Turn the last of them off and the scene target drops to the 8-bit swap-chain format underneath a
still-active tone mapper: photometric radiance of a few thousand nits clamps to 1.0 everywhere.

That is exactly the failure the on-demand chain in `Renderer::render()` was written to prevent —
its own comment says *"the raw photometric radiance reached an LDR swap-chain — pure white in
daylight, pure black at night"*. The guard existed; the effect that needs it did not ask.

**Fix:** `ToneMapping::requiresHDR()` returns `true`. Converting linear HDR radiance into a
display-referred image IS what the effect does, so it must pin the requirement itself.

> [!IMPORTANT]
> **Rule for every new effect: `requires*()` describes what YOUR pass reads, and nothing else.**
> Never rely on a sibling effect to hold a resource up for you. The set is aggregated and
> re-evaluated at runtime, so any requirement you leave undeclared is a resource that disappears
> the moment the user unticks something unrelated.

**Verified:** with bloom AND motion blur off, the log still reports
`Scene render target created (2560x1440, format: R16G16B16A16_SFLOAT)` and the frame is correctly
exposed, where it was a flat grey before.

**File:** `Graphics/Effects/Camera/ToneMapping.hpp`

---

### Fixed: shadow maps and render-to-texture kept BY_REGION dependencies (Aug 2026)

`IntermediateRenderTarget::createRenderPass()` carries a long CRITICAL note on why its
write->read dependency must **not** be `VK_DEPENDENCY_BY_REGION_BIT`. Two render passes were
missed when that was fixed, and both are consumed non-locally:

| Render pass | Consumed by | Why by-region is wrong |
|---|---|---|
| `RenderTarget/ShadowMap.hpp` | every lit draw, sampled with **PCF** | a PCF tap reads a NEIGHBOURHOOD, and from arbitrary world positions |
| `RenderTarget/Texture.hpp` | reflections / environment probes | a cubemap is sampled in ARBITRARY directions |

A by-region dependency only orders the *matching* framebuffer region, so a neighbouring tile may
not be written yet when it is read. All four dependencies (both directions, both files) now use
`dependencyFlags = 0`.

> [!WARNING]
> **Metal is genuinely tile-based, so MoltenVK HONOURS the region restriction** where desktop
> drivers quietly widen it. Expected symptom when it bites: oblique blocks of stale frame-N-1
> content in the GPU's tile order, worst during motion. This is why "it works on Windows" proves
> nothing here — the same warning as the swap-chain dependency entry above.
>
> **`reflexion-debug --demo-options=0,4,0` exercises it hard:** option 1 = 4 is
> `CameraContinuous`, which re-renders the probe cubemap **every frame, six faces**.

**Files:** `Graphics/RenderTarget/ShadowMap.hpp`, `Graphics/RenderTarget/Texture.hpp`

---

### Reduction passes must sample with an explicit LOD (Aug 2026)

Generalised from the tone-mapping fix above and now applied to `VeilingGlare` as well (all 26 sample
sites: the 13-tap downsample, the 9-tap upsample tent, the material-properties fetch and the
composite).

Every sample in a post-process reduction chain is a **fixed-level read of a single-mip image**, so
the mip level is never something the shader should be deriving from derivatives. It is not
cosmetic: a downsample pass MINIFIES, which pushes the implicit LOD near 1, and on MoltenVK that
was measured returning **ZERO** on a single-mip image — the defect that made the luminance chain
meter a black scene and pin the exposure to the ISO ceiling.

**Use `textureLod(tex, uv, 0.0)` (or `texelFetch`) in any downsample/upsample/gather pass.** Plain
`texture()` is only appropriate where the read is genuinely 1:1 with the target.

**Files:** `Graphics/Effects/Camera/VeilingGlare.cpp`, `Graphics/Effects/Camera/ToneMapping.cpp`

---

### Fixed: skinned meshes collapsed on ~1/3 of the frames on MoltenVK — the skinning SSBO shared a VMA block (Sep 2026)

> [!CRITICAL]
> **On MoltenVK, a descriptor-referenced host-visible buffer must not share its VkDeviceMemory.**
> A VMA block is one `MTLBuffer`; MoltenVK tracks the Metal argument-buffer residency
> (`useResource:stages:`) per `MTLBuffer` from the stage flags of the LAST descriptor binding that
> referenced it (KhronosGroup/MoltenVK#1870, open). The skinning SSBO (VERTEX|COMPUTE) sharing a
> block with the instance-transforms SSBO (VERTEX) and the RT SSBOs (FRAGMENT) was left
> non-resident for the vertex stage on a random ~1/3 of the frames: every bone read zero, the
> whole Paladin — four Visuals and its cast shadow — collapsed to its origin, and the frame after
> it was fine again. The rate was decided at launch by memory placement (identical launches: 33 %,
> 25 %, 0 %, 17 %), which is why the Fox next to it never showed it and why Linux/Windows never did.
>
> **Fix:** `Buffer::setDedicatedMemory(true)` on the skinning SSBO when the device advertises
> `VK_KHR_portability_subset` (`RenderableInstance::Abstract::createSkinningResources()`): 0 % on
> 4/4 launches against ~2/3 of 22 launches without. **Two rules survive it:** (1) any macOS
> flicker bench needs SEVERAL launches — one clean run is a coin toss; (2) the discriminating
> experiment was `VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT` on every host-visible buffer, NOT
> the driver version (1.4.2 reproduced it), NOT a host/GPU race (`vkDeviceWaitIdle()` before the
> write still reproduced it), NOT the section index. Full story in
> [`docs/troubleshooting.md`](troubleshooting.md) → macOS / MoltenVK.

### Fixed: chained post-process passes relied on EXTERNAL subpass dependencies alone — a race on MoltenVK (Aug 2026)

> [!CRITICAL]
> **A `VK_SUBPASS_EXTERNAL` dependency is NOT a reliable inter-encoder barrier on MoltenVK.**
> The whole post-process chain (motion blur 4 passes, bloom 10, auto-exposure) had zero explicit
> `vkCmdPipelineBarrier` — every write→read ordering between back-to-back render passes rested on
> the IRT's external subpass dependencies. Formally correct Vulkan: core validation AND
> Synchronization Validation are clean, which is precisely why the defect was invisible to every
> Vulkan-level sweep. On Metal each render pass is its own command encoder, and MoltenVK's
> translation of those dependencies left the encoders racing on Apple M2.
>
> **Symptoms (the LAST two classes of the macOS blocky corruption):** displaced 64 px blocks
> around moving objects (motion-blur gather reading a partially-written NeighborMax tile image),
> green blocks (bloom chain reading uninitialized `DONT_CARE` memory), auto-exposure metering
> rejections (~2/s on M2, 0 on Linux, same commit).
>
> **The decisive measurement:** `MTL_DEBUG_LAYER=1` fired **no assertion** yet its serialization
> **suppressed the corruption completely** — a timing race below the Vulkan layers, proven in one
> run. (Metal API Validation validates encoder usage, but its overhead also serializes encoder
> scheduling — remember it as a *suppressor probe*, not only a validator.)
>
> **Fix:** `IndirectPostProcessEffect::recordFullscreenPass()` emits an explicit write→read image
> barrier (no layout change, `SHADER_READ_ONLY → SHADER_READ_ONLY`) after `endRenderPass()`.
> One site covers all 14+ chained passes; redundant and free on conforming desktop drivers.
>
> **Rules that follow:**
> - Any pass recorded outside `recordFullscreenPass()` whose output is sampled by a later pass
>   must emit the same barrier itself.
> - "Synchronization Validation is clean" proves the **Vulkan** level only. On macOS, always add
>   the `MTL_DEBUG_LAYER=1` suppressor probe to the diagnostic matrix: validation-clean +
>   suppressed-by-debug-layer = translation-level race, fix with an explicit barrier.
> - The regression metric is `ToneMapping::meteredRejectedCount()` — it must stay at 0 on the M2
>   after any change to the post-process recording path.
>
> **Verified:** counter pinned at 0 through an active session (camera motion, all effects on,
> no validation layers) where the pre-fix build accumulated 8 rejections within seconds; user
> confirmed zero visual glitches. **File:** `Graphics/IndirectPostProcessEffect.cpp`.

---

### Fixed: probe self-sampling is a GPU FAULT on Apple Silicon — the populate gate now auto-excludes (Aug 2026)

> [!CRITICAL]
> **A draw that samples the image it is being rendered into does not just glitch on macOS — it
> faults the GPU.** `basic-scenery`'s bronze sphere sampled its own environment probe while being
> rendered INTO that probe (the demo never called `excludeFromRendering()`). On Apple M2 the
> feedback draw triggered a GPU error/recovery: macOS discarded every in-flight command buffer
> (`kIOGPUCommandBufferCallbackErrorInnocentVictim` — the innocents die with the offender), MoltenVK
> reported `VK_ERROR_DEVICE_LOST`, and the engine limped through a cascade of secondary failures
> (`vkResetFences` rejected on the in-flight fence — "the fence must be destroyed", every later
> IRT/effect creation failing) into a segfault. On desktop drivers the same feedback loop merely
> reads stale texels, which is why the demo "worked" on Linux/Windows for months.
>
> **Diagnostic signature:** a burst of `VUID-vkCmdDrawIndexed-imageLayout-00344` ("layout
> SHADER_READ_ONLY_OPTIMAL doesn't match previous known layout COLOR_ATTACHMENT_OPTIMAL") on a
> sampler variable, immediately followed by `Lost VkDevice ... (victim of GPU error/recovery)`.
> The named sampler tells you WHICH texture; the layout mismatch tells you it is being sampled
> mid-render-pass. Everything after the device loss is noise — fix the trigger, not the cascade.
>
> **Fix (engine, two layers):**
> - `Material::Interface::samplesTexture(const Vulkan::TextureInterface *)` — a material can now
>   be asked whether any of its components samples a given texture (overridden by the Basic and
>   Standard resources).
> - `Scene::checkRenderableInstanceForRendering()` consults it and **auto-excludes** the instance
>   from Texture/Cubemap render targets its own material samples — no registration needed, the
>   manual `excludeFromRendering()` list remains for other cases. Cost is confined to probe
>   renders by a `renderType()` test.
>
> **Post-device-loss robustness remains an open item**
> ([`docs/todo/post-device-loss-robustness.md`](todo/post-device-loss-robustness.md)): the engine does not
> yet recover or fail-stop cleanly after `VK_ERROR_DEVICE_LOST` — it must not be reachable through
> scene content in the first place.
>
> **Files:** `Scenes/Scene.rendering.cpp`, `Graphics/Material/Interface.hpp`,
> `Graphics/Material/{Basic,Standard}Resource.{hpp,cpp}`. See also
> `docs/reflection-pipeline.md` § 2.3 fix 4.

---

### Fixed: the directional shadow map lacked TRANSFER_DST — the shadow pass had stopped submitting (Aug 2026)

**Third and fourth instance of the same family** as the two entries above (AdaptLum, then
velocity/DoF) — but this one is the mirror image, `TRANSFER_DST` on a *write* rather than
`TRANSFER_SRC` on a readback, and it cost an entire feature rather than a diagnostic.

**Defect 1 — `RenderTarget::ShadowMap::createImages()`.** The depth image was created
`DEPTH_STENCIL_ATTACHMENT | SAMPLED` while the lines directly below it ran
`transitionImageLayout(UNDEFINED → TRANSFER_DST_OPTIMAL)` → `clearDepthImage(1.0F)` →
`transitionImageLayout(TRANSFER_DST_OPTIMAL → DEPTH_STENCIL_READ_ONLY_OPTIMAL)`. The clear was
introduced by `64f71ade`, whose own comment says *"exactly as `DummyShadowTexture` does"* — and
`DummyShadowTexture` **does** declare `TRANSFER_DST_BIT`. The flag simply was not copied over.

The cascade is the part worth remembering, because only the first VUID names the real fault:

1. `VUID-VkImageMemoryBarrier-oldLayout-01213` ×2 — both barriers invalid.
2. `VUID-vkCmdClearDepthStencilImage-pRanges-02659/02660` — the clear itself invalid.
3. The layers therefore never record the transition, so the tracked layout stays `UNDEFINED`.
4. `VUID-vkCmdDraw-None-09600` at submit → `vkQueueSubmit` returns
   `VK_ERROR_VALIDATION_FAILED_EXT` → `Unable to submit command buffer for render target
   'SunLightShadowMapSampler_…'`, **every frame, forever**. The render pass declares
   `initialLayout = DEPTH_STENCIL_READ_ONLY_OPTIMAL`, which can then never be reached.

**Defect 2 — `TransferManager::downloadImage()` had a fallback that could not work.** When the
source lacked `TRANSFER_SRC_BIT` it transitioned that source to `VK_IMAGE_LAYOUT_GENERAL` and
blitted it into a scratch image. `vkCmdBlitImage` requires `TRANSFER_SRC_BIT` on `srcImage`
(VUID-vkCmdBlitImage-srcImage-00219): **a layout grants a layout, never a usage**, so the path
was invalid by construction and only ever produced validation errors. It also captured the
scratch `shared_ptr` in a scope that ended before the submit. Deleted (−130 lines); the function
now refuses the image up front with a trace naming the missing flag, and
`RenderTarget::Texture` declares `TRANSFER_SRC_BIT` unconditionally on its **color** image
(never on depth — `TRANSFER_SRC` is the flag that costs depth-compression metadata on some
architectures, and nothing reads those depth images back).

> [!CAUTION]
> **A layout transition can never substitute for a missing usage flag.** Four occurrences in two
> months across both directions of transfer. The reflex, at the *creation* site: an image that
> will be cleared or copied INTO needs `TRANSFER_DST_BIT`; an image that will be read back or
> copied FROM needs `TRANSFER_SRC_BIT`. Grep the creation site before writing the copy.

> [!WARNING]
> **Every one of these four was silent on the NVIDIA driver.** This one had disabled directional
> shadows outright — the scene simply rendered without them, and nobody noticed until the
> validation layers were switched back on. Corrections landed over a period with the layers off
> are UNVERIFIED, whatever the screenshots showed: turn them on and reload before believing a
> rendering feature still works.

**Verified (Aug 2026, RTX 3070 Ti, Release + `VK_LAYER_KHRONOS_validation`):** `sponza` loads
with **zero** VUIDs and shadows are visibly cast again; `offscreen-rendering` +
`dumpRenderTarget("Security")` writes a correct 1024² PNG with zero VUIDs, exercising the
rewritten direct download path; `Core.RendererService.screenshot()` unaffected.

**Files:** `Graphics/RenderTarget/ShadowMap.hpp`, `Graphics/RenderTarget/Texture.hpp`,
`Vulkan/TransferManager.cpp`

---

### Fixed: the albedo attachment had TWO consumer families and one was forgotten — every metal lost its reflections (Aug 2026)

**Symptom.** Six white metal panels (roughness 0.1 → 0.6) facing an 8000-nit emissive band, RTR
alone in the stack: flat `95/95/95` on every panel, no reflected band at all — while the emitter
itself rendered at 217/255. Same on SSR. Nobody had reported it: on Sponza and the watch the metals
are small and the loss read as "a bit dull".

**Cause.** Commit `7bbb3cd7` (2026-08-29) made the albedo G-buffer attachment carry the DIFFUSE
albedo `baseColor·(1−metalness)·(1−transmission)` so the SSGI/RTGI combines would stop lighting
metals and glass as Lambertian sheets — a correct goal. Its author (this AI) grepped for the GI
combines' identifier (`emAlbedo`), found the two, and wrote "the only consumers are SSGI and RTGI".
The reflections read the SAME attachment under another name (`albedoTex`) as the metal Fresnel F0:
`F0 = mix(0.04, albedo, metalness)`. A metal now read `albedo = 0` → `F0 = 0` → Schlick ≈ 0
head-on → no reflection, on every metal, in every scene, for one day.

**Fix.** Two lanes for two readers: `.rgb` = the BASE colour again (exactly what RTR/SSR always
expected), `.a` = the diffuse weight `(1−metalness)·(1−transmission)` (the lane was written `1.0`
and read by nobody — zero cost, 8-bit linear); the GI combines apply `rgb * a`. Exact for both
families, including a FRACTIONAL metalness (the normals MRT only carries `round(metalness)`).
Contract: `src/Graphics/AGENTS.md` § "The albedo G-buffer: BASE colour in RGB, DIFFUSE WEIGHT in
ALPHA".

**Lessons.**
- ⚠️⚠️ **Before changing what an attachment CARRIES, enumerate its readers by the ATTACHMENT** —
  the binding (`context.albedo`, `requiresAlbedo()`, `needsAlbedo`, the sampler in every effect's
  descriptor layout) — **never by one consumer's variable name.** Two effects reading one image
  under two names is the normal case, not the exception.
- ⚠️ A regression of this kind is invisible on a showcase scene and obvious on a calibration
  scene: the `post-processor-effect-debug` bench (projet-alpha) found it on its FIRST capture.
  Run the bench after any change to a G-buffer contract.
- ⚠️ "Verified by grep" in a doc is a claim about the grep PATTERN. Record the pattern, or the
  list of readers, not the conclusion.

### Fixed: a primary camera on a STATIC ENTITY was overridden by the scene's default camera (Aug 2026)

**Symptom.** A demo declaring a fixed camera (`AbstractDemo::enableFixedCamera()` → `Toolkit::
generatePerspectiveCamera< StaticEntity >(..., primaryDevice = true)`) rendered a white-over-black
frame: the log said `New virtual video device 'FixedCamera_…' available` then `There is no camera
in the scene ! Creating a default camera ...` and connected `DefaultCamera_…` to the swap chain,
leaving `FixedCamera_… -> [NOT_CONNECTED]`. The default camera sits at the origin with the identity
orientation and no HDR: half the frustum inside the floor (black), the rest raw radiance (white).

**Cause.** `Scene::initializeBaseComponents()` decided whether the scene had a camera/microphone by
crawling the NODE TREE only. A component on a static entity was invisible to it, so it created a
`DefaultCamera` — `asPrimary()`, i.e. `PrimaryCameraCreated` fired AFTER the real one and the last
primary wins the video output.

**Fix.** The check inspects the static entities too (under `m_staticEntitiesAccess`). Side fix in the
same function: an early `return true` inside the crawl skipped `setEnvironmentSoundProperties()`
whenever the scene already had both devices — every scene with a player. It now runs unconditionally.

**Lessons.**
- ⚠️ A scene has TWO entity families (nodes, static entities); every scene-wide inspection must
  walk both. `Scene::onNotification` already dispatched both to `checkEntityNotification()` — the
  camera was correctly registered as a video device, then superseded.
- ⚠️ `PrimaryCameraCreated` = "the LAST primary wins". Anything that creates a primary device late
  (a default, a fallback) silently steals the output from what the demo authored.
- ⚠️ The three-frame test that found it: the fixed camera at the ethereal player's exact pose must
  give a bit-identical frame; injected keys (`Core.InputManagerService.keyPress(87, 0)` …) must not
  change it; a console `setPosition/lookAt` round trip must return to it bit-exactly.

### Fixed: the RTR glossy cone was a mip lookup — a kernel that is a texel is a kernel that depends on where the feature falls (Aug 2026)

**Symptom.** "Gros pâtés lumineux" on Sponza's rough surfaces; on the bench, the SAME roughness gave
a reflected band 164 px wide at row 775 and 135 px at row 792 — the band straddled a mip texel
boundary in one case and sat inside a texel in the other. The far/near comparison inverted for that
reason alone. And beyond roughness 0.3 the blur stopped growing: the pyramid had no coarser mip.

**Cause.** `textureLod(pyramid, uv, log2(width))` — the mip whose texel IS the kernel. A box the size
of the kernel, aligned to a grid, is shift-variant with the period of the grid; at LOD 4-5 that is
64-128 px. The uniform (screen-space) cone width of v1 hid this behind a second defect: it ignored
the hit distance, 2-3× too sharp for distant hits and blurring contact reflections that must stay
sharp.

**Fix.** Two steps, each measured on `post-processor-effect-debug`: the cone width per pixel from the
hit distance (v2 — formula right, pyramid now the limit), then a 24-tap Gaussian disk gather on a mip
2× finer than the kernel (v3 — kernel within 10 % of GGX up to r = 0.4, far/near 1.28-1.35 as
physics says, grain 1.5/255, +0.16 ms). Details: `src/Graphics/AGENTS.md` § "RTR glossy cone".

**Lessons.**
- ⚠️⚠️ **A prefiltered-mip lookup is not a filter of the requested width**: sample a mip FINER than
  the kernel and integrate over the kernel yourself. Measure any blur against the SAME feature at two
  grid positions before calling its width a property of the roughness.
- ⚠️ When a gather renormalizes by a gathered weight (confidence), the FINAL blend must use the
  pixel's OWN weight — otherwise every pixel whose kernel overlaps a "no data" neighbour dims.
- ⚠️ A truncated Gaussian is not the Gaussian: cut at 2 σ it loses 12 % of its second moment. Either
  cut wider or say what width you actually mean (FWHM here).
- ⚠️ The measurement instrument has limits too: the panel must be taller than ~4 σ of the widest
  kernel it is meant to read, and the differential LDR protocol runs out of levels on broad low hills.

### Fixed: big dark squares under Sponza's decals with RTGI — a blended quad owned the whole G-buffer albedo (Aug 2026)

**Symptom.** With RTGI on, Sponza's shadowed walls carried large dark rectangles; the ivy read as
a whitish haze. Owner report: "le RTGI seul défonce la scène avec des gros carrés sombres".
Reproduced at the default pose; `--load-demo sponza`, RTGI on/off A/B.

**Cause, three layers.** (1) `dirt_decal` is `alphaMode BLEND`, `baseColorFactor.a 0.35`, and
OMITS `metallicFactor` → glTF default **1.0**: the asset declares its dirt METAL. (2) The two-lane
albedo attachment (`.a` = diffuse weight `1−metalness`) made that a weight of **0**. (3) Translucent
materials wrote every G-buffer attachment with blending DISABLED (the flat-water fix, which the
normals need): the decal quad REPLACED the wall's albedo lanes over its whole extent, transparent
texels included → RTGI × 0 under every quad. The albedo lane was displayed on screen to prove it
(a temporary `em_Color = vec3(albedo.a)`): black rectangles exactly where the decals are.

**Fix.** A blended material writes `a = weight · opacity` and its albedo attachment is
alpha-blended by that lane; normals/matprops keep REPLACE. Two candidates were measured first:
write-mask 0 for blended materials killed the squares but took the ivy's GI away (its leaves are
BLEND too); the opacity-weighted blend keeps both right.

**Lessons.**
- ⚠️⚠️ **glTF defaults are data**: a material that omits `metallicFactor` IS metal (1.0). An asset
  that "looked fine" only did so because nothing honoured its metalness; the moment a path does
  (the diffuse weight), the asset's truth shows. Do not fix it in the loader (the import is the
  identity) — fix the asset, or accept what it says.
- ⚠️⚠️ **A per-attachment write policy is per SEMANTICS, not per pass**: normals want the top-most
  surface (replace), the diffuse albedo wants coverage (blend by opacity). One rule for all MRT
  attachments was wrong twice (alpha-weighted normals = flat water; replaced albedo = dark squares).
- ⚠️ When a value lane doubles as a blend factor (`SRC_ALPHA` reads the attachment's OWN source
  alpha), fold the factor INTO the lane (`weight · opacity`) — the blend then does the right thing
  for both readers.
- ⚠️ Displaying a G-buffer lane as the frame colour is the cheapest instrument there is; it settled
  in one capture what three A/Bs were circling.

**Same root, second face — the "gros pâtés flous partout" of the RTR report.** The decal quads
also REPLACED the wall's material-properties nibbles: reflectivity `max(metalness, 1 − roughness)`
= 1 over every dirt stain (metal by glTF default), and RTR rendered each stain as a blurred mirror
of the courtyard on matte stone. Fixed the same day: a blended material writes `a = step(0.5,
opacity)` into the (otherwise unused, always 1) alpha lane of the material-properties attachment
and that attachment blends by `SRC_ALPHA` — an exact per-fragment replace-or-keep of packed data,
no interpolation, no `discard`. Measured with the nibble displayed as the frame: 0.74 → 0.60 under
the decals, frame share above 0.7 halved (26.9 → 13.2 %); what stays is the asset's metal where
the stain is ≥ 50 % opaque. Normals keep REPLACE by design (top-most surface). Lesson: **a packed
lane cannot be blended, but it can be SELECTED — a binary alpha turns the blend unit into a
per-fragment write mask.**

**Open (todo item)**: blended materials are OPAQUE instances in the TLAS (GI/AO/shadow rays hit
the whole decal quad).

### Fixed: the lens flare shone through walls — the source's visibility was a frustum test, not a depth test (Aug 2026)

**Symptom.** Sponza: ghosts and halo with the sun behind the arcade ("il passe à travers la
géométrie", the owner disabled the effect).

**Cause.** `LensFlare::recordOverlayPasses()` derived `lightOnScreen` from the light's projected
position alone (in the frustum → 1, with an edge fade). Nothing ever asked the depth buffer whether
geometry stood in front of the source.

**Fix.** The ghost + halo pass probes the depth around the projected light (16-tap disk, 1.2 % of
the screen height) and scales the flare by the fraction of taps at the far plane. Details:
`src/Graphics/AGENTS.md` § "LensFlare — the source is probed in the depth buffer".

**Lessons — three traps met while proving it on the bench.**
- ⚠️⚠️ `Component::DirectionalLight::direction()` returns the light's **UBO lane**, filled on the
  first update: read right after creation (a demo's `onBuilding()`/`onEnabled()`) it says (0, 1, 0).
  Own the direction you created the light with, or read the entity's frame.
- ⚠️ `deriveLightingFromSky()` is ASYNCHRONOUS (it waits for the cubemap): at `onBuilding()` and
  `onEnabled()` the LightSet has no sun yet. A demo that needs the sun there creates it by hand on
  the manifest's direction (Sponza does), as the bench does.
- ⚠️ A sky manifest's `Stars[].Direction` may not point at the disc painted in its cubemap
  ("AutumnFieldPureSky": no disc at the manifest's direction on screen; "Forrest": the sun behind
  trees). A flare stimulus needs a disc AT the light's direction — measure, never assume the data.
- ⚠️ The flare's ghosts are copies of the THRESHOLD texture at displaced positions and its halo is
  a 0.6-frame ring around the light: a light at the frame centre puts the ring outside the frame and
  the ghosts over uniform sky, i.e. an invisible flare — aim off-centre to see anything.

### Fixed: `Screen` blending SUBTRACTED the background — an SDR operator on an unclamped HDR buffer (Sep 2026)

**Symptom.** In `game-logic`, the fire sprite rendered as a **black hole with cyan fringes**, but
only over the bright porcelain poussin behind it. A metre lower, over the dark ground, the very same
sprite rendered a correct orange flame. The boundary followed the *background's* silhouette, not the
sprite's — the tell that the destination, not the source, drove the failure.

**Root cause.** `BlendingMode::Screen` installed
`srcColorBlendFactor = ONE` / `dstColorBlendFactor = ONE_MINUS_SRC_COLOR`, i.e.
`result = src + dst·(1 - src)`. That is the SDR screen operator and it is **only defined for a
source inside [0,1]**. The scene attachment is an unclamped `VK_FORMAT_R16G16B16A16_SFLOAT` holding
absolute luminance in nits, and **Vulkan does not clamp blend factors on a floating-point
attachment** (it clamps them only for normalized formats). The sprite declared
`EmissiveStrength: 320`, so the factor became **-319** and the background was subtracted.

**Why it read as a COLOUR bug rather than a black one.** The factor is per channel. Measured on the
sprite's own 4-bit palette, linearised, times 320:

| Palette entry | sRGB | `src` (nits) | `1 - src` |
|---|---|---|---|
| flame edge | (117, 0, 0) | (55.9, 0, 0) | **(-54.9, +1, +1)** |
| flame body | (251, 100, 48) | (309, 41, 9.5) | (-308, -40, -8.5) |

An orange flame has **no blue**, so its blue factor stayed at `+1` while red and green went deeply
negative. Over a bright surface the red channel was annihilated and the background's own green and
blue passed through untouched: 2031 pixels measured at **R=11.4, G=164.2, B=167.2** — a *cyan*
flame — and 52.9 % of the flame body crushed to black. Over a dark background `dst ≈ 0`, the
negative term weighed nothing and the flame looked perfect, which is why this survived for so long.

**The fix.** `BlendingMode::Screen` is **deleted** — from the enum, `to_cstring()`, the
`GraphicsPipeline` switch and the JSON validator. `to_BlendingMode()` maps the legacy `"Screen"`
string to `Add` and `getBlendingModeFromJSON()` warns, so old manifests keep loading while telling
the author to fix them. The five affected sprites (`fire001`, `fire002`, `fireball001`,
`explosion002`, `nuke`) moved to `"Add"`. After: **2** such pixels left in the frame — both on a
genuinely blue material elsewhere — 0 % black, red back to 230, 0 VUID.

> [!CAUTION]
> ⚠️⚠️ **An operator defined over an SDR ratio domain has NO meaning in a nits buffer.** This is
> the general trap, and `Screen` was only its first instance. Audit any blend factor that reads the
> SOURCE COLOUR (`ONE_MINUS_SRC_COLOR`, `SRC_COLOR`, `ONE_MINUS_DST_COLOR`, `DST_COLOR`): those are
> ratios by construction, and this renderer feeds them luminances. `Multiply` (`ZERO`, `SRC_COLOR`)
> is the same family and is still present — it does not flip sign, but `dst·320` blows the
> destination up instead of darkening it, so it belongs on an LDR overlay, never on an emissive
> surface. Factors that read only ALPHA (`SRC_ALPHA`, `ONE_MINUS_SRC_ALPHA`) are safe: alpha stays
> in [0,1].
>
> ⚠️ **Only 2 of the 5 assets using `Screen` were actually broken**, and that is exactly why the
> mode looked healthy. `fireball001`, `fire002` and `nuke` declare no `EmissiveStrength` (so 1.0),
> stayed inside [0,1], and rendered correctly. A mode whose correctness depends on an unrelated
> scalar in the same manifest is not a mode, it is a trap. **"Other assets using it look fine" is
> not evidence that a mode is sound** — check the domain, not the sample.
>
> ⚠️ The three healthy sprites did change appearance slightly on migration: with `src` in [0,1],
> `Screen` is a touch darker than `Add` (`dst·(1-src)` vs `dst`). That is expected, not a
> regression.

**How it was localised without a GPU capture.** The whole diagnosis came from arithmetic on the
asset — `magick identify -verbose` prints a colour-mapped PNG's palette, so `src` was computable
exactly — and the pixel proof was a channel test on a screenshot: `B > R + 30 && G > R + 30`
counted the defective pixels and gave their bounding box. **No orange source and no additive or
alpha blend can produce a pixel at R=11 / B=167.** When a symptom is a colour, one channel
inequality over the frame is a sharper instrument than looking at it.

### Fixed: SSR darkened every participating dielectric — the Fresnel was on the colour, not in the composite weight (Sep 2026)

**Symptom.** On `global-illumination`, the screen-space lane made the floor "mirror the room" while
the ray-traced lane did not, and the owner read it as "the walls reflect the scene in screen-space,
RT is right, the walls are not reflective". The walls were innocent in both lanes: `GI_White` is
roughness 1.0, its reflectivity nibble is `max(metalness, 1 - roughness) = 0`, and both combines
gate on it (measured bit-stable at 191.71 across every screen-space variant). The surface that
moved was the **floor** — `enableBasicGround()` hands out the DEFAULT `StandardResource`
(roughness 0.5, metalness 0), nibble 8/15, roughness fade 1 in both lanes: a strong participant of
both, on a bench described as "white matte floor" (the demo now builds its floor from the walls'
own `GI_White` through `onSetupGroundLevel()`, same day — projet-alpha `src/Builtin/AGENTS.md` § 11).

**Root cause.** Same `mix(scene, data.rgb / confidence, confidence × intensity × reflectivity)`
in both combines, different confidences. RTR: `distFade × fresnel × roughnessFade` with the colour
premultiplied — a dielectric's weight is ≈ 0.04–0.06, so the substitution removes about what the
lobe reflects. SSR (`529b46c4`, Aug 2026, "smooth dielectrics participate"): the Fresnel multiplied
the COLOUR only, the confidence had none, and the output was not premultiplied while the combine
divides by the confidence as if it were. Net per pixel: `(1 - conf·I·R)·diffuse + F·I·R·refl` —
**≈ 43 % of the shading removed, ≈ 2 % of reflection added** on the roughness-0.5 floor. The
resolve's own comment defended it: "the Fresnel rides on the COLOR, never on the confidence — the
division cancels any weight folded into it". The division renormalizes the colour; the mix weight
IS the confidence. RTR proved the correct split all along.

Measured, same pose, same exposure (left wall as the exposure control), Rec.709 mean over the lit
floor crop:

| Lane | Reflections on | Reflections off | Δ |
|---|---|---|---|
| RayTracing (RTR) | 151.8 | 149.2 | **+2.7** |
| ScreenSpace (SSR), before | 109.5 | 129.8 | **−20.3 (−16 %)** |
| ScreenSpace (SSR), after | 131.3 | 129.8 | **+1.5** |

After the fix the SSR footprint over the whole frame is +0.15 of mean luminance with **0.0 %** of
pixels darkened by more than 5 (12.2 % before); walls, ceiling and mirror column are bit-stable
across the two builds, and the ray-traced lane is unchanged to the second decimal (its code did
not move — the regression control).

**The fix.** The SSR resolve computes the primary surface once (depth, normal, packed
roughness/metalness, view position, `F0 = mix(0.04, albedo, metalness)`, Schlick), splits the
Fresnel like RTR — `fresnel = max(F.rgb)` into the confidence, `F / fresnel` on the colour — and
writes the PREMULTIPLIED pair on both paths. The environment miss is now `envFallbackIntensity ×
fresnel × roughnessFade` and its cubemap sample is scaled by `FrameContext::skyLuminance` (new push
constant, block at 92 bytes): it used to inject a [0,1] value into a chain in nits, and to skip the
roughness fade the trace applies — a surface the trace refused to reflect on reflected the sky.

> [!CAUTION]
> ⚠️⚠️ **Two lanes that composite the same concept must weight the mix IDENTICALLY, or the same
> surface reads as two materials — and the wrong lane looks like the "reflective" one.** A
> darkening reads as a reflection when the removed shading is replaced by an image of the room.
> Any change to one composite goes to the other in the same commit; the bench is the default-
> material floor of `global-illumination`, RTR is the reference.
>
> ⚠️⚠️ **A participation mask change exposes every weighting defect downstream.** Before
> `529b46c4` a metalness-0 surface published `metalness × (1 - roughness) = 0` and never reached
> the combine, so the missing Fresnel weight was invisible. Widening who participates is the moment
> to re-measure how much each participant substitutes.
>
> ⚠️ **"The walls reflect" is a pose-dependent human reading — attribute by slot before believing
> the noun.** The attribution that settled it was cheap: one lane, full chain vs `disable(Reflections)`
> vs `disable(IndirectDiffuse)`, a signed per-pixel map (red adds, blue removes) and a per-region
> table with the exposure control included. Walls: zero. Floor: the whole footprint.
>
> ⚠️ A normalized cubemap value in the HDR chain is ALWAYS a defect (the AtmosphericFog rule, again):
> under a 17 000 lx sky the old SSR fallback was ≈ 1 nit, i.e. invisible, so it passed every
> sky-driven scene and only showed as a flat constant in a closed room.

**Verified:** projet-alpha cascade builds (0 warnings), `global-illumination` re-measured at the
same pose — see the table in `docs/reflection-pipeline.md` § 4.3 — 0 VUID.

**Files:** `Graphics/Effects/Lighting/SSR.{hpp,cpp}`. Docs: `src/Graphics/AGENTS.md` § SSR,
`docs/reflection-pipeline.md` § 3.1, 4.3, 4.4, 5.

### Fixed: SSGI kept the linear range fade RTGI had retired, and a 5 m range against 8 (Sep 2026)

**Symptom.** On `global-illumination`, the screen-space lane rendered the shadowed right wall at
**0.0** and the ceiling at 7.5 where the ray-traced lane read **84** and 28; the `IndirectDiffuse`
slot added +1.2 of mean luminance in one lane and +34.1 in the other. Read as "SSGI barely works
indoors".

**Root cause, two halves, neither of them the technique.** The screen-space lane's range default
(then `IndirectDiffuse/ScreenSpace/MaxDistance`) was **5 m** against 8 m for the ray-traced key —
in a 6 m corridor the opposite wall was out of range. And `SSGI.cpp` still faded every bounce **linearly from the surface**
(`1 - hitDist / maxDistance`), halving a bounce found at mid-range, while `RTGI.cpp` had replaced
that curve by a smoothstep over the **last fifth** only (Jul 2026), with a comment measuring the
linear fade at about a factor two of lost indirect energy on Sponza. The correction had landed on
one lane; the A/B then compared two ranges and two attenuations along with two techniques.

**Fix.** Same default (8 m) and the RTGI fade, verbatim, in `SSGI.cpp`. The owner's `settings.json`
carried the persisted 5 (projet-alpha never resets its settings) and was edited to 8. The same day
the owner closed the class of defect: every knob common to both lanes became ONE concept-level key
(`IndirectDiffuse/MaxDistance`, read by SSGI and RTGI alike — `src/Graphics/AGENTS.md` § *The
post-processing settings tree*), so two defaults can no longer drift.

> [!CAUTION]
> ⚠️⚠️ **A correction made to one occupant of a slot is a divergence until it reaches the other.**
> RTGI's fade comment even measured the linear fade *"against the screen-space path"* — the
> screen-space path was the reference for the gap, and kept the defect. When a slot has two lanes,
> the fix's checklist has two files.
>
> ⚠️ **A key with the same meaning and unit in both lanes carries the same default.** Otherwise the
> lane switch is not an A/B of techniques; it is an A/B of settings, and the shorter range reads as
> the weaker technique. (`ContactShadows` already followed this rule; `PixelDoubling` is the
> legitimate exception — it is not the same trade-off in both lanes.)
>
> ⚠️ The residual gap is structural and stays: an off-screen surface carries no light, and SSGI has
> no sky term on purpose. Do not chase it with a sky term or a longer range.

**Verified:** cascade builds, `global-illumination` re-measured at the same pose (numbers in
`src/Graphics/AGENTS.md` § *SSGI / RTGI parity*), 0 VUID.

**Files:** `Graphics/Effects/Lighting/SSGI.cpp`, `SettingKeys.hpp`.

### Fixed: the ray-traced reflections showed a world WITHOUT indirect light — the irradiance probe volume (Sep 2026)

**Symptom (owner report).** On `global-illumination`, the lit green column appeared **black** in the
mirror column under the ray-traced lane, not under the screen-space one. Half of the owner's
hypothesis was right — the omni sits 0.75 m behind the plane of the face the mirror sees, so no
direct light reaches it (`N·L < 0` everywhere on it) — and the other half was the defect: that face
is not black in the direct view, RTGI lights it (RGB (0, 65, 0) with the IndirectDiffuse slot,
(0, 0, 0) without), while the RTR hit shading had **no indirect term at all**: direct light with
shadow rays, the scalar ambient (0 in that box), the sky IBL × sky luminance (0, no sky), emission.
The reflected world was a world without GI; a sky-driven scene hid it behind the IBL term at the hit.

**Fix.** A world-space radiance cache the hits can query wherever they land, on screen or not:
`Graphics::IrradianceProbeVolume`, a DDGI irradiance probe field (camera-centred scrolling grid,
octahedral irradiance E/π and distance-moment atlases, Chebyshev visibility, infinite bounce through
feedback — `src/Graphics/AGENTS.md` § *The irradiance probe volume*). RTR adds
`diffuseAlbedo × probeIrradiance()` at every hit and drops the raster IBL diffuse leg there while
the volume runs (the probes integrate the sky with visibility). Measured, same pose, zero VUID: the
reflected face **0.07 → (0, 214, 0)**, stable over 90 s, the reflected ceiling black → 204; the direct
view unchanged at (0, 64, 0). Cost 0.27 ms on that scene, 0.69 ms on Sponza.

> [!CAUTION]
> ⚠️⚠️ **A screen history cannot light a hit behind the camera.** The RTGI multi-bounce used to
> (`historyFeedback()`, until 2026-09-12) reproject the hit into the previous frame — the face in this
> report is behind the camera and had no history. Only a world-space representation (probes, a hash
> cache, a surface cache) answers there, which is why RTGI now reads the same probe volume at every
> bounce hit (`probeFeedback()`). When a traced effect shades its own hits, ask where its indirect
> term comes from before trusting the reflection of anything lit by bounce light.
>
> ⚠️ **The rule "the reflection must match the raster" has TWO halves**: the shadow rays gave the
> direct half (Jul 2026), the probes give the indirect half. Sky-driven scenes never showed the
> missing half because the IBL diffuse at the hit stood in for it — an unoccluded sky, which is
> also why that leg is now switched off at the hit while the volume runs (it would count the sky
> twice).
>
> ⚠️ Three first-run defects, each with a caution in the AGENTS section: `#version 450` disables
> `GL_EXT_ray_query` in glslang (`'rayQueryEXT' : undeclared identifier`, pointing at a wrong line);
> the fragment-only stage flags of the RT and bindless set layouts refuse a compute pipeline; the
> `writeCombinedImageSampler()` helper writes the image's CURRENT layout (UNDEFINED at creation).
>
> ⚠️⚠️ **Two poses are two exposures: compare estimators at a PINNED exposure only**
> (`Act.setExposure(aperture, shutterSeconds, iso)`). The first reading of this face — "mirror 214 vs
> direct 64, 107 vs 64 at one bounce, the probes 1.7× RTGI" — came from two auto-exposed frames and
> its single-bounce half had the wrong SIGN. At f/2.8 · 1/39 s · ISO 100 (2026-09-12, G channel):
> probe-fed RTGI **135** vs mirror **118**; RTGI single bounce with the probes recursive (the
> screen-history era, whose feedback added nothing off screen) 38 vs 116; everything single bounce
> 38 vs 30. RTGI's multi-bounce reads the probes since that day (`probeFeedback()` in place of
> `historyFeedback()`, `MultiBounce/Clamp` deleted): the 3× gap is closed, the two estimators agree
> within ~15 % with the traced side brighter (expected — one exact bounce refines an interpolated
> cache), and the probes sit ~20 % BELOW RTGI at one bounce. Residual ACCEPTED (owner decision
> 2026-09-12) as the price of a cache: no calibration, never scale either estimator to the other.

**Verified:** cascade builds (0 warnings), `global-illumination` measured at three poses and over 90 s,
Sponza 0 VUID, emeraude-base 2049/2049, GPU cost in the AGENTS section.

**Files:** `Graphics/IrradianceProbeVolume.{hpp,cpp}`, `Graphics/Effects/Shared/IrradianceProbesGLSL.hpp`,
`Graphics/Renderer.{hpp,cpp}`, `Graphics/BindlessTextureManager.cpp`, `Graphics/Effects/Lighting/RTR.cpp`,
`SettingKeys.hpp`.

### Fixed: a PARKED camera shimmered under TAA — the colour clip rejected valid history every frame (Sep 2026)

- **Symptom** (owner, `relief`): the far ground shimmers on a still camera on Linux, not on Windows. The
  Windows machine had `Core/Graphics/PostProcessing/TemporalAA/Enabled = false` (settings diff): no
  TAA, no jitter, no shimmer. Measured with `temporalCapture(16)` + `tools/temporal-analysis.py`: far
  band peak-to-peak 21.2/255, 61 % of its pixels > 8; with the jitter off, 0.3. NOT the lighting lane
  (RT 21.0 vs SS 21.2), NOT the resolution (1280×720 the same), NOT the velocity (probed exactly 0).
- **Cause 1**: the variance clip compared the history, the filtered accumulation of every jitter
  phase, with the RAW jittered 3×3 of the frame. On sub-pixel detail the two differ by construction,
  so the valid history was thrown away every frame.
- **Cause 2**: the Karis weights were fed NITS. `1/(1+L)` is `1/L` then, and a dark history texel
  sticks (dark seams appeared on the clouds once the clip was relaxed).
- **Fix**: the depth decides (`src/Graphics/AGENTS.md` § The TAA resolve): the history keeps its
  linear depth in alpha and is clipped only where that surface left the 3×3 depth range; the
  weights use the exposed luminance. Result 0.01 % of pixels > 8 (from 9.5 %), max 12 (from 163).
- ⚠️ A settings DIFF comes first when two machines disagree: the "Windows does not shimmer" lead was
  a disabled TAA, and an hour of GPU/driver hypotheses would have been built on it.

### Fixed: disabling TAA by selection left the projection JITTERING with nothing to resolve it (Sep 2026)

- **Symptom**: `Core.SceneManagerService.PostProcess.disable(TemporalAA)` on a parked camera, and
  the whole frame shimmers — the brick cube of `light-and-shadow-debug` moving on **64 %** of its
  pixels (peak-to-peak > 4/255 over 8 frames), the walls speckling.
- **Cause**: `PostProcessStack::requiresJitter()` asked every occupant of `m_orderedEffects` — the
  flattened slot table, which holds every RESIDENT occupant, selected or not — so a TAA that was
  filed but disabled still answered "yes", and `Renderer::prepareFrameJitter()` kept advancing the
  Halton sequence.
- **Fix**: the predicate also requires `isEnabled()`. Measured after: the direct cube 7.66 → **0.176**
  (the RT effects' own floor), the walls and the mirror floor **0.000**.
- ⚠️ **Methodology consequence**: any "TAA off" series taken through the console before 2026-09-13
  measured the RAW jittered frame, not a jitter-free one. The protocol in projet-alpha
  `docs/temporal-stability-measurement.md` §1 relied on a launch-time setting for that reason; the
  console switch is now equivalent.

### Fixed: the RTR trace sampled its hit textures at an UNDEFINED LOD — the shimmer in every textured reflection (Sep 2026)

- **Symptom** (owner, 2026-09-13, and already on Sponza 2026-08-30): a strong *fourmillement* on
  every surface receiving ray-traced reflections, with TAA on.
- **Measured first** (`light-and-shadow-debug --demo-options 0,1`, mirror floor, parked camera, RT
  lane, 8-frame peak-to-peak of 8-bit luma): the mirror floor reflecting the dark wall is stable
  at **0.004** in every configuration; the reflection of the static brick cube moves **0.65** mean,
  p99.9 **53**, against **0.008** with reflections off — ×80. With the jitter truly off the same
  reflection sits at **0.052**: the instability is 100 % jitter-driven. Its energy was on the
  mortar lines and the silhouette (correlation with the image gradient: displacement, not noise).
- **Cause 1, fixed**: `texture()` on the bindless hit textures inside divergent control flow, with a
  `hitUV` that is not continuous across a quad — implicit derivatives undefined, finest mip taken,
  minified textures point-sampled. Now `textureLod()` with a ray-cone footprint (Ray Tracing Gems
  ch. 20; see `docs/reflection-pipeline.md` § 3.2). Crop mean **0.65 → 0.44**, p99.9 **53 → 32**,
  interior of the reflected cube 0.131 with no pixel above 4.
- **Cause 2, fixed**: the half-res trace unprojected the half-res pixel centre with the depth of a
  full-res texel half a pixel away — a ray-origin bias along the view ray. Now the fetched texel's
  own NDC. ⚠️ Same pattern still in `RTAO.cpp` and `SSR.cpp` (`docs/todo/`).
- **Cause 3, fixed the same day (owner decision)**: what remained (75 % of the residual energy)
  was the reflected SILHOUETTE — the geometric edge aliasing of a half-res, one-sample trace under
  a half-pixel jitter, which TAA integrates for the direct image but only partly for a reflection
  it cannot reproject (the reflected content does not move with the reflector's velocity). No
  texture filter touches it. The RTR now accumulates its raw trace through the shared `GIDenoiser`
  in REFLECTION mode — history reprojected through the VIRTUAL point `P + V·hitT`, blended toward
  the surface path with the roughness, validated on the virtual distance, variance-clipped —
  before the blur and the pyramid. Crop mean **0.44 → 0.177**, p99.9 **32 → 2.0**, no pixel above
  16; one frame after a 0.3 m camera jump the reflection already sits where the new pose puts it
  (0.02 % of the region > 16). ⚠️ Never reproject a REFLECTION through the surface velocity
  alone — a mirror reflecting a static wall smears under camera motion when its history follows
  the floor; the virtual path is what makes the history land. Details: `docs/reflection-pipeline.md` § 3.2, `src/Graphics/AGENTS.md` § GIDenoiser.
- ⚠️ **Protocol lessons**: (1) `light-and-shadow-debug` carries ANIMATED content (the sprite, the
  palm) — a whole-frame or whole-floor statistic mixes their legitimate motion into the reading;
  measure on crops of a STATIC reflected object and check the crop with reflections off reads ~0.
  (2) Compare the reflection of an object with the object's own direct rendering under the same
  TAA: the DIRECT cube integrates to 0.52 with 0.09 % of pixels > 16, the REFLECTED one to 0.44
  with 0.64 % > 16 — the mean can look equal while the tail, which the eye reads as shimmer, is
  seven times heavier.

### Fixed (guard) / OPEN (root): 7x7 black squares on every glossy surface of the screen-space lane — a NaN in the colour buffer under an animated BLENDED sprite (Sep 2026)

- **Symptom** (owner, `light-and-shadow-debug` with the mirror floor, 2026-09-13): sharp, fully black
  **7x7** squares on the sphere and the palm trunk in SSR mode, at FIXED positions, appearing at a
  CONSTANT period; none on the ray-traced lane. Disabling SSR alone removed them; disabling SSGI,
  SSAO or the contact shadows did not. Commenting out the animated `pinup` sprite removed them too.
- **Chain, measured backwards from the shape**: a single NON-FINITE texel of the SSR resolve is
  spread by the separable bilateral blur (radius 2 on those roughnesses → **5x5**, a NaN passes
  every weight), mixed by the combine, then widened by one pixel on each side by the TAA's 3x3
  variance-clip statistics (→ **7x7**), and tone-mapped to black. The texel is non-finite because
  the SSR resolve read the scene colour at a hit on the sprite's quad — `pinup.json` is
  `BlendingMode: Normal` + `Lit: true`, a blended AND lit quad whose fully transparent texels still
  run the lighting pass and the blender: `dst · 1 + NaN · 0 = NaN`, so the colour buffer holds NaN
  wherever a transparent texel's lighting produced one. The animation frames move those texels —
  the period. RTR never sees them: it alpha-tests the sprite at the hit and shades the hit itself.
- **Guard (in place)**: `SSR.cpp`, the resolve rejects a non-finite reflected colour as a miss. A
  filter must never ingest NaN/Inf; this is the filter's own protection. ⚠️ **Its role in the
  disappearance is NOT proven**: after the owner invalidated the build and every cache, a clean
  rebuild with all the day's fixes AND the probes showed no square of any colour, sprite present —
  the probe would have painted any non-finite value reaching the resolve. Either one of the
  committed fixes removed the cause (the transfer → compute barrier is the only one that repairs a
  timing-dependent race, which fits a session-dependent symptom), or some intermediate CLion
  incremental rebuilds were stale and their verdicts wrong. Undecidable without a reproduction.
- **ROOT, OPEN — not reproducible since the clean rebuild**: `docs/todo/blended-lit-sprite-writes-nan.md`
  keeps the facts and the hypotheses (sprite lighting NaN; the `CutFrameAroundTranslucency` path,
  whose region a translucent's per-frame extent could drive). **The instrument is permanent**:
  `Core/Graphics/PostProcessing/DebugNonFinite = true` (relaunch) makes the SSR resolve paint WHICH
  input is non-finite — red = grabbed colour, blue = colour pyramid, green = trace data, magenta = the
  mix only — and the TAA paint in red any pixel whose 3x3 reconstruction holds one. The day the
  squares return: one key, one look, the guilty buffer.
- ⚠️ **Attribution lessons**: (1) this was NOT caused by the day's RTR/denoiser work — an A/B by
  `git stash` of the four files, rebuilt and looked at by the owner, kept the squares; three
  plausible mechanisms (a NaN-unsafe history test, a missing compute stage in the grab barrier, an
  fp16 overflow of the mirror's specular peak) were each fixed or tested and were NOT it. Only the
  live per-effect bisection on the owner's instance (`disable(<slot>)` one at a time) and the
  owner's own sprite removal named the carrier and the trigger. (2) The AI never reproduced it in
  320 captured frames: a NaN written through the blender depends on content the AI's runs did not
  have in the same state — do not read "not reproduced on my side" as "not there". (3) The 7x7
  measurement was worth more than any hypothesis: it is the arithmetic of two filters, and it ruled
  out the 8x8 compute-workgroup reading and the mip-texel reading.

### Fixed: the grab-pass publication barrier omitted the COMPUTE stage (Sep 2026)

- `PostProcessor.cpp` and `GrabPass.cpp` published the grabbed copies with
  `TRANSFER → FRAGMENT_SHADER | COLOR_ATTACHMENT_OUTPUT` while SSR reads the grabbed depth (Hi-Z
  copy) and colour (colour pyramid) in COMPUTE shaders — a transfer → compute hazard with no
  synchronisation. Fixed by adding `VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT` to both destination masks.
  ⚠️ It was suspected of the black squares above and it was NOT them (owner-tested); it stays as a
  correctness fix against the specification, with no measured visual effect.

### Fixed: a degenerate hit normal became a NaN shadow-ray origin — DEVICE LOST scaling with the light count (Sep 2026)

**Symptom:** Intel's Sponza lit by its own 22 wall lamps lost the device at the first frames
(`VK_ERROR_DEVICE_LOST` on the shadow-map submit, a `[device_fault]` block of
`INSTRUCTION_POINTER_UNKNOWN` addresses, no VUID); with 14 lamps it survived one run out of two,
with none it never failed. The frames that did render under the ray-traced lane were near black.

**Cause, named by GPU-assisted validation** (`VK_KHRONOS_VALIDATION_VALIDATE_GPU_BASED=GPU_BASED_GPU_ASSISTED`
on the launch, the only instrument that saw it): `VUID-RuntimeSpirv-OpRayQueryInitializeKHR-06351`,
*OpRayQueryInitializeKHR operand Ray Origin contains a NaN*, in the RTGI and RTR fragment shaders
and the probe-volume compute. The hit normal interpolated from the vertex buffer was degenerate on
some hits (`normalize(0)` = NaN), the shadow-ray origin `hitPos + hitNormal * bias` inherited it,
and every light in the loop traced one illegal query per bad hit — 22 lamps turned one bad hit
into 22 faults, which is why the count "mattered" without being the cause. NVIDIA answers a NaN
ray query with a device loss; the denoiser spread the NaN of the surviving frames into darkness.

**Fix (RTGI, RTR, `IrradianceProbeVolume`):** `getHitNormal()` returns `vec3(0.0)` on a degenerate
interpolation, the caller resolves a zero world normal to the ray-facing direction, and
`shadowRayVisibility()` returns *unoccluded* for a non-finite origin or direction. **Rules:** a ray
query operand is checked finite BEFORE `rayQueryInitializeEXT()`, always; a `normalize()` of data read
from a buffer is guarded; a device loss "that depends on the number of lights" is a per-light fault
multiplied, not a capacity — run GPU-AV before bisecting counts.

### An asset's lights can arrive with ZERO intensity — and one exporter zeroes them all (Sep 2026)

Intel's Sponza 2022 glTF (3ds Max 2024, Babylon.js exporter) declares 24 `KHR_lights_punctual`
lights with `intensity: 0.0` — the sun, the sky placeholder and the 22 lamps. Positions, colours
and the sun's orientation are exact, the photometry is gone, and no other format of the package
carries it (USD: no light prim; FBX: `Null` nodes). `SceneDataConsumer` now refuses a light without
energy instead of building a 0 cd point light with an UNBOUNDED culling radius (a null radius means
"reach everything", see `PointLight::touch()`). The values were written INTO the GLB with
`tools/gltf-lights.py` (100 klx for the sun, 100 cd per lamp — owner decision 2026-09-13); the
asset stays the authority, the demo writes no light.

### The Intel Sponza normal maps were DirectX-convention — the engine decodes Khronos (Sep 2026)

**Symptom (owner):** every mortar joint of Sponza's walls shaded as if lit from BELOW.
**Not** the tangent frame, **not** the Y-up flip, **not** the irradiance orientation (verified on
`coordinates-debug` under `AxisDebug`: the ground is tinted by the +Y face). **Measured on
`normal-map-debug`** (the quad under the TOP omni, exposure pinned): Sponza's `brickwall_01` as
shipped puts the bright lip of every groove ABOVE it (107 vs 102 below); with the green channel
flipped, BELOW (106 vs 103); the store's Khronos-standardised `Walls/Bricks001-normal`, below. A
curl test on the source PNGs (`corr(∂R/∂v, ∂G/∂u)`) has the opposite sign on Intel's files and
on the standardised store. **Cause:** 3ds Max bakes +Y down (D3D texture origin), the Babylon
exporter did not convert, and glTF mandates +Y up. **Fix in the DATA:** the 27 normal-map images
of `Sponza.ktx2.glb` were re-encoded from Intel's PNGs with the green channel inverted, same
recipe as the original (`ktx create v4.4.2`, UASTC quality 2, RDO λ 0.75, zstd 18, linear,
13 mips, RGB), the GLB repacked without touching anything else. **Rule:** a normal map carries no
metadata about its convention; when relief looks lit from below, put the texture on
`normal-map-debug` (option 1-3 Sponza, 4 = the Khronos control) before touching the engine.

### Fixed: the reflection effects weighted the environment with a raw FRESNEL instead of the environment BRDF (Sep 2026)

Owner-reported: *"un blanc dégueulasse qui entoure toutes les aiguilles du cyprès"*. Sponza's cypress
rendered grey-white where it must be green, and switching the Reflections slot off made it green
again — in BOTH lanes, which is what proved the cause was the WEIGHT given to the reflection, not how
it is gathered.

**Root**: `RTR.cpp` and `SSR.cpp` weighted their composite with a raw Schlick Fresnel where the
split-sum requires the ENVIRONMENT BRDF — `F0 * A + B`, the integral of the specular BRDF over the
lobe (Karis, *Real Shading in Unreal Engine 4*, 2013). Schlick sends F to **1.0**
at grazing incidence. The alpha mask's EDGE texels carry an AUTHORED near-tangent normal (measured on
`Cypress_LF01_Spring_Normal.png`: mean Z **+0.272** at the edge against **+0.676** on the needle body,
and +0.980 under the transparent texels — a blend of the latter two cannot produce 0.272), so
`NdotV → 0` on every silhouette. A dielectric's F0 is a colourless 0.04, so `fresnelTint` is WHITE and
the reflection carries none of the leaf's green. Fixed with Lagarde's roughness-bounded form
(*Moving Frostbite to PBR*, 2014).

⚠️ **Environment term only** — the direct-lighting Fresnel uses `dot(H,V)` and must keep plain Schlick.

**Measured**, share of foliage pixels bright AND desaturated (the white rim), floor 0.9 %:
RT **71.15 % → 2.17 %**, SS **16.42 % → 2.98 %**; foliage saturation RT 8.45 % → 30.92 %.

⚠️⚠️ **It is not "reflections off"**, and that is the test that separates a correct re-weighting from
a lever: the stone KEEPS its reflection (34.39 against 28.15 with the slot off) while the foliage
stops being flooded. A smooth metal is untouched by construction (F0 = 1, roughness 0 returns 1.000).

⚠️⚠️ **The raster IBL path was already correct** — it goes through `IBLTexture::Role::BRDFLut`. Only
the reflection effects were not, so the raster and the reflections computed the same quantity two
different ways on the same surface. **When a value looks wrong, check whether another path in the
same engine already computes it correctly before deriving a correction for it** — three
physically-founded fixes in a row (roughness-bounded Fresnel, geometric specular antialiasing,
traced specular occlusion) each moved the number, 71 % → 51 % → 40 % → 28 %, while the real defect
was a MISSING TERM rather than a mis-tuned one.

⚠️ **A blunt global lever looks exactly like a fix.** Moving `roughnessFade` to
`smoothstep(0.2, 0.6, roughness)` dropped the rim from 51 % to 2.09 % and was confirmed by eye — while
dimming the reflection of every material under 0.6 roughness in both lanes (stone control's saturation
+27 %). Reverted; its only value was diagnostic.

### The application SAVES its settings on exit — restore a debug key AFTER the process is gone (Sep 2026)

`Core.shutdown()` writes the live settings back to `~/.config/LNIsle/projet-alpha/settings.json`.
Restoring a temporarily-changed key while the instance is still running is undone by its own exit:
`MipMappingLevels` and `DebugMaterialPropertiesLane` were each restored, then silently rewritten to
the experimental value by the shutdown that followed. **Stop the process first, restore second, and
verify by re-reading the file.**

### Overriding a private virtual that owns the GPU creation leaves the window BLACK (Sep 2026)

`Material::Interface::onDependenciesLoaded()` is private *because it IS the material's GPU creation*.
An override added on `StandardResource` to adjust flags replaced it without chaining: 133 materials
never created, 14 986 renderables failing with *"The PBR material '...' is not created ! It can't
configure the light generator."*, and a black window. The fix is the contract, not the call: a
protected `onBeforeCreation()` hook that `Interface` invokes before `create()`, which derived classes
override instead. **Before overriding a resource hook, check every class BETWEEN yours and the one
that declares it** — `StandardResource.hpp` carried no override, the base two levels up did.

⚠️ Disabling mipmapping globally (`Core/Graphics/Texture/MipMappingLevels = 1`) is a decisive
DIAGNOSTIC and never a measurement: it also sharpens every normal map, and the stone control moved
27.57 → 18.89 with its saturation 12 % → 20 %. When the control moves, read the image, not the table.

### ⚠⚠ A sub-geometry range indexes the INDEX buffer — the instanced bind used it as a VERTEX-buffer BYTE offset (Sep 2026, FIXED)

`Geometry::Interface::subGeometryRange(i)` returns `{firstIndex, indexCount}`. The engine says so
wherever it reads it (`Graphics/Geometry/Interface.cpp:246`,
`Graphics/RenderableInstance/Abstract.cpp:394`), and `buildSubGeometries()` builds it from
`group.first * 3` — triangles times three, i.e. **indices**.

`CommandBuffer::bind(geometry, modelVBO, subGeometryIndex, modelVBOOffset)` — the **instanced**
overload — passed `range[0]` straight into `vkCmdBindVertexBuffers` as the **byte offset of the
vertex buffer**. Three things were wrong at once: the wrong buffer, the wrong unit, and applied
twice, because `CommandBuffer::draw(geometry, subGeometryIndex, instanceCount)` already passes
`range[0]` as `vkCmdDrawIndexed`'s `firstIndex` and `range[1]` as its `indexCount`.

**Only INSTANCED, MULTI-SUB-GEOMETRY renderables were affected.** The non-instanced sibling
(`bind(geometry, subGeometryIndex)`) has always bound 0 and ignores its `subGeometryIndex` — which
is why `Component::Visual` content was fine and nothing in the cascade had ever hit this: the first
instanced multi-layer renderable was a procedural tree (bark layer + foliage layer) planted with
`Component::MultipleVisuals`.

**Symptom**: the geometry silently does not draw, and the validation layers name it exactly:

    vkCmdDrawIndexed(): Format VK_FORMAT_R32G32B32_SFLOAT has an alignment of 4 but the alignment
    of attribAddress (477) is not aligned in pVertexAttributeDescriptions[0] (binding=0 location=0)
    where attribAddress = vertex buffer offset (405) + binding stride (72) + attribute offset (0)

⚠️ **Read the OFFSET in that message, not the format.** `405` is neither 4-aligned nor a multiple
of the 72-byte stride — no legitimate vertex-buffer offset ever looks like that, and its value is a
first-INDEX. A VUID about attribute alignment is naming a bad **binding offset**, not a bad vertex
layout.

⚠⚠ **This is why the validation layers stay on.** With them off, 240 trees are simply absent and
every plausible explanation — culling, level of detail, the camera, a material — costs an hour
each. See `docs/todo/vegetation-octahedral-imposter-atlas.md` for the demo that found it
(projet-alpha `forest`).

Fix: `src/Vulkan/CommandBuffer.cpp`, the instanced overload binds the geometry VBO at 0 and now
ignores its `subGeometryIndex`, like its non-instanced sibling.

### ⚠⚠⚠ One bind call, two identities — the state tracker followed only one (Sep 2026, FIXED)

`RenderableInstance::Abstract` skips a geometry bind when the state tracker says it is redundant
(`Abstract.cpp`, *"Bind geometry (VBO/IBO) only if it changed"*). The call it skips is
`bindInstanceModelLayer()`, and for an **instanced** renderable that call binds **two** vertex
buffers: the geometry's, shared by every instance of a renderable, and the instance's own **model
matrix buffer**, which is not shared at all. The tracker compared the geometry and the layer index
only.

The lighted render list is **state-sorted**, so the instances of one renderable arrive adjacent:
every one after the first had its bind elided and drew **with the first one's matrices**, landing
exactly on top of it.

**Measured** on the projet-alpha `forest` demo, counted from directly above the whole terrain:

| | entities | components | groves on screen |
|---|---|---|---|
| before | 44 | 132 | **4** (= 12 renderables / 3 per grove) |
| after | 44 | 132 | **44** |

⚠⚠ **It failed in complete silence.** Every entity was in the rendering octree, every component
was listed on its entity, no instance was broken, and not one error or warning was logged. The
scene graph was right and the frame was not, which sends you looking at culling, level of detail
and the camera — an hour each.

⚠️ **This is the SECOND time this exact class of bug has been fixed in that struct**, and the first
one is commented three lines above the new field: every light of a scene shares ONE descriptor set
and only the dynamic offset distinguishes them, so deduplicating on the handle alone made a single
light light the whole scene. **When one bind call carries two identities, tracking one of them
silently reuses the other.** Before adding anything to `RenderStateTracker`, ask what else rides on
the same call.

Fix: `RenderStateTracker::lastInstanceModelBuffer`, fed by a new virtual
`RenderableInstance::Abstract::instanceModelBufferHandle()` — `VK_NULL_HANDLE` by default, the
model VBO in `Multiple`. The non-instanced path binds no such buffer, so its redundant-bind
elimination is untouched.

### ⚠⚠ The shadow pass is recorded BEFORE every `prepareRender()` of the frame (Sep 2026, FIXED)

`Renderer::renderShadowMaps()` runs right after `Scene::updateVideoMemory()` and **before** any
`Scene::prepareRender()` — the main view's and the render-to-textures' alike. Anything a frame
stages inside `prepareRender()` therefore reaches the shadow pass **one frame late**, and anything
the shadow pass reads from a member written there belongs to the PREVIOUS frame.

That is what happened to the vegetation wind. `Scene::castShadows()` bound
`m_preparedInstanceTransformsDS`, written in `prepareRender()`, so at shadow time it held the
descriptor set captured on the previous frame: **another frame-in-flight slot**, outside this
pass's fence, carrying the previous frame's wind. A tree and its shadow swayed one frame apart, and
on the very first frame the member was still null and every vegetation shadow draw was skipped —
78 of the demo's 132 instanced components logging *"the sealed pipeline layout declares the
'PerSceneTransforms' set, but the renderable instance cannot provide it"*.

⚠️ **That error message is honest but its shape misleads.** It names a descriptor-set contract and
invites a search in `Saphir::Generator`; the divergence was neither in the generator nor in the
binding condition, it was in **when** the set became available. An item opened against it was
titled *"instanced vegetation casts no shadow"* and was **wrong**: the shadows were cast, from
frame 2 onward, from a stale buffer. Measured before believing it — a top-down capture shows the
ground shadow offset from each canopy in the sun's direction, which ambient occlusion (centred
under the canopy) never does.

**Fix**: the wind is staged and uploaded in `Scene::updateVideoMemory()`, which runs immediately
before the shadow pass; `Scene::castShadows()` fetches the CURRENT frame's descriptor set instead
of the prepared member. The previous view-projection half of the same header stays in
`prepareRender()` behind its primary-view guard — it needs a render target, the wind does not. The
`InstanceTransforms` upload is cumulative, so the later one still carries both.

⚠️ **Rule**: a per-frame value the SHADOW pass must see is staged in `updateVideoMemory()`, never
in `prepareRender()`. Same family as the `[ONE FRAME, ONE TRUTH]` latch in `prepareRender()`: a
frame must read one state, and a pass must read its own slot.

### ⚠⚠ A SPARSE cutout mask vanishes at distance under a FIXED threshold — whatever the mips do (Sep 2026, FIXED)

Symptom (owner, forest): far pines were bare trunks, full crowns up close; only the conifers. It is not
the LOD — forcing every tree to LOD 0 changed nothing — and not the geometry (the side opacity of the
crown is the same at every level). The needle card is 23 % opaque; a box-filtered mip of it averages to
0.23, a fixed 0.5 threshold drops the card whole. Castaño's coverage-preserving mips do not rescue it: a
2×1 or 1×1 level can only express 0 or 1, and 0 is the nearest to 23 %.

Fix: the HASHED cutout (`StandardResource::enableHashedAlphaTest()`, Wyman & McGuire 2017), on the mean
mips a box filter already gives. Two traps met on the way:

- ⚠️ Blaming the TAA, then the mips, then the preset density — each was measured and cleared before the
  real cause. **Force the suspected variable (LOD 0 here) and photograph at the owner's pose** before
  touching anything; the density change I made first made the pines barer, not fuller.
- ⚠️ A hashed threshold anchored on the WORLD position crawls under the wind: anchor it on the REST
  position (`svRestPositionModelSpace`). A per-primitive anchor (`gl_PrimitiveID`) is out: it needs the
  geometry-shader feature, which MoltenVK lacks.

Details and numbers: `src/Graphics/AGENTS.md` § Alpha Test and § Alpha COVERAGE.

### ⚠⚠⚠ The BC7 mip chain lost the MEAN of every texture — a bilinear resample is not a mip filter (Sep 2026, FIXED)

Symptom: distant pines rendered as bare trunks (owner, `forest` and `terrain`), and their imposter atlas baked bare
(1.9 % coverage). Not the LOD, not the alpha test, not the cache: the conifer with the BROADLEAF's textures was full
at 380 m, so it was the texture. `TextureCompressor` built each level with `Processor::resize(Linear)`, a display
resize that reads two pixels around ONE point per destination pixel, at x·(w−1)/W from the top-left corner — a 1 × 1
target is the corner pixel. Measured on the 23 % needle mask (1022 × 2048): mean 0.35 at 3 × 8, **0.001** at 1 × 4;
the broadleaf's mask (1024²) went 0.38 → 0.118 → 0.000. Every card small enough to sample the tail vanished, and
every texture of the engine had a wrong mip tail. Fix: emeraude-base `Processor::downsample()` (exact area-weighted
box filter, odd sizes included), `generateMip()` uses it, `TextureCache::Version` 3.
⚠️ Measure a mip chain OFFLINE on the real image before suspecting the shader: a 20-line probe gave the answer the
screenshots could not.

### ⚠⚠⚠ Holding a render-target list lock across its callback DEADLOCKED the logic and render threads (Sep 2026, FIXED)

Symptom (owner): `terrain` with its forest ran smoothly for about a second, then froze for good — the console
silent, no imposter atlas baked; the same forest without imposters (option 5 = 1) never froze. Thread stacks (gdb):
the LOGIC thread, updating the sun node under the node lock, reached `SkyFollowsSun` → `refreshAmbientLightProperties()`
→ `forEachRenderToTexture()` and waited for the texture-target list lock; the RENDER thread held that very lock in
`renderRenderToTextures()` → `prepareRender(imposterBake)` → `populateRenderLists()` and waited for the node lock.
Latent since the helpers existed: it needs a render-to-texture target that is PREPARED (the list was empty in
`terrain` until the imposter bake) and a logic-side caller of the list running under the node lock (an animated sun
refreshes the ambient every tick — a frozen sun almost never, which is why runs at `--demo-options 100000,25` passed).
Fix: `forEachRenderToShadowMap/Texture/View()` now SNAPSHOT the list under the lock and process it without
(`Scene::snapshotRenderTargets()`). Rule: never hold a container lock while calling back into code that takes other
locks — the `.claude/rules` "DEFER" rule, applied to locks.

### ⚠️ The imposter bake needs the COARSEST LOD and a TRANSPARENT clear (Sep 2026)

Two faults of the first bake, both silent: the atlas was 100 % covered (the renderer's opaque clear colour was kept,
`ImposterBake` now overrides it with transparent black), and the aspen and pine views were nearly empty (baked at
LOD 0, whose cards fall under a pixel of a 128 px view; the camera now stands at 280 m so `selectLODLevel()` picks the
coarsest level — the one the imposter replaces). Graphics `AGENTS.md` § 15e.

## Related Documentation

- `@AGENTS.md` - Engine root context
- `@src/PlatformSpecific/AGENTS.md` - Platform-specific system details
- `@src/Graphics/AGENTS.md` - Graphics system
- `@src/Saphir/AGENTS.md` - Shader generation system
