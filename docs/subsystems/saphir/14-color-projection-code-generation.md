## Color Projection Code Generation

Color projection allows lights to project a texture onto surfaces (gobo/light mask effect). It is **independent** of shadow mapping — a light can project colors without a shadow map.

### RenderPassType Drives Generation

The `RenderPassType` enum encodes shadow + color projection combinations. The `LightGenerator` switch statements use fallthrough logic to set `enableShadowMap` and `enableColorProjection` flags:

```cpp
case DirectionalLightPassFullCSM: enableColorProjection = true; [[fallthrough]];
case DirectionalLightPassCSM: enableShadowMap = true; lightType = Directional; break;

case DirectionalLightPassFull: enableColorProjection = true; [[fallthrough]];
case DirectionalLightPassShadowMap: enableShadowMap = true; [[fallthrough]];
case DirectionalLightPassColorMap: if (!enableShadowMap) { enableColorProjection = true; } [[fallthrough]];
case DirectionalLightPass: lightType = Directional; break;
```

### The volumetric clouds' shadow is a BRANCH in every directional variant, not a pass type (Sep 2026)

The clouds' Beer shadow map (`Graphics::CloudShadowMap`, `src/Graphics/AGENTS.md` § *The clouds'
shadow on the world*) is read by the PBR directional passes — classic AND CSM — whenever the
generator has the bindless table (`bindlessTexturesEnabled()`). No `RenderPassType` was added: the
variants are precompiled per renderable, so a cloud-shadow family would have doubled every
directional program for a term most scenes never use. Instead:

- **Vertex**: `PositionWorldSpace` is requested `ToNextStage` on every such directional pass (the
  CSM passes already forwarded it — a second request is harmless).
- **Fragment**: the bindless `textures2D[]` array is declared (de-duplicated if the colour projection
  declared it) with `GL_EXT_nonuniform_qualifier`, then `float cloudShadow = 1.0;` and, when the
  light's `cloudShadowIndex` (a `uint` stored as float bits, `floatBitsToUint`) is not `0xFFFFFFFF`,
  `exp(-min(B, G · max(d − R, 0)))` from the map at `cloudShadowMatrix · worldPosition` — a UNIFORM
  branch. `cloudShadow` multiplies the light's radiance next to `projectionColor`.
- **The light block** (`LightGenerator::getUniformBlock*()`): the Directional block now ALWAYS
  declares its full layout — colour, direction, intensity, colour-projection index, boost,
  view-projection matrix, PCF radius, shadow bias, then `cloudShadowMatrix` (float 32) and
  `cloudShadowIndex` (float 48); the CSM block appends them at 84 and 100. ⚠️ It used to shrink with
  the variant's features, which would have moved the two cloud members per variant; the C++ side
  (`DirectionalLight` offsets) writes ONE layout per block kind, and the two must stay in lockstep.

### Emission on the UNLIT path — `emissionMultiplier()` MULTIPLIES, it does not ADD

Emission is normally applied by `LightGenerator`, which **never runs for an unlit material**.
So `SimplePass` (the skybox, unlit sprites) used to emit a bare `svOutputFragment = SurfaceColor;`
— the material-properties block carried `emissiveStrength` but nothing consumed it, which is why a
skybox authored at 8000 nits rendered at its raw texel value. Found only by dumping the generated
GLSL; the C++ setters all looked correct.

`Material::Interface::emissionMultiplier()` closes it, and the asymmetry is deliberate:

- **UNLIT**: there is no lighting to add emission on top of, so the surface colour **IS** the
  emitted radiance — the emission MULTIPLIES it (`SceneRendering.cpp`, `SimplePass` branch).
- **LIT**: `LightGenerator` keeps ADDING the emission to the shaded result.

Applying it in both would double-count. ⚠️ And it is deliberately **NOT** applied to the albedo
G-buffer attachment written from the same `fragmentColor()` expression: albedo is a reflectance in
[0,1], and pushing 8000 nits into it poisons every consumer that reads it — RTGI first.

