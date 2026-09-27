## 5. Material UBO System (SharedUniformBuffer)

### Architecture

Materials use a **SharedUniformBuffer** for GPU-side property storage:

```
┌─────────────────────────────────────────────────────────────────────┐
│ SharedUniformBuffer (single Vulkan UBO)                             │
├─────────────────────────────────────────────────────────────────────┤
│ Material 0 (index=0, offset=0)      │ 128 bytes (blockAlignedSize)  │
├─────────────────────────────────────────────────────────────────────┤
│ Material 1 (index=1, offset=128)    │ 128 bytes                     │
├─────────────────────────────────────────────────────────────────────┤
│ Material 2 (index=2, offset=256)    │ 128 bytes                     │
└─────────────────────────────────────────────────────────────────────┘
```

Each `StandardResource` has:
- `m_sharedUBOIndex`: Slot in the shared buffer (0, 1, 2...)
- `m_materialProperties[]`: Float array with material data (albedoColor, roughness, metalness, etc.)

### Concurrency Contract (MANDATORY)

> [!CRITICAL]
> **Materials are loaded concurrently from the resource thread pool, and they SHARE their buffer
> identifier by design.** `getSharedUniformBufferIdentifier()` encodes only the material kind and its
> texture count (`MaterialStandardResource2Textures`, `MaterialStandardResource4Textures`, …), so two
> unrelated materials routinely request the very same buffer from different threads.
>
> **Two invariants follow, both enforced in the engine — do not undo them:**
>
> 1. **Reaching a shared buffer is a single atomic call.** Use
>    `SharedUBOManager::getOrCreateSharedUniformBuffer(name, blockSize)`. Never write
>    `getSharedUniformBuffer()` followed by `createSharedUniformBuffer()`: that check-then-act
>    sequence hands a `nullptr` to every loser of the race, and a material that cannot reach its
>    buffer **fails to load entirely**, silently removing every sub-mesh using it from the scene.
>    `createSharedUniformBuffer()` stays strict (it reports a genuine duplicate name) and is for
>    owners of a unique name, such as `LightSet`'s per-scene light buffers.
> 2. **Claiming a seat is guarded.** `SharedUniformBuffer::addElement()` scans the seat table for a
>    free slot then writes it; the `m_elementsAccess` mutex makes the pair atomic. Without it two
>    materials receive the **same** `m_sharedUBOIndex`, hence the same UBO byte offset, and overwrite
>    each other's uniform block with no error reported at all.
>
> `writeElementData()` is deliberately **not** guarded: once a seat is granted its owner holds it
> exclusively and writes to a byte range that belongs to no one else. Do not add a lock there — it
> would serialize every per-frame material update.
>
> **Lived symptom (Aug 2026):** the `reflexion-debug` dragon loaded incomplete, missing its wings or
> its body depending on the run. Log signature: the same `There is no shared uniform buffer named 'X'`
> info line **twice in a row** (two threads missing the lookup together), then
> `A shared uniform buffer named 'X' already exists !` → `Unable to get the shared uniform buffer` →
> `Unable to load the material resource ...`. The doubled info line is the tell: single-threaded, the
> first miss would have created the buffer.
>
> **Files:** `Graphics/SharedUBOManager.{hpp,cpp}`, `Graphics/SharedUniformBuffer.{hpp,cpp}`,
> `Graphics/Material/Interface.cpp` (`getSharedUniformBuffer()`). Same pattern already applied in
> `Vulkan/LayoutManager.cpp`.

### Descriptor Binding (Preferred API)

When creating descriptor sets, use the **explicit helper methods** that ensure correct byte offsets:

```cpp
// PREFERRED - Uses SharedUniformBuffer's getDescriptorInfoForElement()
// This method handles the element index → byte offset conversion internally
const auto descriptorInfo = m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex);
m_descriptorSet->writeUniformBuffer(bindingPoint, descriptorInfo);
```

**Available helper methods:**

