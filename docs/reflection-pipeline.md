# Reflection Pipeline

> Everything the engine can put in a reflection, from the legacy lerp to the hardware ray query,
> and — more importantly — **how the seven paths arbitrate between themselves**.

> [!CRITICAL]
> Three contracts in this document are not written anywhere else in the code base, and each of
> them has already cost a debugging session:
> 1. The **reflectivity nibble** (`matProps.R`, high 4 bits) is the ONLY channel by which a
>    material tells the screen-space effects "reflect me". Section 4.
> 2. The **normals-buffer alpha** packs `roughness + metalness * 2.0`, and the decode side
>    **binarizes the metalness**. Section 5 — this is a live defect, read it before trusting
>    any SSR/RTR result on a partially-metallic material.
> 3. SSR and RTR **composite by `mix()`**, i.e. they SUBSTITUTE the pixel, they do not add a
>    specular lobe. Section 4.3.

**Validation bench:** every behaviour described here is exercised by the `reflexion-debug`
scene in projet-alpha (`src/Builtin/ReflexionDebug.cpp`) — owner design: ONE reflective
subject (sphere or cube) on a neutral floor, sky-driven lighting, KeyPad3 sky cycling, a palm
tree and the animated glTF dragon around the subject as living reflection content. Option #2
selects the reflection type under test. When you change anything in this document's scope,
re-run that scene before and after.

---

## 1. The seven paths

Ordered from the crudest simulation to the most exact evaluation.

| # | Path | Entry point | Angular fidelity | Cost |
|---|------|-------------|------------------|------|
| 1 | **Legacy lerp** | `StandardResource::setReflectionComponent(texture, amount)` | none — a flat `mix()`, no Fresnel, no roughness | 1 fetch |
| 2 | **Static cubemap + Fresnel** | same, high-quality shader path | Fresnel-Schlick only, perfect mirror | 1 fetch |
| 3 | **IBL split-sum** | `PBRResource::setReflectionComponentFromEnvironmentCubemap(iblIntensity)` | correct in roughness, **energy conserving** | 2 fetches |
| 4 | **Dynamic cubemap probe** | `Scene::createRenderToCubemap` + `set*ComponentFromRenderTarget` | perfect mirror only (no prefilter) | 6 full scene passes / frame |
| 5 | **SSR** | `Effects::Framebuffer::SSR` in the post-process stack | hard cutoff at `roughness > 0.5` | 5 passes, half-res |
| 6 | **RTR** | `Effects::Framebuffer::RTR` in the post-process stack | fade over `roughness ∈ [0.45, 0.7]` | 4 passes, half-res by default |
| 7 | **Grab-pass transmission** | `PBRResource::setTransmissionComponent` | refraction side of the same Fresnel split | grab pass |

Paths 1-4 are **material** features, resolved in the object's ambient pass. Paths 5-6 are
**post-process** features, resolved on the G-buffer after the scene is lit. They are not
alternatives to each other — they stack, and section 4 explains how.

---

## 2. Material-side paths (1-4)

### 2.1 The `Reflection` component and its filling types

All four material paths come from one component, `ComponentType::Reflection`, whose
`FillingType` selects the behaviour:

```json
{ "Reflection": { "Type": "Automatic", "Amount": 0.8 } }   // scene environment cubemap
{ "Reflection": { "Type": "Cubemap", "Data": { "Name": "Reflections/Chromic" } } }
{ "Reflection": { "Type": "Value", "Amount": 0.5 } }        // NO cubemap — SSR/RTR only
```

| `Type` | Standard | PBR | Effect |
|--------|----------|-----|--------|
| `Automatic` | `m_isUsingEnvironmentCubemap = true`, `Amount` → `reflectionAmount` | idem, `Amount` → `iblIntensity` | samples the scene's **prefiltered** cubemap (bindless reserved cube slot 2) |
| `Cubemap` / `Texture` | dedicated sampler | dedicated sampler | samples that texture, **mip 0 only** |
| `Value` | `m_postProcessReflectivityAmount` | idem | **no cubemap at all** — only writes the reflectivity nibble, so the surface reflects exclusively through SSR/RTR |
| `None` | — | — | no reflection |