### `declareViewUniformBlock()` is IDEMPOTENT (Sep 2026)

Several parts of ONE shader need the view block: the lighting (`SceneRendering`, before the light
generator), a transmissive material's grab-pass refraction and its depth-based opacity
(`StandardResource`). All of them go through `Generator::Abstract::declareViewUniformBlock()`, the one
place that builds that block (regular / cubemap / CSM layouts, one instance name `ubView`), so the
helper now returns `true` without declaring when the shader already holds `ubView` at the same set and
binding. Before (owner decision on WHERE to fix, 2026-09-24: the helper, not Saphir's `declare()` and
not the material), `AbstractShader::declare()` dropped the duplicate but warned
`An uniform block declaration named 'View' already exists !` once per lit program — 14 lines for one
diamond on `forest`, noise that buried a real conflict. ⚠️ A view block at ANOTHER set or binding still
reaches `declare()` and its warning: that one is a real conflict.

### Screen-space refraction is done in VIEW space, and `svModelScale` is why (Aug 2026)

`KHR_materials_volume` gives the refraction ray its LENGTH (`thicknessFactor`) and the material's
IOR gives its DIRECTION. The screen displacement is then the **projection of the ray's exit point**,
never a world direction reinterpreted as a UV.

**Why view space and not world space.** Projecting an exit point needs a world→clip matrix, and the
fragment stage does not have one:

- `Generator::Abstract::declareViewUniformBlock()` puts `projectionMatrix` in the regular View UBO
  but **no view matrix** — the view matrix travels as a push constant for regular rendering.
- That matrices push constant is declared **`VK_SHADER_STAGE_VERTEX_BIT | GEOMETRY`** only
  (`RenderableInstance::Abstract`), so a fragment shader cannot read it either.
- The cubemap View UBO *does* carry a view matrix, but inside a per-face `instance[]` array indexed
  by `gl_ViewIndex` — a **vertex** input.

View space needs none of that: the camera is the origin, so the incident direction is simply
`normalize(positionViewSpace)`, and `projectionMatrix` alone closes the computation. The view
transform is rigid, so a world-space ray LENGTH carries over untouched.

**`ShaderVariable::ModelScale` (`svModelScale`)** is the new synthesizable vertex variable this
needs: glTF authors `thicknessFactor` in MESH space, so it must be scaled to world units.
`AbstractVertexStage::synthesizeModelScale()` emits `vec3(length(M[0].xyz), length(M[1].xyz), length(M[2].xyz))`
as a **flat** output — it is a per-draw/per-instance constant, never per-vertex. ⚠️ Its four branches
(MDI / instancing attribute / instance-transforms SSBO / push constant) **mirror
`synthesizeVertexPositionInWorldSpace()`**; a fifth model-matrix path must teach BOTH.

**The octahedral imposter billboard (Sep 2026)** — `AbstractVertexStage::enableImposterBillboarding(bounds, grid)`
(set by an imposter material, `StandardResource::setImposterAtlas()`). The quad's [-1, 1]² corners are placed in
OBJECT space around the bounding sphere, facing the eye brought back through the inverse model matrix, so the
standard matrices, the velocity and the TBN downstream apply unchanged; it is a unique preparation registered right
after the model matrix it reads, and `vertexPositionExpression()` / `vertexFrameExpression()` return
`imposterPosition` / `imposterTangent|Binormal|Normal` like the heightfield's. It also picks the three views around
the direction to the eye and outputs `svImposterUV0..2` (smooth), `svImposterWeights`, `svImposterCell0..2` and the
billboard frame as three `vec3` (`svImposterRight|Up|Back` — a `mat3` varying takes three locations). The GLSL of the
hemi-octahedral mapping is `Saphir/ImposterGLSL.hpp`: ⚠️⚠️ a SECOND implementation of emeraude-base
`Math/OctahedralMapping.hpp`, transcribed line by line — the unit tests guard the C++ one only; change both at once.

