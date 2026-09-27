## PBR Advanced Material Features

The PBR Cook-Torrance BRDF supports several advanced material layers. Each feature is **compile-time conditional** — when a parameter is at its default (off) value, no extra shader code is generated.

### Clear Coat

Adds a second specular lobe on top of the base material (car paint, varnished wood).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `clearCoatFactor` | 16 | 0-1 | Coat intensity (0 = none) |
| `clearCoatRoughness` | 17 | 0-1 | Coat roughness (0 = mirror) |
| `clearCoatNormalScale` | 49 | 0+ (1.0) | Clear coat normal map intensity |

- Uses a separate GGX NDF + Smith G with its own roughness
- Energy conservation: base specular is scaled by `(1 - clearCoatFactor)`
- **Clear coat normal map** (KHR_materials_clearcoat): Optional dedicated normal map for the clear coat layer, simulating micro-imperfections (orange peel, swirl marks) independent of the base surface. When no clear coat normal is provided, the coat uses the base surface normal (`Ncc = N`).
- **Fragment-local TBN**: The clear coat normal is transformed using a tangent frame derived from the fragment normal N (`cross(N, up)`), NOT from the vertex TBN matrix. This avoids dependency on base normal mapping being active.
- **Files**: `LightGenerator.PBR.cpp` (per-light), `LightGenerator.cpp` (ambient IBL), `StandardResource.cpp` (texture sampling + UBO)

### Subsurface Scattering (SSS)

Simulates light scattering beneath the surface (skin, wax, leaves, marble).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `subsurfaceIntensity` | 18 | 0-1 | Master SSS weight + wrap amount |
| `subsurfaceRadius` | 19 | 0+ | Scatter distance for thickness falloff |
| `subsurfaceColor` | 20-23 | vec4 | Color of scattered light |
| Thickness map | texture | 0-1 | Optional per-pixel thickness |

**Three techniques combined:**
1. **Wrap diffuse** — Softens shadow terminator: `NdotLWrap = (NdotL + wrap) / (1 + wrap)`
2. **Back-lit transmittance** — Light through thin areas: `exp(-thickness / radius) * NdotLBack`
3. **Ambient SSS** — Tinted ambient in shadow areas

> [!WARNING]
> **SSS wrap value clamped to 0.99**: `sssWrap = min(sssIntensity, 0.99)`. When `sssIntensity = 1.0`, `smoothstep(sssWrap, 1.0, x)` requires `edge0 < edge1`. With `sssWrap = 1.0`, this becomes `smoothstep(1.0, 1.0, x)` — **undefined behavior in GLSL** (produces NaN on some GPUs, causing flickering/darkening). See: `LightGenerator.PBR.cpp` lines 702, 716.

- **Files**: `LightGenerator.PBR.cpp` (per-light wrap + transmittance), `LightGenerator.cpp` (ambient SSS)

### Sheen

Adds a soft edge highlight for fabric-like materials (velvet, silk, wool).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `sheenColor` | 24-27 | vec4 | Sheen color tint (black = off) |
| `sheenRoughness` | 28 | 0-1 | 0 = satin, 1 = wool |

- Uses Charlie distribution (sin²-based NDF) for soft retroreflection
- Energy conservation via DFG approximation: `sheenScaling = 1 - max(sheenColor) * (0.157 * sheenRoughness + 0.04)`
- Applied to both per-light and ambient passes
- **Files**: `LightGenerator.PBR.cpp` (per-light), `LightGenerator.cpp` (ambient)

### Anisotropic Specular

Stretches specular highlights along a direction (brushed metal, hair, vinyl records).

| Parameter | UBO Offset | Range | Effect |
|-----------|-----------|-------|--------|
| `anisotropy` | 29 | -1 to 1 | Stretch strength (0 = isotropic) |
| `anisotropyRotation` | 30 | 0-1 | Direction rotation (maps to 0-2π) |

**BRDF functions (compile-time conditional):**
- `distributionGGXAniso(T, B, N, H, at, ab)` — Anisotropic GGX NDF
- `visibilityAniso(T, B, N, V, L, at, ab)` — Smith height-correlated anisotropic visibility (Heitz 2014)

**Key implementation details:**
- **Roughness squaring**: `alphaRoughness = roughness²`, then **`at = mix(alphaRoughness, 1.0, aniso²)`, `ab = clamp(alphaRoughness, 0.001, 1.0)`** — `KHR_materials_anisotropy`'s own formula, verbatim from the extension's implementation notes and the Khronos Sample Renderer's `brdf.glsl`, which agree word for word. It widens ONE axis toward the isotropic ceiling instead of scaling both apart.
  > ⚠️⚠️ **It was `at = alpha * (1 + aniso)` / `ab = alpha * (1 - aniso)` until 2026-09-14, and that
  > violated the extension.** At roughness 1.0 it gave `at = 2.0` — outside a GGX alpha's valid
  > range — and `ab = 0.0`, so the highlight stayed maximally anisotropic exactly where the spec
  > says anisotropy must have **no effect**. Measured on `AnisotropyStrengthTest`: the lobe still
  > stretched **×1.84** down the roughness-1.0 column (now ×0.85, i.e. flat), while the
  > roughness-0.0 column — where the extension says the effect should be strongest — was
  > unmeasurable (a near-delta lobe of a few pixels) and now runs **18.73 → 9.61**.
  > ⚠️ The squaring drops the SIGN of `aniso`, whose range this engine documents as -1..1. glTF's
  > `anisotropyStrength` is ≥ 0 so nothing is lost there, and no material in the store declares a
  > negative value (27 use a positive one) — but a negative anisotropy now behaves like its
  > absolute value instead of stretching the other way. Rotate by a quarter turn to get that.
  > ⚠️ Control for any change here: `MetalRoughSpheres`, which declares no anisotropy, must come
  > out **bit-exact** (it did: 0.000 % of pixels, max delta 1).