> [!IMPORTANT]
> `Type: "Value"` is the only way to author a surface whose reflection comes **purely** from
> the post-process stack. It is the isolation switch when you need to tell a cubemap artefact
> from an SSR/RTR artefact. The C++ mirror of this filling type is
> `setPostProcessReflectivity(amount)` on both `StandardResource` and `PBRResource`.

### 2.2 Path 3 — the IBL split-sum, the only energy-correct one

Generated in `LightGenerator::generateAmbientFragmentShader()` (`Saphir/LightGenerator.cpp`),
**ambient pass only** — the light passes write nothing to it.

Karis 2013 split-sum, completed by the Fdez-Agüera 2019 multi-scatter compensation, using the
three reserved bindless slots baked by `Compute::IBLBaker`:

| Slot | Array | Content |
|------|-------|---------|
| 1 | cube | irradiance, 32², RGBA16F, stores **E/π** |
| 2 | cube | GGX-prefiltered environment, 128², 6 mips, one roughness per mip |
| 3 | 2D | split-sum BRDF LUT, 128², RGBA16F, scale/bias on F0 in RG |

```glsl
const vec2 iblEnvBRDF = texture(textures2D[BRDFLutSlot], vec2(NdotV, roughness)).rg;
const vec3 iblFssEss  = iblF0 * iblEnvBRDF.x + iblEnvBRDF.y;
const float iblEms    = 1.0 - (iblEnvBRDF.x + iblEnvBRDF.y);
const vec3 iblFavg    = iblF0 + (vec3(1.0) - iblF0) / 21.0;
const vec3 iblFmsEms  = iblEms * iblFssEss * iblFavg / (vec3(1.0) - iblFavg * iblEms);
const vec3 iblKD      = albedo * (1.0 - metalness) * max(vec3(1.0) - iblFssEss - iblFmsEms, vec3(0.0));

fragColor.rgb += iblFssEss * reflectedColor * iblIntensity;
fragColor.rgb += (iblFmsEms + iblKD * ao) * iblIrradiance * iblIntensity;
```

The diffuse lobe takes exactly what the specular did not — that is the energy conservation.

The roughness → mip mapping is
`clamp(roughness, 0, 1) * (IBLTexture::PrefilteredMipLevels - 1)`, i.e. `* 5.0`.

> [!CAUTION]
> **`iblIntensity` is scaled by the scene's environment luminance** (`scaledIBLIntensity()`).
> The cubemap is a normalized [0,1] source; without that scale a reflection contributes a
> fraction of a nit in a scene lit in thousands. Corollary already documented in
> [`caution-points.md`](caution-points.md): multiply by `environmentLuminance` ONLY what comes
> out of the cubemap — anything re-read from the rendered scene (grab pass, screen-space
> capture) is already an absolute luminance.

### 2.3 Path 4 — the dynamic cubemap probe

```cpp
const auto probe = toolkit.generateEnvironmentCubemapRenderer("MyProbe", 512, 5000.0F, false);
material.setReflectionComponentFromRenderTarget(probe.second);
```

`Toolkit::generateEnvironmentCubemapRenderer()` pairs a 6-face cubemap camera with a
`RenderTarget::Texture< ViewMatrices3DUBO >` registered in the scene's render-to-texture set.
`Renderer::renderRenderToTextures()` replays **opaque + translucent + translucentGB** on all six
faces.

**Update policy (Aug 2026).** The render target's `automaticRendering` / `renderOutOfDate`
contract is now HONORED by the Renderer (it existed with no consumer — every target re-rendered
every frame whatever its flags):

| Policy | Setup | Behaviour |
|--------|-------|-----------|
| Continuous (default) | `setAutomaticRenderingState(true)` — set by `createRenderToCubemap` | re-rendered every frame |
| Once | `setAutomaticRenderingState(false)` + `setRenderOutOfDate()` | one bake, then stops; `setRenderOutOfDate()` re-bakes on demand |
| Suspension | `setSuspendableByPostProcessReflections(true)` — default for cubemap probes | while an ENABLED reflection provider (SSR/RTR, `providesReflections()`) is in the scene stack, the probe is suspended AFTER one guaranteed render — the traced reflection does its job better; the materials keep a stale-but-real bake |