**`ShaderVariable::RestPositionModelSpace` (`svRestPositionModelSpace`, Sep 2026)** is the raw position
ATTRIBUTE, before skinning, the vegetation wind and any displacement — deliberately not
`vertexPositionExpression()`. It is the anchor of the hashed alpha test
(`StandardResource::alphaCutoutStatement()`, `src/Graphics/AGENTS.md` § Alpha Test): a threshold hashed on
the WORLD position crawls over a leaf the wind is swaying. Smooth-interpolated (the fragment takes its
derivatives). ⚠️ On a heightfield surface the attribute is the flat patch point, not the surface: no
hashed material runs there today.

**Two properties worth keeping as tests**, both verified:

- A **thin-walled** material (`thicknessFactor` 0, i.e. no `KHR_materials_volume`) must be a
  **bit-exact no-op** — the ray has zero length. `TransmissionTest` measured max diff **0.0**.
- **IOR 1.0 must not displace anything.** At `eta = 1`, `refract()` returns the incident direction
  unchanged, the exit point lies on the camera ray, and a perspective projection maps every point of
  a camera ray to the same pixel. `TransmissionRoughnessTest` sweeps the IOR by ROW (2.42 / 1.76 /
  1.50 / 1.33 / 1.00) at constant thickness and gives a monotone ladder ending in a 7× collapse:

  | IOR | 2.42 | 1.76 | 1.50 | 1.33 | **1.00** |
  |---|---|---|---|---|---|
  | changed pixels | 12964 | 12659 | 11981 | 10487 | **1615** |

  The residue at 1.00 is the silhouette, where `dot(N, I) > 0` and the identity no longer holds.

**Frosted glass — roughness drives the LOD.** The colour grab pass carries a mip chain
(`src/Graphics/AGENTS.md` § 15c-bis), so a rough transmissive surface reads a blurred copy of the
scene behind it. The mapping is the Khronos reference's, verbatim:

```glsl
lod = log2(float(textureSize(grabPass, 0).x)) * clamp(roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0), 0.0, 1.0);
```

⚠️ The **IOR term is not decoration**: it keeps `ior = 1` (air) perfectly sharp however rough the
surface claims to be — with no refractive interface there is nothing to scatter. That gives the
second free criterion on `TransmissionRoughnessTest`, measured as mean gradient per grid cell:
sharpness collapses along the roughness columns (10-14 down to 0.06) on every IOR row **except**
`1.00`, which stays at 10-13 across all nine columns.

⚠️ The roughness is read from the **texture variable** when a map drives it — the UBO scalar is only
the FACTOR in that case, exactly as the cubemap transmission path resolves it. Reading the UBO
unconditionally would blur a rough-mapped surface uniformly.

⚠️ Total internal reflection makes `refract()` return the **zero vector**; normalising it is a NaN
that poisons the whole sample. `grabPassRefractionOffset()` returns no displacement instead, and
also bails when either clip `w` is ≤ 0 (behind the eye), where the perspective divide is meaningless.

⚠️ The offset is taken as the **screen DELTA** between the fragment's own projected position and the
exit point's, rather than using the projected exit point outright the way the Khronos reference does.
That makes it immune to the TAA sub-pixel jitter — a constant NDC translation carried by both terms,
so it cancels exactly — whereas `gl_FragCoord` already carries the jitter.

### One ambient Fresnel for every branch — `ambientFresnelDeclaration()` (Aug 2026)

The ambient pass has **four** branches that split light with a Fresnel term: refraction,
reflection+transmission, reflection-only, and thin-surface transmission. They had drifted apart on
two axes at once:

- only the **reflection-only** branch knew about iridescence, so a surface that was both iridescent
  and transmissive rendered iridescent in the 26 light passes and plainly dielectric in the ambient
  one — measured on `IridescentDishWithOlives.glb`: 26 light-pass shaders carried
  `evalIridescence`, **zero** ambient shaders did;