- **Procedural tangent frame**: T/B derived from N in fragment shader (`cross(N, up)`), NOT from mesh TBN. This avoids triangle-seam artifacts at UV discontinuities.
- **Normal mapping compatible**: Procedural frame is rebuilt from the perturbed N, so anisotropy correctly follows normal-mapped surfaces.
- **Files**: `LightGenerator.PBR.cpp` (BRDF functions + per-light), vertex shader TBN only for normal mapping

> [!CAUTION]
> **The clear coat reflects the environment along ITS OWN normal since 2026-09-14, and that is what
> makes a coat normal map visible at all.** Until then the ambient pass handed the coat
> `reflectedColor` — the environment sampled along the BASE normal, at the BASE roughness — weighted
> by a Fresnel built from the BASE normal, so `clearcoatNormalTexture` could not appear in the
> dominant term of an IBL-lit scene. `ClearCoatTest`'s `Coat normal map` row rendered as a smooth
> capsule while `Shared normal map` (whose BASE carries the same map) corrugated correctly — the
> shape of the bug, and the thing that identifies it.
> The material now emits `SurfaceClearCoatReflectionNormal` (the coat normal in WORLD space) and
> `SurfaceClearCoatReflectionColor` (the prefiltered cubemap along it, at the COAT's roughness LOD),
> and declares them through `declareSurfaceClearCoatReflection()`.
> - ⚠️ **Declared from the MATERIAL's fragment generation, never from `setupLightGenerator()`**: the
>   two variables exist only under the conditions that emit them (bindless environment cubemap, high
>   quality, a coat normal map, a non-zero coat factor), which are not knowable at setup time. The
>   material's fragment code is generated BEFORE the light generator's, which is what makes that
>   possible. Declaring them unconditionally yields a shader that fails at RUNTIME with
>   `undeclared identifier`; the C++ compiles either way.
> - ⚠️ **Emission order is load-bearing**: the sample consumes `SurfaceClearCoatNormal`, so it is
>   emitted AFTER that component's block. Both are `Location::Top`, where the order is the EMISSION
>   order.
> - ⚠️ `TangentToWorldMatrix` is now requested when EITHER the base or the coat has a normal map. A
>   material whose base is smooth and whose coat is not would otherwise have no world tangent frame.
> - ⚠️ **A coat WITHOUT a normal map still samples at the BASE roughness**, which is wrong for the
>   same reason — a clear coat is typically far smoother than what it covers. Scoped out
>   deliberately so every other row of the test stays bit-exact; recorded as remaining work.
> - Measured: the two rows carrying a coat normal map change on **5.43 %** of their pixels (max delta
>   207 / 221) and the corrugation appears; the three rows with no coat normal map are **bit-exact**
>   (0.00 %, delta 0). Zero VUID, base suite 2049/2049.
> - ⚠️ The per-light lobe was ALSO wrong and is fixed with it: `Ncc` resolved the tangent-space map
>   against a frame built procedurally from N — the trick anisotropy uses, right for a DIRECTION
>   field and wrong for a normal map. It now uses `transpose(ViewTBNMatrix)`, like the base normal.

> [!WARNING]
> **Anisotropy tangent frame**: Do NOT use `ViewTBNMatrix` for anisotropy direction. Per-vertex tangent vectors from UV-mapped meshes have discontinuities at UV seams, causing visible triangle edges in specular highlights. Always compute T/B procedurally from the fragment normal.

### Feature Combinations

Features can be combined freely. The shader generator handles all combinations:
- Clear Coat + Subsurface, Clear Coat + Anisotropy, etc.
- Sheen is typically used alone (fabric materials are rarely metallic/clear-coated)
- When `subsurfaceIntensity = 0`, `sheenColor = black`, `anisotropy = 0`, `clearCoatFactor = 0`: no extra code generated

### Clear Coat Normal — Fragment-Local TBN Pattern

> [!WARNING]
> **Do NOT use `ViewTBNMatrix` for clear coat normal transformation.** The clear coat normal map must use a fragment-local tangent frame derived from the surface normal N, identical to the anisotropy pattern. Using the vertex TBN matrix (`ViewTBNMatrix`) causes GPU hangs when normal mapping is not active on the base material, and GLSL compilation errors (`svViewTBNMatrix` undeclared) when the TBN synthesis is conditional on `m_useNormalMapping`.
>
> **Pattern:**
> ```glsl
> const vec3 ccT = abs(N.y) < 0.999 ? normalize(cross(N, vec3(0.0, 1.0, 0.0))) : normalize(cross(N, vec3(1.0, 0.0, 0.0)));
> const vec3 ccB = cross(N, ccT);
> const vec3 Ncc = normalize(ccT * SurfaceClearCoatNormal.x + ccB * SurfaceClearCoatNormal.y + N * SurfaceClearCoatNormal.z);
> ```
>
> **Code reference:** `LightGenerator.PBR.cpp` — Ncc blocks in both CC+SSS and CC-only paths
