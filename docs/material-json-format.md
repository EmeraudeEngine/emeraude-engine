# The JSON material format

`Graphics::Material::StandardResource` is loaded from a JSON object. This is the reference for that
object — until 2026-08-29 the only description of it was the parser itself.

## The rule

**Every material feature is a TOP-LEVEL key.** A feature's scalars live inside its own block; a
feature's companion **texture maps are top-level keys of their own**, named after their
`ComponentType`. Nothing is nested inside another feature's block, and nothing is conditional on how
a neighbouring block happens to be filled.

```jsonc
{
    "Albedo":      { "Type": "Color",   "Data": [0.94, 1.0, 0.96, 1.0] },
    "Roughness":   { "Type": "Value",   "Data": 0.02 },
    "Metalness":   { "Type": "Value",   "Data": 0.0 },
    "IOR":         1.58,
    "Reflection":  { "Type": "Automatic", "IBLIntensity": 0.25 },
    "Transmission":{ "Type": "Value", "Data": 1.0, "ScreenSpace": true,
                     "AttenuationColor": [0.09, 0.55, 0.22],
                     "AttenuationDistance": 0.35, "Thickness": 0.6 }
}
```

## Component blocks

Every component block carries a `Type` and, for most types, a `Data`:

| `Type` | `Data` | Meaning |
|---|---|---|
| `None` | — | the component is absent (explicit, and the same as omitting the key) |
| `Value` | number | a scalar |
| `Color` | `[r, g, b]` or `[r, g, b, a]` | a colour, alpha defaulting to 1 |
| `Texture` | texture object | a 2-D texture |
| `VolumeTexture`, `Cubemap`, `AnimatedTexture` | texture object | the other texture kinds |
| `Gradient` | gradient object | a gradient |
| `Automatic` | *(none)* | the engine supplies the source — the scene's environment cubemap. The block's own extra keys are read directly from it. |

⚠️ `Automatic` is the one type that has **no** `Data`: its parameters sit beside `Type`.

### Surface

| Key | Types | Extra keys |
|---|---|---|
| `Albedo` (or `Diffuse`) | Color, Texture… | |
| `Roughness` | Value, Texture… | |
| `Metalness` | Value, Texture… | |
| `Normal` | Texture | `Scale` |
| `Height` | Texture | `Scale` |
| `AmbientOcclusion` | Texture | |
| `Opacity` | Value, Texture | `AlphaThreshold` |
| `AutoIlluminationColor` | Color, Texture | `Amount` |

### Optics

| Key | Types | Extra keys |
|---|---|---|
| `IOR` | **plain number, top level** | the material's index of refraction (`KHR_materials_ior`) |
| `Reflection` | `Automatic`, Cubemap, Texture… | `IBLIntensity` |
| `Refraction` | `Automatic`, Cubemap, Texture… | `IOR`, `Amount` |
| `Transmission` | Value, Texture | `AttenuationColor`, `AttenuationDistance`, `Thickness`, `ScreenSpace` |
| `Dispersion` | **plain number, top level** | chromatic dispersion strength |

⚠️ `IOR` at the top level is new (2026-08-29). It used to be reachable **only** from inside the
`Refraction` block, so a transmissive glass without a cubemap refraction could not declare one at
all. Since the same date the IOR drives the dielectric F0 in **all four** ambient Fresnel branches,
not just refraction, so it belongs to the material. `Refraction` still sets it when present and runs
first, so the top-level key has the last word when both appear.

⚠️ `"ScreenSpace": true` picks the **grab pass** — the transmitted light is the rendered scene behind
the surface. Without it, transmission reads the environment cubemap. Use the grab pass for anything
that should show the room it stands in.

⚠️ `Thickness` is **two things at once**: the optical path for Beer-Lambert absorption AND the length
of the refraction ray whose exit point offsets the grab-pass sample. Beer only cares about
`Thickness / AttenuationDistance`, so pick the pair together — see
`tools/gltf-conformance-bench/make-volume-probe.py`.

### Layers

| Key | Types | Extra keys |
|---|---|---|
| `ClearCoat` | Value, Texture | `Roughness` |
| `Subsurface` | Value, Texture | `Radius`, `Color` |
| `Sheen` | Color, Texture | `Roughness` |
| `Anisotropy` | Value, Texture | `Rotation` |
| `Iridescence` | Value, Texture | `IOR`, `ThicknessMin`, `ThicknessMax` |
| `Specular` | Value | `Color` |

⚠️ `Specular`'s value is `KHR_materials_specular`'s factor and the spec bounds it to **[0, 1]**;
`setSpecularFactor()` does not currently enforce that.