> [!CAUTION]
> **Four measured defects of this path (Aug 2026, `reflexion-debug` + `offscreen-rendering`
> benches — diagnosis owner-driven), fix lot planned:**
> 1. ✅ **FIXED — the reflection sample was re-multiplied by `environmentLuminance`.** A probe's
>    content is the RENDERED SCENE — already an absolute luminance — but the shader applied the
>    normalized-cubemap contract (the rule already existed for grab-pass transmission:
>    `m_transmissionIsSceneRadiance`). Now the materials flag a render-target source
>    (`declareReflectionSourceAbsolute()` / refraction variant) and every reflected/refracted
>    leg goes through `LightGenerator::reflectionIntensity()` / `refractionIntensity()` —
>    artistic weight alone for an absolute source, weight × environment luminance for a
>    normalized cubemap. The sky-derived legs (irradiance, split-sum compensation) keep the
>    scale. Measured on the Backrooms bench: sphere saturation 47 % → 0 %, luminance coherent
>    with the scene, reflected ceiling grid visible at the right energy.
> 2. ✅ **FIXED — the probe target was LDR (RGBA8_SRGB).** `Instance::findColorFormat()` IGNORED
>    the requested bits (commented-out parameters) and locked every render target to
>    `R8G8B8A8_SRGB`: a photometric scene clamped to 1.0 wholesale. It now honors a 16+ bit
>    request (`R16G16B16A16_SFLOAT`, SRGB fallback), the cubemap `RenderTarget::Texture` ctor
>    takes the color bits, and `Scene::createRenderToCubemap()` defaults probes to **16-bit HDR**.
>    Validated under the 100 klx BlueSky: full scene (ground, palm, dragon, sun) in the probe
>    reflection at the right energy, instead of the flat white veil. Owner-validated.
> 3. ✅ **FIXED — the "once" bake fired at frame 1** (black, unlit scene frozen forever) and
>    "once" had no refresh story. The contract is now EVENT-DRIVEN (owner decision): "once"
>    means "not every frame", not "never again". The ENGINE signals a re-bake on its own
>    events — a renderable instance becoming ready (async load materialized,
>    `Scene::signalOnDemandRenderTargets()`) and a background switch; the APPLICATION signals
>    its specific changes (a particular movement) via `setRenderOutOfDate()` on the target.
>    ⚠️ The signal is DEFERRED (atomic flag consumed by `beginRenderFrame()`, outside any
>    lock): it fires from inside the Renderer's render-to-textures loop, which holds the
>    render target list mutex — walking the lists there self-deadlocked the render thread.
>    Validated: once-probe shows the full scene with the animated dragon FROZEN in its
>    last-event pose while the live one animates beside it.
> 4. ✅ **FIXED — the subject was rendered into its own probe** (inception feedback, visible in
>    `offscreen-rendering`). `RenderTarget::Abstract` now carries a rendering EXCLUSION LIST
>    (opaque renderable-instance keys, `excludeFromRendering()`), consulted by the single
>    populate gate (`Scene::checkRenderableInstanceForRendering`). The caller registers its
>    subject after creation — first brick of the future probe system. ⚠️ Lifetime is the
>    caller's: exclusions are not cleaned when an instance dies. Validated: the city crosses
>    the offscreen-rendering bronze sphere's reflection continuously, no inception disc.
>
> Still true besides: **no roughness response** (bare `texture()`, no mip chain, no GGX
> convolution — a `roughness = 0.35` metal reflects like polished chrome), no per-face
> amortisation, no LOD.

### 2.4 The reflection cost ladder (owner design)

The material-side modes order themselves by GPU cost, and the arbitration with the
post-process rungs is SEMANTIC, not additive — one specular lobe, one radiance source per
pixel, cheapest source that contains the needed information:

| Mode | Source | Overridable by SSR/RTR? |
|------|--------|--------------------------|
| Texture (explicit cubemap) | authored texture | **NEVER** — artistic intent, publishes a ZERO reflectivity nibble (`LightGenerator::declareReflectionArtistic()`) |
| Auto (scene environment) | prefiltered sky IBL | yes — the trace is MORE scene-coherent than the cubemap |
| Camera probe (once / continuous) | rendered scene probe | yes — same information, better evaluated; the probe re-render is suspended meanwhile |
| Post-process (SSR/RTR) | traced scene | — (falls back to the environment on miss) |

An explicit `ReflectivityMap` keeps priority over the artistic rule: an artist asking for
per-pixel post-process control is obeyed.

---

## 3. Post-process paths (5-6)

Both are `IndirectPostProcessEffect`s. Both declare
`requiresDepth / requiresNormals / requiresMaterialProperties / requiresHDR`; RTR additionally
declares `requiresRayTracing`. Those flags are what makes `Renderer` allocate the corresponding
G-buffer attachments — see `Graphics/AGENTS.md` § "G-Buffer MRT (fixed order)".

They are **mutually exclusive by convention**, chosen at stack construction:

```cpp
if ( RTEnabled && settings.getOrSetDefault< bool >(GraphicsRayTracingReflectionEnabledKey, true) )
    stack->addEffect(std::make_shared< RTR >(renderer, RTR::Parameters{...}));
else
    stack->addEffect(std::make_shared< SSR >(renderer, SSR::Parameters{...}));
```

Nothing in the engine forbids adding both; nothing hybridises them either.

### 3.1 SSR — 5 passes

| Pass | Target | What it does |
|------|--------|--------------|
| 1 Trace | half-res RGBA16F | linear ray march in reconstruction space, adaptive stride, ≤ `maxSteps` (128), then 8 binary refinement steps. Outputs `(hitUV.xy, confidence, 0)` |
| 2 Resolve | half-res | converts `hitUV` → reflected colour by sampling the scene colour; on a miss, falls back to the **prefiltered** cubemap (reserved cube slot 2) weighted by `envFallbackIntensity` |
| 3-4 Blur | half-res | separable 5-tap gaussian |
| 5 Composite | full-res | see section 4.3 |

Confidence = `distFade · edgeFade · facingFade · roughnessFade`, with
`roughnessFade = 1 - smoothstep(0, 0.4, roughness)` and an early-out at `roughness > 0.5`.

### 3.2 RTR — 4 passes

| Pass | Target | What it does |
|------|--------|--------------|
| 1 Trace | half-res (full-res if `RayTracing/Reflection/PixelDoubling` = false) | one mirror ray per pixel against the TLAS via `rayQueryEXT` |
| 2-3 Blur | half-res | separable **bilateral** filter, depth + normal aware, radius scaled by roughness |
| 4 Composite | full-res | depth-aware 4-tap upsample, then section 4.3 |

The trace pass is the interesting one:

- **Ray flags are `NoneEXT`, not `OpaqueEXT`**, so candidate hits on `FORCE_NO_OPAQUE` instances
  (alpha-test materials) come back for confirmation. The shader samples the opacity — or the
  albedo alpha — at the candidate barycentrics and confirms only above `alphaCutoff`. This is
  what lets rays pass through foliage and sprite texels.
- **Material lookup is per sub-geometry**: `getHitMaterialIndex(instanceIndex, geomIdx)` reads
  `GPUMeshMetaData[2][geomIdx]`, clamping to 0 when the BLAS has more sub-geometries than the
  renderable has material slots (procedural sprite quads).
- **Hit shading is Lambert only**: `albedo * (directLighting + sceneAmbient)`. Each light's
  contribution is gated by a shadow ray, but **only for lights that cast shadows in the raster
  passes** — a light without a shadow map deliberately shines through geometry on screen, and
  the reflection must match the image.
- **On a miss**, the environment cubemap is sampled along `reflDir`.

> [!NOTE]
> **Engine cubemap convention.** A world direction `D` samples the cubemap at
> `vec3(D.x, -D.y, D.z)` — engine UP is -Y, cubemaps are stored Y-up. This holds in the skybox,
> in the material reflection code and in both post-process miss branches. Get it wrong and the
> reflection is vertically mirrored.

---

## 4. Arbitration — the part that is not obvious

### 4.1 The reflectivity nibble

