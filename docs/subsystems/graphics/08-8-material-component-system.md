## 8. Material Component System

### Base colour — the texture is TINTED by the colour (Aug 2026)

When the albedo component is a **Texture**, the generated fragment code multiplies the
sampled texel by the material's base colour:

```glsl
const vec4 SurfaceAlbedoColor = texture(AlbedoSampler, uv) * MaterialUB(AlbedoColor);   /* StandardResource */
```

This is what every source format means: glTF `baseColorFactor` and FBX `base_color` both specify
the **PRODUCT** of factor and texture. Both loaders used to call `setAlbedoComponent(texture)` and
**drop the factor entirely** — a material tinted by factor over a neutral texture imported
untinted, and the factor's **alpha went with it**. They now set the tint through
`setAlbedoColor()` alongside the texture component.

> [!IMPORTANT]
> **`DefaultAlbedoColor` is `White`, and that is load-bearing — it was `Grey`.** The colour is now
> a multiplicative factor on the textured path, so its neutral value MUST be the multiplicative
> identity. Leaving it grey would have darkened **every** textured material in the engine by half.
> Only a material that configures no colour at all sees any change, and white is the correct
> neutral there too. ⚠️ The exception this note used to carry is **gone**: `BasicResource` kept
> `DefaultDiffuseColor{Grey}` and was the one tier outside the reasoning, but the class was removed
> (`a16195166`) and `StandardResource` is the only concrete material left — the reasoning now covers
> everything. Its `DynamicColorEnabled` gate was deliberately **not** ported for exactly this
> reason: the gate existed only to avoid multiplying by a grey default, so with a white default the
> unconditional multiply IS the contract.