| Method | Returns | Purpose |
|--------|---------|---------|
| `getByteOffsetForElement(index)` | `VkDeviceSize` | Byte offset for element within its UBO |
| `getDescriptorInfoForElement(index)` | `VkDescriptorBufferInfo` | Complete descriptor info ready to use |

**Why this API exists:** The old API passed an "offset" parameter that was ambiguous (element index vs byte offset). This caused bugs where all materials read from offset 0. The new explicit methods eliminate this ambiguity.

> [!NOTE]
> **AI-Friendly Design:** These methods follow the "Clarity Over Cleverness" principle from [`docs/cpp-conventions.md`](../../cpp-conventions.md#ai-friendly-code-guidelines)

### Material Property Layout (std140)

**StandardResource** — the ONE lit material (Cook-Torrance metallic-roughness) — stores properties
in a 120-float array (480 bytes, std140; `StandardResource::MaterialPropertiesSize`, one neutral list in
`neutralMaterialProperties()`). ⚠️ This line said "80 floats" until 2026-09-22 — the layout had already
grown to 116 with the indexed UV tables, and the table below had not followed:

| Offset | Property | Type | Range/Default |
|--------|----------|------|---------------|
| 0-3 | albedoColor | vec4 | Base color |
| 4 | roughness | float | 0.0-1.0 (0.5) |
| 5 | metalness | float | 0.0-1.0 (0.0) |
| 6 | normalScale | float | 0.0-1.0 (1.0) |
| 7 | specularFactor | float | 0.0+ (1.0) — KHR_materials_specular |
| 8 | ior | float | 1.0-3.0 (1.5) |
| 9 | iblIntensity | float | 0.0-1.0 (1.0) |
| 10 | autoIlluminationAmount | float | 0.0+ (0.0) |
| 11 | aoIntensity | float | 0.0-1.0 (1.0) |
| 12-15 | autoIlluminationColor | vec4 | Emissive color |
| 16 | clearCoatFactor | float | 0.0-1.0 (0.0) |
| 17 | clearCoatRoughness | float | 0.0-1.0 (0.0) |
| 18 | subsurfaceIntensity | float | 0.0-1.0 (0.0) |
| 19 | subsurfaceRadius | float | 0.0+ (1.0) |
| 20-23 | subsurfaceColor | vec4 | SSS tint (1.0, 0.2, 0.1) |
| 24-27 | sheenColor | vec4 | Sheen tint (black = off) |
| 28 | sheenRoughness | float | 0.0-1.0 (0.5) |
| 29 | anisotropy | float | -1.0-1.0 (0.0) |
| 30 | anisotropyRotation | float | 0.0-1.0 (0.0) |
| 31 | transmissionFactor | float | 0.0-1.0 (0.0) |
| 32-35 | attenuationColor | vec4 | Volume attenuation |
| 36 | attenuationDistance | float | 0.0+ — engine default **1.0 m**, glTF's is **+INFINITY** (see below) |
| 37 | thicknessFactor | float | 0.0+ — engine default **1.0**, glTF's is **0** (thin-walled) |
| 38 | heightScale | float | 0.0+ (0.02) — POM depth |
| 39 | iridescenceFactor | float | 0.0-1.0 (0.0) |
| 40 | iridescenceIOR | float | 1.0+ (1.3) |
| 41 | iridescenceThicknessMin | float | nm (100.0) |
| 42 | iridescenceThicknessMax | float | nm (400.0) |
| 43 | dispersion | float | 0.0+ (0.0) |
| 44-47 | specularColorFactor | vec4 | KHR specular color (white) |
| 48 | emissiveStrength | float | 0.0+ (1.0) — HDR multiplier |
| 49 | clearCoatNormalScale | float | 0.0+ (1.0) — CC normal map intensity |
| 50 | opacity | float | 0.0-1.0 (1.0) — global transparency |
| 51 | alphaThreshold | float | 0.0-1.0 (0.5) — cutout cutoff |
| 52 | reflectionAmount | float | 0.0-1.0 (**1.0**) — artistic override; the neutral 1.0 leaves the mix BRDF-controlled |
| 53 | refractionAmount | float | 0.0-1.0 (**1.0**) — artistic override; the neutral 1.0 leaves the blend Fresnel-controlled |
| 54 | fogResponse | float | 0.0-1.0 (1.0) — atmospheric fog response |
| 55 | dofMask | float | 0.0-1.0 (1.0) — depth-of-field response |
| 56-71 | uvwTransform[4] | 4 × vec4 | indexed table of DISTINCT transforms, `(scale.xy, offset.zw)`; slot 0 is always the identity (1,1,0,0) |
| 72-87 | uvwRotation[4] | 4 × vec4 | same slots, `(cos, sin, 0, 0)`, neutral **(1,0,0,0)** — KHR_texture_transform's `rotation`, trig resolved once on the CPU |
| 88-115 | uvwIndex[7] | 7 × vec4 | one float per ComponentType, four to a vec4: which table slot that component reads |
| 116-119 | parallaxParameters | vec4 | POM (max layers 0-64, fade start 8 m, fade end 18 m, unused) — see § Parallax Occlusion Mapping |

The GLSL struct is generated to match this layout exactly.

> [!NOTE]
> **Slots 7, 8 and 44-47 were dead until 2026-08-28.** The layout, the codegen and the BRDF term
> (`LightGenerator.PBR.cpp`: `dielectricF0 = ((ior-1)/(ior+1))²`, then
> `F0 = mix(min(dielectricF0 · specularColor · specularFactor, 1), albedo, metalness)`) were all in
> place and spec-exact, but **no loader ever wrote them**, so every asset got the identity. Because
> the identity IS the default, there was no symptom. `GLTFLoader` now fills all three (factors and
> both textures — see [`Scenes/Loaders/AGENTS.md`](../../../src/Scenes/Loaders/AGENTS.md) § *Known gaps*);
> `FBXLoader` deliberately does not, the ufbx semantics being ambiguous between OpenPBR and legacy
> Phong. ⚠️ A UBO slot that a shader reads is NOT evidence anything writes it — check both ends.
> Recorded as a trap in [`../../docs/caution-points.md`](../../caution-points.md) § *An
> IDENTITY default makes an unwired feature indistinguishable from a disabled one*.
>
> ⚠️ The UV transform table has **four** slots (the identity + three distinct transforms, which covers
> every one of 1347 measured materials) and serves every ComponentType since 2026-09-14; a material
> declaring more distinct transforms falls back to the identity for the surplus and says so once.

> [!CAUTION]
> **Slots 32-37 (the KHR_materials_volume group) carry ENGINE defaults that are NOT glTF's.**
> `attenuationDistance` defaults to 1.0 m here and to **+infinity** in the extension;
> `thicknessFactor` to 1.0 here and to **0** — thin-walled, no volume — in the extension. The
> divergence is invisible only because `attenuationColor` defaults to WHITE and the absorption is
> `exp(log(colour) / distance * thickness)`: `log(1)` is 0, so the whole product is 0 and the
> transmittance is 1 whatever the other two say. **Set a colour without setting a distance and the
> engine invents an absorption over one metre.** `GLTFLoader` states the spec's defaults explicitly
> for that reason; the JSON material format still uses these, so any new consumer must decide which
> contract it is honouring rather than assume they agree.

### Material Opacity and GrabPass

`Material::Interface` provides two key query methods used by the rendering pipeline for render list dispatch:

- **`isOpaque()`**: Returns `!BlendingEnabled`, but also returns `false` when `requiresGrabPass()` is `true` (a material requiring grab pass is inherently non-opaque). It deliberately ignores `AlphaTestEnabled` — see [Alpha Test](#alpha-test--the-binary-cutout-contract-aug-2026).
- **`requiresGrabPass()`**: Virtual method (default `false`). Overridden by `StandardResource` based on material properties (e.g., transmission with screen-space refraction).

These are propagated through `Renderable::Abstract::isOpaque(layerIndex)` and `Renderable::Abstract::requiresGrabPass(layerIndex)` to all concrete renderables, enabling the Scene to dispatch into 3 render categories: Opaque, Translucent, and TranslucentGB.

⚠️⚠️ **The renderer's grab pass is RETIRED, never recreated in place** (fixed 2026-09-24). It is
pre-allocated for every scene, and its blit runs on every frame whose scene holds a TranslucentGB
object — so as soon as one refracting material is on screen, the last frames' command buffers
reference its images. `Renderer::refreshGrabPass()` (called by every scene-target recreation, the act
teardown included) used to call `GrabPass::recreate()`, which destroyed them on the spot:
`VUID-vkDestroyImage-image-01000` ×2 + `VUID-vkFreeMemory-memory-00677`, then the same objects
reported leaked at `vkDestroyDevice`. It now hands the old object to the `DeferredDestructor` and
creates a fresh one, like `recreateSceneTarget()`; `GrabPass::recreate()` is deleted. Found on
`forest` once its diamond chick was added: the teardown logged it on every run that recreated the
scene target (3 of 3), none after the fix (3 of 3 with the recreation).

**Code references:**
- `Material/Interface.hpp:isOpaque()` — non-virtual, checks blending and grab pass
- `Material/Interface.hpp:requiresGrabPass()` — virtual, default false
- `Material/StandardResource.hpp:requiresGrabPass()` — override
- `Renderable/Abstract.hpp:requiresGrabPass()` — pure virtual

### Alpha Test — the Binary Cutout Contract (Aug 2026)

`MaterialFlagBits::AlphaTestEnabled = 1U << 16` declares a material a **binary CUTOUT**: the fragment
shader discards the texels whose alpha falls below a cutoff, and the material **STAYS OPAQUE** — opaque
render list, depth write kept, no back-to-front sorting, state-sorted batching preserved.

Two setters raise the flag:

- **`BasicResource::enableAlphaTest()`** — fixed 0.5 cutoff. It requires a texture whose alpha channel
  is enabled (`setTextureResource(texture, true)`); without one the flag emits no code. Like every other
  material setter it refuses to act once the resource is created (it warns and returns).
- **`StandardResource::enableAlphaTest(threshold = 0.5)`** (Aug 2026) — **configurable, UBO-backed** cutoff
  (`AlphaThreshold`, UBO offset 51): the generated GLSL compares against the uniform, never a literal,
  so the threshold is per-material and runtime-adjustable (`setAlphaThresholdToDiscard()`). The alpha
  source is the **opacity texture component** when present (red channel), the **albedo texture alpha**
  otherwise (glTF `alphaMode: MASK`). It also disables the blending flag — cutout and blending are
  mutually exclusive by construction.
- **`StandardResource::enableHashedAlphaTest()`** (Sep 2026) — the same cutout with a **STOCHASTIC**
  threshold: `MaterialFlagBits::AlphaHashedEnabled = 1U << 19`. Chris Wyman & Morgan McGuire, *Hashed Alpha
  Testing* (I3D 2017, JCGT 6(2)), listing 1 without the anisotropic refinement: each pixel compares its
  alpha with its own threshold in (0,1], so the fraction of the surface kept EQUALS the alpha at every
  distance. The lattice is sized to one cell per PIXEL from the screen derivatives of the anchor, the two
  power-of-two lattices around it are blended and the blend remapped back to a uniform distribution; the
  cell hash is PCG3D (Jarzynski & Olano, JCGT 9(3) 2020), an INTEGER hash so every vendor draws the same
  pattern. The anchor is **`svRestPositionModelSpace`** — the raw position attribute, before skinning and
  the vegetation wind: on the world position the pattern crawls over a swaying leaf. One spelling for the
  three sites that test an alpha (albedo alpha, opacity component, shadow pass):
  `StandardResource::alphaCutoutStatement()`. ⚠️ It reads derivatives: emit it in UNIFORM control flow.
  ⚠️ The ray-traced alpha test does NOT hash (a ray query has no screen derivatives): it keeps the fixed
  UBO threshold `enableAlphaTest()` stored. Caller of record: `Toolkit::vegetationMaterial()` (Foliage).

**Opacity — the owner's 3-rule contract (Aug 2026).** `StandardResource` expresses opacity exactly three
ways, parsed from the JSON `Opacity` component and mirrored by `setOpacityComponent()`:

1. **Scalar value [0,1]** → GLOBAL transparency: uniform alpha from the UBO (`Opacity`, offset 50),
   blending (glTF BLEND).
2. **Map + `AlphaThreshold` key** → binary CUTOUT: per-pixel discard below the UBO threshold, NO
   blending, stays opaque, casts cutout shadows, RT alpha-tests at the same cutoff (glTF MASK +
   `alphaCutoff`).
3. **Map without `AlphaThreshold`** → grayscale per-pixel alpha SCALE: `texel.r × amount`, blending.

Loader wiring: glTF `alphaMode MASK` → `enableAlphaTest(alphaCutoff)`; USD
`opacityThreshold > 0` → cutout, translucent USD/FBX materials get
`setOpacityComponent()` so the alpha VALUE finally reaches the blend (both used to raise the blending
flag with no alpha wired).

The discard fires on that flag **INDEPENDENTLY of the blending mode**. Gating it on blending was exactly
what used to force a cutout out of the opaque list: the only way to obtain a discard was to call
`enableBlending()`, which bought a distance sort that a coverage mask does not need.

| | `enableAlphaTest()` | `enableBlending(mode)` |
|---|---|---|
| Render list | **Opaque** (front-to-back, early-Z) | Translucent (back-to-front) |
| Colour blending | disabled (`blendEnable = VK_FALSE`) | enabled, per `blendingMode()` |
| Per-frame distance sort | none | mandatory |
| State-sorted batching | preserved | given up to the distance order |
| Transparency expressed | strictly binary — in or out | a genuine gradient |

Depth write is untouched by the flag: a cutout writes depth like any other opaque surface (depth write
is decided by the `RenderableInstance`, never by the material's transparency mode).

**`blendingMode()` offers exactly three modes** — `Normal` (`SRC_ALPHA`, `ONE_MINUS_SRC_ALPHA`),
`Add` (`ONE`, `ONE`) and `Multiply` (`ZERO`, `SRC_COLOR`) — plus `None`.

> [!CAUTION]
> **There is deliberately no `Screen`, and it must never be reintroduced** (deleted Sep 2026). It
> required `dstColorBlendFactor = ONE_MINUS_SRC_COLOR`, and **Vulkan does not clamp blend factors on
> a floating-point attachment**: the scene target is an unclamped `R16G16B16A16_SFLOAT` in NITS, so a
> sprite at `EmissiveStrength: 320` turned that factor into **-319** and subtracted the background
> per channel. Because an orange flame carries no blue, red and green were annihilated while blue
> passed at `+1` — a *cyan* flame, but only over bright surfaces. Measured, fixed and explained in
> [`docs/caution-points.md`](../../caution-points.md) § *`Screen` blending SUBTRACTED the
> background*; the authoring table lives in
> [`docs/material-json-format.md`](../../material-json-format.md) § `BlendingMode`.
>
> The rule that generalises: **a blend factor reading the SOURCE or DESTINATION COLOUR is a ratio by
> construction and has no meaning in a luminance buffer.** `Multiply` survives only because it does
> not flip sign; it still belongs on an LDR overlay, never on an emissive surface. Alpha-reading
> factors are safe — alpha stays in [0,1].

> [!WARNING]
> **`isOpaque()` must NOT be taught about `AlphaTestEnabled`, and must stay that way — an alpha-tested
> material IS opaque.** Returning `false` there does two damaging things at once:
>
> 1. The Scene dispatches the layer into the **distance-sorted translucent list**, paying for a sort
>    and losing the state-sorted batching, for a mask that has nothing to sort.
> 2. `Vulkan::GraphicsPipeline::configureColorBlendState()` keys its default branch on
>    `material.isOpaque()`: a `false` flips `blendEnable` to `VK_TRUE` and installs the blend factors of
>    `blendingMode()`, so the cutout's already-binary alpha gets **colour-blended** on top.
>
> Either one defeats the flag entirely. This is the single invariant that makes the cutout mode worth
> having: the flag adds a discard and changes **nothing else** about how the material is classified.

**The two other paths honour the flag as well:**

- **`isAlphaTest()`** returns `true` for `AlphaTestEnabled` (in addition to `OpacityEnabled` and
  `BlendingEnabled`), so the **RT pipeline alpha-tests at hit time** — candidate hits are confirmed
  against the material's cutoff instead of being taken as solid.
  `StandardResource::exportRTMaterialData()` exports its UBO threshold as `alphaCutoff` (Basic keeps 0.5).
  See [`docs/reflection-pipeline.md`](../../reflection-pipeline.md).
- **`requiresAlphaTestedShadows()`**: a cutout must cast a **CUTOUT shadow**, not a solid rectangle.
  `StandardResource` returns `true` when an alpha source exists AND (the flag is set **OR** the blending
  mode is `Normal`), and its shadow discard **reads the UBO threshold** (the shadow fragment shader
  declares the material uniform block — the colour pass and the shadow agree by construction).
  ⚠️ The `BlendingMode::Normal` branch is load-bearing, not belt-and-braces: a JSON `"Opacity"` texture
  arms blending and NOT the flag, and no JSON key can request a cutout — gating on the flag alone made
  every JSON-authored foliage shadow its quad. See [`docs/shadow-mapping.md`](../../shadow-mapping.md).

> [!CAUTION]
> **BasicResource's cutoff is FIXED at 0.5** (StandardResource's is configurable — see above). The program
> caches now key on the material FLAG BITS as well as the descriptor layout hash (Aug 2026: both
> `Renderable::ProgramCacheKey` and the generators' `computeProgramCacheKey()` fold in
> `material->flags()`), so the *structural* presence of the discard is discriminated. But plain VALUES
> are still not part of any key: a per-material cutoff **literal** baked into the generated GLSL could
> still serve one material's program to another sharing layout and flags. The rule is therefore:
> **a configurable threshold lives in the material UBO** (StandardResource's `AlphaThreshold` slot) — never
> in the GLSL. Basic cannot follow: its 12-float material block is FULL (diffuseColor 0-3,
> specularColor 4-7, shininess 8, opacity 9, autoIllumination 10, emissiveStrength 11); growing it is
> the price of ever making Basic's cutoff configurable. See
> [`docs/pipeline-caching-system.md`](../../pipeline-caching-system.md).
>
> 0.5 is the right value for a mask authored as coverage, and Basic's **three paths agree at 0.5**: the
> colour discard, the shadow discard, and `GPURTMaterialData::alphaCutoff`. Standard's three paths agree
> on its UBO threshold the same way.

**Which mode for which authoring intent:**

| The asset expresses… | Use |
|---|---|
| A **coverage mask** — cutout foliage, a fence, a grate, a Doom two-sided middle texture (vanilla writes the texel straight to the framebuffer and never reads the destination, so its transparency is strictly binary) | **alpha test** |
| A **genuine gradient** — smoke, a soft particle, glass that tints what is behind it | **blending** |
| **Refraction** — bending what is behind the surface | **grab pass** (`requiresGrabPass()` → TranslucentGB) |

**Code references:**
- `Material/Interface.hpp:MaterialFlagBits::AlphaTestEnabled` — the flag and its contract
- `Material/Interface.hpp:isOpaque()` — blind to the flag ON PURPOSE
- `Material/Interface.hpp:isAlphaTest()` — RT hit-time alpha test
- `Material/StandardResource.cpp:requiresAlphaTestedShadows()` — alpha source AND (flag OR `BlendingMode::Normal`)
- `Material/StandardResource.hpp:enableAlphaTest(threshold)` — the configurable, UBO-backed setter
- `Material/StandardResource.cpp:parseOpacityComponent()` — the 3-rule JSON contract
- `Material/StandardResource.cpp:alphaSourceTextureComponent()` — opacity component, else albedo alpha
- `Material/StandardResource.cpp:generateShadowAlphaTestCode()` — shadow discard against the UBO threshold
- `Material/StandardResource.cpp:alphaCutoutStatement()` — the fixed or HASHED discard, the one spelling
- `Material/GPURTMaterialData.hpp:alphaCutoff` — the RT side (Basic 0.5, Standard = UBO threshold)
- `Graphics/Renderable/ProgramCacheKey.hpp:materialFlags` — codegen flags in the program cache key
- `Vulkan/GraphicsPipeline.cpp:configureColorBlendState()` — the `isOpaque()` branch

### Normal Map Scale

The `normalScale` parameter (offset 6) controls normal map intensity by scaling the tangent-space XY components before re-normalizing:

```glsl
vec3 raw = texture(normalSampler, uv).rgb * 2.0 - 1.0;
vec3 normal = normalize(vec3(raw.xy * ubMaterial.normalScale, raw.z));
```

- `1.0` = full normal map effect (default)
- `0.5` = half intensity (smoother bumps)
- `0.0` = flat surface (normal map ignored)

**Code references:** `StandardResource.cpp:generateFragmentShaderCode()`

### Parallax Occlusion Mapping (POM)

POM ray-marches through a height map in the fragment shader to create depth/relief illusion on flat surfaces without extra geometry. Uses `ComponentType::Displacement` with height map textures.

**Activation:** a material with a Height component always gets the POM code (`m_useParallaxOcclusionMapping`).
Its **layer count is a UBO value**, `parallaxParameters.x` (vec4 at float offset 116: max layers, fade
start, fade end, unused): `setParallaxIterations(n)` sets it per material (clamped to [0, 64]), and a
material that never calls it takes `Core/Graphics/Texture/POMIterations` **when it is created**
(default 0). A count of 0 leaves `pomTexCoords` on the mesh UVs, bit-exact — the height map is then
ignored and the surface is plain normal mapping.
⚠️ Until 2026-09-22 the count was a GLSL literal read from the generator and gated the whole codegen:
it was outside the program cache key, so a program built under one count served every later material
and every later launch (the SPIR-V cache is on disk). It is a value now, never a literal — do not move
it back into the generator.

When active, a displaced UV (`pomTexCoords`) is computed at the start of the fragment shader and ALL subsequent texture samples use it automatically via `textCoords()`.

**Key implementation details (rewritten 2026-09-22):**
- Height map convention: white = high, black = low. POM inverts: `depth = 1.0 - texture().r`.
- The view vector is built in **WORLD space** and taken to tangent space with **`WorldTBNMatrix`**.
  ⚠️⚠️ NOT `TangentToWorldMatrix`: despite its name it is `NormalMatrix · (T, B, N)` — a **VIEW**-space
  frame outside MDI, a world one under MDI. The march used it on a world vector and marched in a
  direction that turned with the camera: that was the "éclaté" POM of the owner's reports
  (2026-09-08, 2026-09-22). The same misuse survives in the reflection-normal code
  (`reflectionNormal = TangentToWorldMatrix[0] · n.x + … + NormalWorldSpace · n.z`, five sites) — not
  attributed, not fixed.
- ⚠️ Tangent-space `(x, y)` is `(x, −y)` in UV: `B` is the image's +Y (Khronos), which points toward
  DECREASING v. The march used `+y` and inverted the relief along v only. Proof that it is right now: the
  relief reads raised under all four horizontal view directions (`relief` demo, 2026-09-22).
- Parallax proper: `offset = V.xy / V.z · heightScale`, with `V.z` bounded at 0.2 (a grazing ray shifts by
  five depths at most). The old code had no `/z` ("offset limiting") — the relief flattened at grazing.
- Layer march (count = `mix(max, max/4, V.z) · fade`, loop bounded by the compile-time 64), then
  `ParallaxRefinementSteps` (5) **bisection** steps (relief mapping, Policarpo et al. 2005), then the
  linear interpolation of the crossing. A single linear step leaves the layers visible as slices on every
  slope facing the camera.
- Every height sample is a `textureGrad()` with the gradients of the UNDISPLACED coordinates, taken
  before any branch: the march exits per pixel, and an implicit-derivative lookup inside non-uniform
  control flow reads undefined derivatives.
- The march runs in MESH UV space and samples the height through the height component's own UV
  transform (`transformedTexCoords()`, re-pointed at the march variable).
- `mutable bool m_pomGenerationActive` flag set at generation time, checked by `textCoords()` to return correct UV variable.

**Distance fade** (`setParallaxFadeDistances(start, end)`, UBO `parallaxParameters.yz`, default 8 → 18 m):
full relief closer than `start`, none beyond `end`; both the depth and the layer count follow the fade,
and beyond `end` the march is SKIPPED — what keeps a large surface affordable (see `docs/troubleshooting.md`
§ GPU hang with POM on large surfaces).

**Vertex shader requirements** (Height component present):
- `WorldTBNMatrix` — world tangent frame
- `PositionWorldSpace` — fragment world position
- `CameraWorldPosition` — camera position (reuses Reflection/Refraction output if present)

**⚠️⚠️ The height map must be a HEIGHT.** Measured 2026-09-22 on the store: `Grounds/Pavement005-height`
is the albedo's luminance (correlation **0.927**), its granite grain as tall as its joints — the march
extrudes every grain into a spike, and no shader change can fix it. Test before blaming the technique:
correlate the height with the albedo luminance (a real height is near 0: `Pavement006` reads −0.001), and
integrate the normal map (Frankot-Chellappa) — a coherent pair correlates > 0.95 (`Pavement006`: 0.993),
and the integrated range gives the physically consistent `heightScale` (`Pavement006`: 21.5 texels of
1024 = 0.021 UV). ⚠️ `heightScale` is in **UV units**: the store's JSON `Height.Scale` is `1.0` on ~90
materials (authored before the PBR material) — a relief one texture repeat deep. Harmless while
`POMIterations` is 0; set it globally and those materials explode.

**Bench:** projet-alpha's `relief` demo — one flat ground, `Pavement006`, low lateral sun. Option 0 is the
technique: 0 = normal mapping, 1 = POM, 2 = mesh shaders. Option 1 is the layer count. Option 2 is the depth in
thousandths of a repeat.

**Handover to real geometry (mesh-shading surface, Sep 2026).** `setParallaxHandover(start, end)` (UBO
`parallaxHandover`, float 120; the array is now 124 floats) sets the band over which a
`Geometry::DisplacedGridResource` hands its relief from displaced geometry to the POM. The geometric depth is
heightScale · (1 − t) and the POM depth heightScale · t, with t = smoothstep(start, end, d). ONLY a mesh-shading
program reads it: without a band, such a program is geometry only, and a vertex program keeps t = 1 (the fallback
is plain POM). The displacement is the material's own, through `Material::Interface::generateSurfaceDisplacementCode()`:
the same height texture, UV transform (`transformedTexCoords(…, coordinates)`) and `heightScale` (UV units × metres
per UV), read with `textureLod()` at the mip of the vertex spacing. One relief, two techniques.

**Code references:**
- `StandardResource.cpp:generateFragmentShaderCode()` — POM GLSL generation (+ distance fade)
- `StandardResource.cpp:create()` — the setting resolved into the UBO
- `StandardResource.cpp:textCoords()` — UV variable selection
- `Saphir/Keys.hpp:ParallaxTextureCoordinates` — `"pomTexCoords"`; `ParallaxParameters` — `"parallaxParameters"`
- `Saphir/Keys.hpp:HeightSampler` — `"uHeightSampler"`