`LightGenerator::materialPropertiesExpression()` (`Saphir/LightGenerator.cpp`) encodes a single
scalar into the **high 4 bits of `matProps.R`**, and that scalar is the entire vocabulary a
material has for talking to SSR/RTR.

Priority, first match wins:

| # | Condition | Expression |
|---|-----------|------------|
| 1 | dedicated `ReflectivityMap` component | `clamp(reflectivityMap, 0, 1)` |
| 2 | PBR + `iblIntensity` + reflection | `clamp(max(iblIntensity·(1-roughness), metalness), 0, 1)` |
| 3 | PBR, no explicit reflection | `clamp(metalness·(1-roughness), 0, 1)` |
| 4 | Standard with reflection | `clamp(reflectionAmount, 0, 1)` |
| — | otherwise | `0.0` |

Packing: `float(uint(reflectivity * 15.0) << 4u) / 255.0`.

> [!CAUTION]
> Two consequences. **15 levels**, so a smooth roughness ramp banded into fifteen steps in the
> reflection strength. And the conversion **truncates** (`uint(x * 15.0)`, no `+ 0.5`), so the
> quantisation is biased downward: `reflectivity = 0.99` encodes as 14/15.

`matProps` is `R8G8B8A8_UNORM` and only the ambient/simple pass writes it; light passes write
`vec4(0, 0, 0, 1)` — **not** `vec4(0)`, see
[`caution-points.md`](caution-points.md) on alpha preservation.

### 4.2 The normals-buffer alpha

The normals attachment is `R16G16B16A16_SFLOAT`; RGB is the view-space normal, and alpha carries
roughness and metalness:

```glsl
/* Saphir/Generator/SceneRendering.cpp — ambient/simple pass only. */
outNormal = vec4(normalExpr, roughnessExpr + metalnessExpr * 2.0);
```

```glsl
/* RTR.cpp / SSR.cpp — decode. */
float originMetalness = packedRM >= 2.0 ? 1.0 : 0.0;
float roughness       = packedRM - originMetalness * 2.0;
```

> [!CAUTION]
> **The encode is continuous, the decode is binary. Any metalness strictly between 0 and 1
> destroys both values.** `metalness = 0.5`, `roughness = 0.3` → `alpha = 1.3` → decoded as
> `metalness = 0`, `roughness = 1.3`. SSR then early-outs on `roughness > 0.5`; RTR early-outs
> on `roughnessFade <= 0`. **The reflection disappears silently**, with no warning anywhere.
> This bites hardest on glTF assets, whose metallicRoughness textures routinely carry
> intermediate values on wear, edges and blends. Bench: `reflexion-debug`, driving the subject
> material's metalness between 0 and 1.

### 4.3 The composite, and why it is not a lobe

Identical in both effects:

```glsl
uint  rPacked      = uint(mp.r * 255.0 + 0.5);
float reflectivity = float(rPacked >> 4u) / 15.0;
float confidence   = refl.a;

if (confidence > 0.001 && reflectivity > 0.0)
    color.rgb = mix(color.rgb, refl.rgb / max(confidence, 0.001), confidence * intensity * reflectivity);
```

> [!CAUTION]
> **This is a substitution, not an addition.** By the time it runs, the pixel already holds the
> direct lighting plus the physically-correct split-sum IBL specular from the ambient pass. The
> `mix()` lerps the WHOLE pixel — diffuse included — toward a Lambert-shaded ray colour. At
> `reflectivity = 1`, `intensity = 0.8` and a grazing Fresnel, almost nothing of the computed
> shading survives.
>
> An energy-correct composite would **add** the traced specular lobe and **subtract** the
> corresponding IBL specular term it replaces. That requires the ambient pass to publish its
> specular contribution separately, which it does not do today.

Note also that the reflected colour is never tinted by the **reflector's** F0. A gold surface
reflects in white: the metal's coloured Fresnel is applied to the IBL path (section 2.2) but
not to the SSR/RTR path.

### 4.4 Non-physical F0 floors

Two places deliberately raise F0 above the physical dielectric value of 0.04, both commented as
"boosted for visibility":