- two of them hardcoded `F0 = 0.04` and ignored `KHR_materials_ior` outright, so a glass authored at
  ior 1.7 reflected like ior 1.5.

`LightGenerator::ambientFresnelDeclaration(name, F0Expression, NdotVExpression)` is now the only
place a Schlick term is written, and `dielectricF0Expression()` the only place F0 is derived from the
IOR (weighted by `KHR_materials_specular`'s factor when present). **Never write a Fresnel inline in
an ambient branch again.**

⚠️ **Iridescence REPLACES the Fresnel term, it is not layered on top** — the thin film *is* the
reflectance of the interface. Hence `mix(schlick, evalIridescence(...), iridescenceFactor)`.

⚠️ The term is now a **vec3** everywhere (a film is per-wavelength), so the complements are
`vec3(1.0) - F`, not `1.0 - F`. `mix(vec3, vec3, vec3)` is valid GLSL and carries the film per
channel.

⚠️ The reflection-only branch keeps computing **its own F0**, because it is the metal-aware one —
F0 is mixed toward the albedo by the metalness, which the dielectric-only sites have no use for.
Only the *composition* is shared. That split is deliberate; do not "unify" it further.

### One volume thickness for both consumers — `volumeThicknessExpression()` (Aug 2026)

`KHR_materials_volume`'s `thicknessTexture` (G channel) **multiplies** `thicknessFactor`, and the
result has **two** consumers that must never disagree: Beer's law absorption in the light generator,
and the LENGTH of the refraction ray whose exit point the screen-space refraction projects (three
sites — standard, dispersion, low quality). `StandardResource::volumeThicknessExpression()` resolves
it once.

⚠️ **Depth-based opacity overrides it** with the measured water column from the depth grab — a
different physical quantity. The map has no say there, by design.

⚠️⚠️ **Beer's law takes the WORLD thickness — `volumeThicknessWorldExpression()` (fixed 2026-09-24).**
`KHR_materials_volume` gives `thicknessFactor` in the MESH's space and `attenuationDistance` in WORLD
space. The refraction ray always scaled the factor by `svModelScale`; Beer's law, at all five sites of
`LightGenerator.cpp`, took the raw mesh-space factor — so a scaled mesh absorbed as if it were its
unscaled size. It surfaced on a 40-unit model shown 5 m tall (`forest`'s chick): with the thickness
authored for the chick, the absorption ran over 20 "metres" instead of 2.5, and a near-white diamond
turned navy blue. The light generator now receives `(thickness · dot(svModelScale, vec3(1/3)))`
(exact for a uniform scale), and `generateVertexShaderCode()` requests `ModelScale` for EVERY
transmissive material — both tiers, every transmission path, not only the grab pass
(`StandardResource::declaresTransmission()`, the one condition both sides share). A mesh at scale 1 is
unchanged to the bit.

### One thin-film thickness for every pass — `iridescenceThicknessExpression()` (Aug 2026)

`KHR_materials_iridescence` sets the film thickness as `mix(thicknessMin, thicknessMax, texel.g)`
from the thickness map, and **the MAXIMUM when there is no map** — the spec's fallback, not the
midpoint.

⚠️⚠️ The two passes disagreed: the ambient pass emitted `mix(min, max, 0.5)` while the light passes
emitted `mix(min, max, 1.0)`, so **one surface carried two different films depending on which pass
shaded it**. Both now go through `LightGenerator::iridescenceThicknessExpression()`, which is also
where the map's G channel enters. Never inline the mix again.

⚠️ The map name being **empty is meaningful** — it selects the spec's maximum-thickness fallback. It
is not a neutral value to be defaulted away.

The generated shaders are the check, and a cheap one: every scene shader must show exactly **one
distinct** `iridescenceThickness = ...` line, with no hardcoded `0.5` or `1.0` weight anywhere.

### Transmission belongs to the AMBIENT pass — and it is TINTED BY THE BASE COLOUR

Two rules, both measured on `IridescentDishWithOlives.glb` and `TransmissionTest` (Aug 2026).

**Rule 1 — only the ambient pass composes transmission.** What a viewer sees *through* a surface is
a view-dependent lookup of the scene behind the fragment: the grab pass, or the environment cubemap
in the fallback tier. `generateAmbientFragmentShader()` owns it — it is the pass that declares
`SurfaceTransmissionColor` and splits it against the reflection with a Fresnel term. A punctual
light contributes **nothing** to it; it cannot see what is behind the glass. `LightGenerator.PBR.cpp`
therefore emits **no** transmission term at all, and instead reduces the diffuse lobe:

```glsl
const vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness) * (1.0 - transmissionFactor);
```

That factor is `KHR_materials_transmission`'s `mix(diffuse_brdf, specular_btdf, transmissionFactor)`
seen from the diffuse side. ⚠️ The light passes used to ADD
`albedo * beerAbsorption * max(dot(-N, L), 0) * transmissionFactor * radiance` on top of an
**undiminished** diffuse — an addition where the spec asks for a mix, using the albedo where the
transmitted colour is the scene behind. Under the model viewer's 100 000 lux key it swamped the
ambient pass's correct result and made every transmissive material milky white. Do not reintroduce
it: a back-lit look for leaves, paper or skin is **subsurface scattering**, which has its own term
and its own material component.

**Rule 2 — the transmitted light is multiplied by the base colour.** The Khronos reference
(`getIBLVolumeRefraction()` in the glTF Sample Viewer) composes
`(1 - F) * attenuatedColor * baseColor`. All three ambient transmission sites in
`LightGenerator.cpp` apply `albedoShaderExpression()`.

⚠️⚠️ **Beer's law is not a substitute for that tint, and assuming it was hid the gap for months.**
Beer needs `KHR_materials_volume`; a material that declares only `KHR_materials_transmission` has
`attenuationDistance = +INFINITY`, so `exp(log(colour)/inf)` is exactly 1 and the absorption is a
no-op. Every `TransmissionTest` sphere is such a material: with rule 1 applied but not rule 2 they
rendered as colourless ghosts — the yellow, green, red and blue came back only with the base-colour
factor. Before rule 1, the tint arrived **by accident** through the additive albedo term, which is
why the gap was invisible.

### MRT normal output — the `N` declaration contract

`SceneRendering` writes the view-space perturbed normal to the G-buffer normal
attachment (`svOutputNormal`) for the **`AmbientPass`** (and the `SimplePass` output slot, which
is unlit and never reaches the light generator), using
`LightGenerator::finalNormalViewSpaceExpression()`. That helper returns the bare identifier
**`N`** whenever normal mapping is active — so `N` **must be declared** for the ambient pass:

- **`AmbientPass`** never reaches a light-pass generator (it returns right after
  `generateAmbientFragmentShader()`, which does *not* declare `N`). So `generateFragmentShaderCode()`
  declares `N` up-front, at the top of the function, for the ambient pass.
- Light passes either self-declare their own `N` (high-quality PBR, two-sided-flipped), shade in
  tangent space without a view-space `N` (Blinn-Phong with normal map), or compute lighting
  per-vertex (low-quality Gouraud).

> **History (Jul 2026)**: the `SimplePass` used to be REMAPPED by `checkRenderPassType()` to a
> light-pass type when the scene used the *static lighting* mode (a single light baked as GLSL
> literals). That whole mode was REMOVED — `SimplePass` is now strictly unlit (light set
> disabled or instance lighting disabled), `checkRenderPassType()` is gone, and the `N`
> declaration guard covers the `AmbientPass` only.

> [!CAUTION]
> **When you touch a lighting shader, always check BOTH quality levels
> (`Core/Graphics/Shader/EnableHighQuality`, default `false`).** High and low quality route
> through *different* generators (per-fragment PBR/Blinn-Phong vs per-vertex Gouraud), so a
> change that compiles in one can break the other — a variable declared by the high-quality
> fragment path (e.g. `N`, `ViewTBNMatrix`) may be entirely absent in the low-quality Gouraud
> path. Test with `EnableHighQuality` both `true` and `false`.
>
> This particular bug is also **latent until a post-process effect enables the normal G-buffer
> attachment**. A `SimplePass` material (static single light) with a normal map compiles fine
> with no post-process, then fails to compile (`'N' : undeclared identifier`, `redefinition`
> for high-quality PBR, or `'ViewTBNMatrix' : undeclared identifier` in low-quality Gouraud)
> the moment SSAO/SSR/RTR/RTGI turns the attachment on. If you add a shading family or a pass
> that writes `svOutputNormal`, keep the `N`-declaration guard in `generateFragmentShaderCode()`
> and the `ViewTBNMatrix` request in `generateVertexShaderCode()` in sync. See
> `docs/caution-points.md`.

### Shader Program Variants

Each `RenderPassType` generates a **distinct shader program** with only the needed code:

| Pass Suffix | Per-Light Samplers | Bindless Arrays | Texture Samples |
|-------------|-------------------|-----------------|-----------------|
| (base) | 0 | 0 | 0 |
| `ShadowMap`/`CSM` | 1 (shadow) | 0 | 1 |
| `ColorMap` | 0 | 1 (2D or Cube) | 1 |
| `Full`/`FullCSM` | 1 (shadow) | 1 (2D or Cube) | 2 |

When color projection is not active, `projectionColor = vec3(1.0)` is hardcoded — no texture sample, and the SPIR-V compiler optimizes out the multiply.

### Bindless Color Projection Sampling

Color projection textures are accessed via the global `BindlessTextureManager` descriptor set, **not** via per-light descriptor sets. The light UBO carries a `uint` bindless index (`ColorProjectionIndex`) encoded as `std::bit_cast<float>(uint32_t)`:

```glsl
// In fragment shader (generated by LightGenerator):
uint cpIdx = floatBitsToUint(uLight.ColorProjectionIndex);
if ( cpIdx != 0xFFFFFFFFu ) {
    // 2D lights (directional, spot):
    projectionColor = texture(uBindlessTextures2D[nonuniformEXT(cpIdx)], projCoords.xy).rgb;
    // Point lights (cubemap) — NEGATED, see the CAUTION below:
    projectionColor = texture(uBindlessTexturesCube[nonuniformEXT(cpIdx)], -DirectionWorldSpace.xyz).rgb;
}
```

> [!CAUTION]
> **A point-light gobo is sampled along `-DirectionWorldSpace`, never the raw vector.**
> `DirectionWorldSpace` is the **FRAGMENT → LIGHT** vector; a projection texture must be sampled
> along the direction the light **EMITS**, light → fragment. The two are antipodal, so the raw
> vector returns the **opposite cubemap face** — and it did, from the day the gobo was written
> (0.8.6, Feb 2026) until Aug 2026.
>
> Measured with `global-illumination --demo-options 0,1`, which projects the `AxisDebug` cubemap
> from the room's only omni light: the ceiling above the light read **MAGENTA (-Y)** and the floor
> **GREEN (+Y)** — exactly swapped. After negating, at the same two camera poses: ceiling green,
> floor magenta.
>
> ⚠️ **This is NOT a Y-up residual**, and it would have been misfiled as one. `git log -S` puts the
> line at 0.8.6 (2026-02-20) with no Y negation ever present, six months before the flip. Check the
> history before filing a sign error under the migration that happens to be nearby — the same
> mistake was made on SSR's camera-ward ray rejection the same week.
>
> ⚠️ The point-light SHADOW lookup takes the SAME vector and negates it inside
> `generate3DShadowMapCode()`, for the same reason. When that one was fixed, this site — six lines
> away in the same generator, fed by the same variable — was not checked. **A sign fix on a
> direction is a cue to audit every other consumer of that variable**, not just the one that
> produced the visible symptom.

The bindless array declarations use `Declaration::Sampler::UnboundedArray` and are placed on the `PerBindless` set. The `GL_EXT_nonuniform_qualifier` extension is required.

**Why bindless?** Per-light descriptor sets use `UNIFORM_BUFFER_DYNAMIC` (binding 0), which does not support `UPDATE_AFTER_BIND_BIT`. This makes deferred texture writes unsafe with frames-in-flight. The bindless set uses `UPDATE_AFTER_BIND_BIT` + `PARTIALLY_BOUND_BIT`, allowing textures to be registered asynchronously after resource loading completes.

### Declaration de-duplication contract

Several declaration kinds are **global, fixed-identity resources** that multiple
composable generators legitimately declare into the *same* shader without
coordinating. For these, `declare()` is **silently idempotent** — a re-declaration
of the same name is byte-identical, returns `true`, and emits **no warning**:

- **Vertex input attributes** (`VertexShader::declare(const Declaration::InputAttribute &)`).
  Name, location and GLSL type are all derived from the `VertexAttributeType`, so a
  duplicate cannot conflict. The `synthesize*` / TBN helpers in `AbstractVertexStage.cpp`
  and the `Generator/*` passes each declare what they consume.
- **Unbounded bindless arrays** (`AbstractShader::declare(const Declaration::Sampler &)`
  when `declaration.isUnbounded()`). A fixed name maps to a fixed set/binding/type
  (e.g. `uBindlessTexturesCube` → cube binding on the `PerBindless` set). They are
  declared **independently** by the material (`StandardResource`, at each of its feature
  sites) **and** the `LightGenerator` variants (cube shadows, color projection) into one fragment
  shader — there is **no single coordinator** across those subsystems, so a localized
  "declare once" cannot cover it. Silent de-dup is the mechanism.

**Do not re-introduce a warning or a `quiet`/once-guard for these** — it only
produced log spam (hundreds of lines per program build) with zero actionable
signal. **Bounded / named samplers still warn** on duplicates: there a
same-name / different-binding clash is a real bug worth catching.

> [!NOTE]
> Every subsystem declares the bindless arrays **on use** (each `StandardResource` feature
> site, each `LightGenerator` variant), unconditionally — no up-front "declare once"
> coordinator and no guards. There is no shared owner across material ↔ LightGenerator,
> so the silent de-dup above is what keeps a single declaration in the shader and the log
> clean. Do not add a localized once-guard back; it cannot cover the cross-subsystem case
> and only fragments the pattern.

### ScaleBiasMatrix UV Caveat

> [!WARNING]
> **Do NOT apply `* 0.5 + 0.5` to color projection UVs!**
>
> `ScaleBiasMatrix` is pre-multiplied into `ViewProjectionMatrix` in the UBO. Shadow maps use `textureProj()` which handles this automatically. Color projection does manual perspective divide (`projCoords = .xyz / .w`), and the UVs are already in [0,1] range.
>
> Adding `* 0.5 + 0.5` causes double-bias (pattern offset). See: `docs/shadow-mapping.md` Color Projection section.

### Helper Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `renderPassUsesColorProjection()` | `Graphics/Types.hpp` | True for `*ColorMap`, `*Full`, `*FullCSM` |
| `renderPassUsesShadowMap()` | `Graphics/Types.hpp` | True for `*ShadowMap`, `*CSM`, `*Full`, `*FullCSM` |
| `renderPassUsesCSM()` | `Graphics/Types.hpp` | True for `*CSM`, `*FullCSM` |

**Code references:**
- `LightGenerator.cpp:generateVertexShaderCode()` — Switch with fallthrough for enableColorProjection
- `LightGenerator.cpp:generateFragmentShaderCode()` — Switch with fallthrough for enableColorProjection
- `LightGenerator.PerFragment.cpp` — Bindless color projection sampling (all 4 shading variants)
- `Graphics/BindlessTextureManager.hpp` — `Texture2DBinding` (1), `TextureCubeBinding` (3) constants
- `Graphics/Types.hpp:RenderPassType` — Enum definition (16 values)
- `Scenes/Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` — Async texture registration