> [!WARNING]
> **The multiplication is UNCONDITIONAL, and must stay that way.** The shader program cache keys
> on the material's **descriptor layout**, never on its flags or values, so a variant emitted only
> when the tint differs from white could serve one material's program to another sharing the same
> layout. Same reasoning as the fixed 0.5 alpha-test cutoff — see
> [Alpha Test](#alpha-test--the-binary-cutout-contract-aug-2026).

> [!NOTE]
> `AlbedoColor` is declared **unconditionally** in `getUniformBlock()`, whatever
> the component's filling type — the uniform block is a fixed layout mirroring the whole
> `m_materialProperties` array. That is precisely why the textured path may reference them.

### Scalar components — source CHANNEL + multiplying FACTOR (Aug 2026)

A texture-driven scalar component (roughness, metalness) reads **one color channel** of the
sampled texel, selected per component via `Component::Texture::setSourceChannel()`
(`EmEn::Base::PixelFactory::Channel`, **default Red** — the grayscale/single-channel convention).
Packed textures select theirs: glTF metallic-roughness packs **roughness in GREEN** and
**metalness in BLUE** (glTF 2.0 § material.pbrMetallicRoughness; reference implementation:
Khronos glTF-Sample-Renderer `material_info.glsl`, `getMetallicRoughnessInfo()`). JSON materials
may set the optional `"SourceChannel"` key (numeric, 0:R 1:G 2:B 3:A) on a texture component.

> [!WARNING]
> **Reading the wrong channel does not fail — it flattens.** The RED channel of a packed glTF
> metallic-roughness texture is typically EMPTY (measured mean 0.9/255 on DamagedHelmet, while
> G and B carried stdev 73 and 108): both properties silently collapse to ~0 over the whole
> surface — mirror-perfect dielectric everywhere, zero surface disparity, no error anywhere.
> That is a *material-identity* bug that reads like a lighting bug.

The generated definition folds **channel, inversion and factor** — every consumer (direct-light
BRDF, IBL prefiltered LOD, transmission LOD, material-properties G-buffer) reads this single
final variable and must NEVER re-apply any of them:

```glsl
const float SurfaceRoughness = texture(RoughnessSampler, uv).g * MaterialUB(Roughness);  /* factor contract */
const float SurfaceMetalness = texture(MetalnessSampler, uv).b * MaterialUB(Metalness);
/* smoothness/gloss source (m_invertRoughness): (1.0 - texel) BEFORE the factor applies */
```

- The UBO scalar is the **VALUE** when no texture drives the component, and the **MULTIPLYING
  FACTOR** when one does — the glTF `roughnessFactor * texel.g` / `metallicFactor * texel.b`
  contract. `DefaultTextureFactor{1.0F}` is the neutral default of the texture overloads —
  **same precedent as the White `DefaultAlbedoColor`** (a 0.5 default would halve every map;
  the old `DefaultMetalness` 0.0 would ZERO metalness maps out).
- **Format translation is the loader's job**: glTF factors multiply (pass them through); in
  **FBX a connected texture REPLACES the scalar** — the loader passes the neutral factor,
  never the authored scalar (a metalness scalar of 0, the FBX default, would erase the map).
- **RT parity**: `RTTextureSlot` carries the channel; `SceneMetaData` packs it into the RT
  material `flags` as 2-bit indices (`RoughnessChannelShift`/`MetalnessChannelShift`), plus
  `RoughnessTexInverted` for gloss sources. The RTR hit shading applies channel, inversion and
  factor exactly like the raster (see `Effects/Lighting/RTR.cpp`) — keep both sides in sync.

### Per-component UV transform — UBO values, never literals (Aug 2026)

Texture components carry a UV transform (`uv * scale + offset`) stored ON the component
(`Component::Texture::setUVWScale/setUVWOffset`, JSON keys `"UVW"` / `"UVWOffset"`) and synced
at creation into per-component material UBO vec4 slots (offsets 56-79) — Albedo/Roughness/
Metalness/Normal/AmbientOcclusion/AutoIllumination.
Applied UNCONDITIONALLY with the identity neutral (1,1,0,0) — same precedent as the White
albedo. Public API: `StandardResource::setComponentUVWTransform()` (components without a slot
return false). Source: glTF `KHR_texture_transform` via the loader.
⚠️ The `m_UVWScale` member existed for years but NO codegen consumed it — a transform that
is stored but never applied fails SILENTLY (stretched textures, zero log).
⚠️ RT hit shading does not apply these transforms yet — known parity gap.

### FillingType Enum

Material components use `FillingType` to determine how data is sourced:

| Value | Description | Data Format |
|-------|-------------|-------------|
| `Value` | Single float | Numeric JSON |
| `Color` | RGB/RGBA color | Array `[r, g, b]` or `[r, g, b, a]` |
| `Texture` | 2D texture | Object `{ "Name": "path" }` |
| `VolumeTexture` | 3D texture | Object `{ "Name": "path" }` |
| `Cubemap` | Cubemap texture | Object `{ "Name": "path" }` |
| `AnimatedTexture` | Animated texture | Object `{ "Name": "path" }` |
| `AlphaChannelAsValue` | Use alpha as value | Object |
| `Automatic` | Auto-configure | Optional params (Amount, IOR, etc.) |
| `None` | Disabled | No data required |

**Code reference:** `Graphics/Types.hpp:FillingType`

### Component JSON Parsing

All material components follow the same parsing pattern via `parseComponentBase()`:

```json
{
    "ComponentName": {
        "Type": "Texture",
        "Data": { "Name": "Category/TextureName" },
        "OptionalParam": 1.0
    }
}
```

**Special case - Automatic type:**
- No `Data` key required
- Parameters read directly from component object
- Used for Reflection/Refraction to use scene environment cubemap

```json
{
    "Reflection": { "Type": "Automatic", "Amount": 0.1 },
    "Refraction": { "Type": "Automatic", "IOR": 1.5 }
}
```

> [!CRITICAL]
> **`"Shininess"` in a manifest is a GLOSSINESS in [0,1], and the lit material stores a ROUGHNESS.**
> The whole data store was authored as a perceptual glossiness (3834 material files of 3917 hold
> `0.1`). Since the material merge there is no Blinn-Phong exponent left on the lit path: the
> conversion happens at the parse boundary ONLY, in `StandardResource::parseSpecularComponent()`,
> and it is the canonical complement
>
> ```
> roughness = 1 - clampToUnit(glossiness)   // 0.1 -> 0.9 | 0.4 -> 0.6 | 1.0 -> 0.0
> ```
>
> (Khronos archived spec-gloss extension). The absent-key fallback is `DefaultRoughness{0.5F}`.
> ⚠️ The Khronos "F0 = specular colour" half is DELIBERATELY NOT applied: legacy Phong specular
> colours are highlight intensities (bright greys), not a dielectric F0 (~0.04) — mapped raw they
> read near-mirror. F0 stays the 0.04 dielectric default; the low roughness carries the highlight.
> ⚠️ `BasicResource` reads the SAME key as a raw Blinn-Phong exponent (`DefaultShininess{200}`,
> no conversion) — the cheap tier still shades Blinn-Phong. Do not port either rule to the other.
>
> See `docs/caution-points.md`, "The legacy specular was not energy-normalised, and `Shininess` was
> authored as a glossiness".

**Code references:**
- `Graphics/Material/Helpers.cpp:parseComponentBase()` - Base parsing
- `Graphics/Material/StandardResource.cpp:parseReflectionComponent()` - Automatic handling
- `Graphics/Material/StandardResource.cpp:parseSpecularComponent()` - Glossiness → roughness

### Material Types Array

> [!CRITICAL]
> **All material resource types must be registered in `Material::Types`!**
>
> `Materials.hpp` defines the valid material types for JSON validation:
> ```cpp
> constexpr auto Types = std::array< std::string_view, 2 >{
>     BasicResource::ClassId,      // "MaterialBasicResource"
>     StandardResource::ClassId    // "MaterialStandardResource"
> };
> ```
>
> Missing types cause silent fallback to `BasicResource` during mesh loading.
>
> ⚠️ There is ONE lit material since the merge: `StandardResource` IS the Cook-Torrance
> metallic-roughness material (it kept the `"MaterialStandardResource"` ClassId). The name
> `PBRResource` and the ClassId `"MaterialPBRResource"` no longer exist. In a JSON **scene**
> definition the `"Type"` strings `"Standard"` and `"PBR"` are both accepted, as synonyms
> (`Scenes/DefinitionResource.cpp`).