| Site | Expression | Physical value |
|------|------------|----------------|
| `LightGenerator.cpp`, IBL, no material IOR | `iblF0 = mix(vec3(0.5), albedo, metalness)` | 0.04 |
| `RTR.cpp`, trace pass | `F0 = mix(0.15, 0.9, originMetalness)` | 0.04 / albedo |

When the material declares an IOR (`KHR_materials_ior`) the IBL path computes the correct
`((ior-1)/(ior+1))²` instead. RTR never does.

---

## 5. Reading a reflection artefact

| Symptom | Most likely cause |
|---------|-------------------|
| No reflection at all on a partially-metallic material | § 4.2 — the metalness packing. Check with `metalness` set to exactly 0 or 1 |
| Reflection strength banded in visible steps along a roughness ramp | § 4.1 — the 4-bit nibble |
| Diffuse colour washed out on a strong reflector | § 4.3 — the `mix()` substitution |
| Gold / copper reflects in white | § 4.3 — no F0 tint on the SSR/RTR path |
| Reflection vanishes at the screen border | expected: `fadeScreenEdge`. Confirm with RTR, which does not depend on screen coverage |
| Reflection shows the sky where geometry should be | the ray escaped: SSR miss → cubemap fallback, or RTR miss → cubemap. Switch to a visually distinctive sky (`reflexion-debug` KeyPad3) to see which source leaks |
| Reflection too bright, unrelated to the scene | § 2.2 — an `environmentLuminance` multiply applied to something already in absolute luminance |
| A mirror-sharp reflection on a rough dynamic probe | § 2.3 — the probe has no prefilter |
| Everything reflects the interior of the skybox | the background mesh is in the TLAS — regression of `6a8282b0` |

**Isolation procedure.** Set the material's reflection to `Type: "Value"` (or call
`setPostProcessReflectivity()`): the cubemap paths disappear and only SSR/RTR remain. Then
disable the post-process effect: nothing should remain. Anything still visible comes from the
ambient pass.

---

## 6. What does not exist

Stated explicitly so nobody looks for it:

- **No reflection probe SYSTEM yet** — but the primitives are no longer raw (Aug 2026): HDR
  target, once/continuous/event-refresh policies, SSR/RTR suspension, per-target exclusion
  list. Still missing for a real system: placement policy, parallax correction (box/sphere
  projection), blending between probes, GGX convolution of the result (a probe reflection is
  mirror-sharp whatever the roughness), per-face amortised update.
- **No planar reflections.** Nothing in the code base; `grep -i planar` only matches UV mapping.
- **No SSR/RTR hybrid.** No "screen-space first, ray-traced on miss" path.
- **No Hi-Z marching** for SSR.
- **No temporal accumulation on reflections.** RTGI has one; SSR and RTR have spatial blur only.
  No reprojection, no ray reuse, no dedicated variance-guided denoiser.
- **No GGX lobe sampling.** One mirror ray, blurred afterwards. A post-hoc blur is not a lobe.

---

## Link Index

| Document | Path |
|----------|------|
| Graphics system (AGENTS) | [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) |
| Saphir shader system | [`docs/saphir-shader-system.md`](saphir-shader-system.md) |
| Caution points | [`docs/caution-points.md`](caution-points.md) |
| Render targets | [`docs/render-targets.md`](render-targets.md) |
| Graphics system overview | [`docs/graphics-system.md`](graphics-system.md) |

| Code | Path |
|------|------|
| Reflectivity nibble + IBL GLSL | `src/Saphir/LightGenerator.cpp` |
| Normals/matProps MRT writes | `src/Saphir/Generator/SceneRendering.cpp` |
| Material reflection components | `src/Graphics/Material/{Standard,PBR}Resource.cpp` |
| IBL baked textures | `src/Graphics/IBLTexture.{hpp,cpp}`, `src/Graphics/Compute/IBLBaker.*` |
| SSR / RTR | `src/Graphics/Effects/Framebuffer/{SSR,RTR}.cpp` |
| Dynamic probe | `src/Scenes/Scene.rendering.cpp`, `src/Scenes/Toolkit.hpp` |
| Validation scene | projet-alpha `src/Builtin/ReflexionDebug.cpp` |
</content>
</invoke>