⚠️ `Iridescence`'s `ThicknessMin`/`ThicknessMax` are **nanometres**, on both sides — no conversion.
Without `IridescenceThickness` below, the film thickness is the **MAXIMUM**, which is the spec's
fallback and not the midpoint.

### Companion texture maps

These are **top-level keys**, read unconditionally, whatever their feature's own block contains:

| Key | Channel | Belongs to |
|---|---|---|
| `ClearCoatRoughness` | R | `ClearCoat` |
| `ClearCoatNormal` | RGB | `ClearCoat` |
| `SpecularColor` | RGB, **sRGB** | `Specular` |
| `IridescenceThickness` | **G** | `Iridescence` |
| `VolumeThickness` | **G** | `Transmission`'s volume |

⚠️ Before 2026-08-29, `ClearCoatRoughness` was read only when `ClearCoat` itself happened to be a
texture — so a scalar clear coat silently dropped its roughness map — and the other four had **no
JSON path at all**, existing only as C++ setters.

⚠️ **The channel matters.** `IridescenceThickness` and `VolumeThickness` are read from **G**, because
glTF packs them there and the R channel of that same image is often the factor map. Reading `.r`
works on a single-purpose texture and produces a silently wrong result on a packed one.

⚠️ **sRGB is decided by the variable name**, not by the key: `Component::Texture` enables it when the
GLSL surface variable name ends with `Color`. That is why `SpecularColor` is sRGB — glTF declares
`specularColorTexture` as such — while the two thickness maps stay linear. Renaming one of those
variables silently changes its colour space.

### Whole-material scalars

`EmissiveStrength`, `FogResponse`, `DoFMask`, `BlendingMode`, `Shininess` (legacy) — plain values at
the top level.

#### `BlendingMode` — three modes, and one that was deleted

| Value | Blend factors (src, dst) | Use |
|---|---|---|
| `"Normal"` | `SRC_ALPHA`, `ONE_MINUS_SRC_ALPHA` | genuine translucency (glass, a faded decal) |
| `"Add"` | `ONE`, `ONE` | **anything self-illuminated** — flames, explosions, sparks, neon |
| `"Multiply"` | `ZERO`, `SRC_COLOR` | darkening overlays |
| `"None"` | blending off | the default; a sprite additionally gets a 0.5 alpha cutout |

> [!CAUTION]
> **`"Screen"` was REMOVED (Sep 2026) and must never come back.** A manifest that still carries it
> keeps loading — it is read as `"Add"` and logs a warning — but the value is obsolete.
>
> The screen operator is `1 - (1 - src)(1 - dst)`, i.e. `src + dst·(1 - src)`, and it is **only
> defined for a source inside [0,1]**. Expressed with fixed-function blending it needs
> `dstColorBlendFactor = ONE_MINUS_SRC_COLOR`, and **Vulkan does not clamp blend factors on a
> floating-point attachment**. The scene target is an unclamped `R16G16B16A16_SFLOAT` holding
> ABSOLUTE LUMINANCE IN NITS, so `EmissiveStrength: 320` made that factor **-319**: the background
> was *subtracted*, per channel, instead of screened.
>
> Because the factor is per channel, the failure is **coloured**, which is what makes it hard to
> recognise. An orange flame has no blue, so its blue factor stayed near `+1` while red and green
> went deeply negative: over a bright surface the red channel was annihilated and only the
> background's green and blue survived. Measured in `game-logic`: 2031 pixels at
> **R=11, G=164, B=167** in the flame — a *cyan* flame — with 52.9 % of its body crushed to black.
> After the migration to `"Add"`: 2 such pixels left in the frame (and both belong to a genuinely
> blue material elsewhere), 0 % black, red back to 230.
>
> **The general rule: an operator defined over an SDR ratio domain has no meaning in a nits
> buffer.** `"Multiply"` is the same family — `dst·src` with `src` in the hundreds blows the
> destination up rather than darkening it — so it belongs on an LDR overlay, not on an emissive
> surface. Emissive surfaces composite additively, because light adds.

## Adding a component

- A **feature**: write its `parse<Name>Component()` and call it from the dispatcher.
- A companion **map**: add ONE line to the `textureMaps` table in
  `StandardResource::load()`. Do not grow a bespoke branch inside a feature parser again — that is
  exactly how the format drifted.

## Where the materials live

`projet-alpha.data/data-stores/Materials/`, indexed in `ResourcesIndex.000.json` under
`Stores.Materials` as `{ "Name": "Category/Name", "Source": "LocalData", "Data": "Materials/…json" }`.

**`Materials/Parametrics/` is the parametric library** — textureless materials (stones, metals,
glasses) that the `geometry-loader` demo cycles by reading the store. Adding one is adding a JSON
file: no rebuild, no code. It replaced the `MaterialDebug` demo, which had to be edited and
recompiled to show a new material.
