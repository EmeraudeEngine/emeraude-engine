# Reflection Pipeline

> Everything the engine can put in a reflection, from the static cubemap to the hardware ray query,
> and — more importantly — **how the six paths arbitrate between themselves**.

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
| 3 | **IBL split-sum** | `StandardResource::setReflectionComponentFromEnvironmentCubemap(iblIntensity)` | correct in roughness, **energy conserving** | 2 fetches |
| 4 | **Dynamic cubemap probe** | `Scene::createRenderToCubemap` + `set*ComponentFromRenderTarget` | GGX-prefiltered mip chain, roughness-driven LOD | 6 full scene passes / frame + convolution |
| 5 | **SSR** | `Effects::Framebuffer::SSR` in the post-process stack | **cone-traced glossy** (color pyramid LOD), fade over `roughness ∈ [0.55, 0.85]` | 5 passes + color pyramid |
| 6 | **RTR** | `Effects::Framebuffer::RTR` in the post-process stack | **glossy via reflection pyramid** (roughness² LOD, assumed hit distance — over-blurs curved reflectors, § 3.2.1), fade over `roughness ∈ [0.6, 0.9]` | 4 passes + reflection pyramid |
| 7 | **Grab-pass transmission** | `StandardResource::setTransmissionComponent` | refraction side of the same Fresnel split | grab pass |

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
> `setPostProcessReflectivity(amount)` on both `StandardResource` and `StandardResource`.

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

**GGX convolution (Aug 2026).** A probe is no longer mirror-sharp whatever the material: its
color cubemap carries a **GGX-prefiltered mip chain** (`enableGGXConvolution()`, set by
`createRenderToCubemap`). Mip 0 stays the NATIVE mirror render (512² untouched — no
resolution regression); after every render, `Compute::ProbeConvolver` records — in the
target's own command buffer, no blocking submit — a blit cascade into a private half-size
scratch cubemap (the prefilter's filtered importance sampling needs a PLAIN mip chain on its
source; reading the probe's own prefiltered mips would be a hazard AND a bias), then one
borrowed IBLBaker prefilter dispatch per upper mip (roughness k/(mips−1), the sky IBL chain
semantics). Materials with a render-target reflection component sample
`textureLod(probe, R, roughness × (mips−1))` — Standard maps Shininess through Beckmann
(`√(2/(s+2))`), PBR uses its roughness (component or scalar) directly. The probe uses a
dedicated TRILINEAR sampler (`maxLod` unclamped — the shared render-to-texture sampler honors
the settings mip level, default 1, which would truncate the chain). Static texture cubemaps
keep the plain mirror fetch: the explicit texture mode is ARTISTIC by contract.
Validated on `reflexion-debug` option #3 (Polished/Aluminium × all modes): the aluminium
probe blurs palm/dragon/floor physically, the polished one keeps the mirror.

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
>    ✅ **FIXED — the once-bake could capture an UNLIT world** (measured on the automated
>    tour: pitch-black floor in the mode-3 probe, palm and dragon fine). Two lighting
>    events were NOT part of the re-bake contract: `applyBackgroundLightingNow()` (sun +
>    ambient — deferred until the ASYNC background resource loads, a window of seconds)
>    and the environment IBL bake/publication (the ambient pass reads the irradiance slot;
>    before publication it parks on the default BLACK cubemap while the applyAmbient
>    contract zeroes the scalar — a bake in that window records a black floor with ZERO
>    warnings, every program exists). Both now fire `signalOnDemandRenderTargets()`:
>    whatever the load race, the LAST re-bake always happens with the sun, the ambient
>    and the IBL in place.
> 4. ✅ **FIXED — the subject was rendered into its own probe** (inception feedback, visible in
>    `offscreen-rendering`). `RenderTarget::Abstract` now carries a rendering EXCLUSION LIST
>    (opaque renderable-instance keys, `excludeFromRendering()`), consulted by the single
>    populate gate (`Scene::checkRenderableInstanceForRendering`). The caller registers its
>    subject after creation — first brick of the future probe system. ⚠️ Lifetime is the
>    caller's: exclusions are not cleaned when an instance dies. Validated: the city crosses
>    the offscreen-rendering bronze sphere's reflection continuously, no inception disc.
>    **AUTOMATIC since Aug 2026:** the same gate now also skips, without any registration, any
>    instance whose MATERIAL samples the render target being populated
>    (`Material::Interface::samplesTexture()`, cost gated on `renderType()` = Texture/Cubemap).
>    The manual list stays for subjects the material test cannot see (e.g. excluding a
>    neighbour). This closed a REAL GPU FAULT: `basic-scenery` never registered its bronze
>    sphere, the self-sampling draw made the Apple M2 GPU fault and recover, macOS discarded
>    every in-flight command buffer (`kIOGPUCommandBufferCallbackErrorInnocentVictim`) and the
>    device was lost — see `docs/caution-points.md` § "Probe self-sampling".
> 5. ✅ **FIXED — a skinned mesh in the probe corrupted the descriptor set binding.**
>    `VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358` + `VUID-vkCmdDrawIndexed-None-08600`
>    on the first frames of `reflexion-debug` options 3 and 4 (and ONLY those two — they are
>    the modes that render the scene into a cubemap **at scene build time**, before the first
>    logic tick). The pipeline layout seals the skinning set on the RENDERABLE
>    (`hasSkeletalData()`) while the binding tested the INSTANCE (`hasSkinningResources()`),
>    and those descriptor sets were created lazily from the logic thread. The probe rendered
>    the dragon in between: the skinning set was skipped and the running set counter shifted
>    the material and the bindless array one slot down. The resources are now created on the
>    render thread by `RenderableInstance::Abstract::prepareSkinningResources()`, an instance
>    missing them is not "ready", and every set is bound at the index the sealed layout
>    declares. Measured: the six reflection modes now run with ZERO validation errors.
>    See [`caution-points.md`](caution-points.md) § Vulkan Validation.
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

### 3.1 SSR — Hi-Z hierarchical trace (Aug 2026, UE-class)

| Pass | Target | What it does |
|------|--------|--------------|
| 0 Hi-Z build | R32F mip chain | compute: mip 0 = scene depth copy, mip N = MIN 2×2 of N-1 (conservative pyramid), ~log2(res) dispatches |
| 1 Trace | RGBA16F | **Hi-Z traversal** (Uludag, GPU Pro 5): the ray climbs mips over empty cells (exponential skips), descends on potential hits, converges at mip 0 with pixel precision. IGN-jittered start. No linear stride — the hit/miss banding of the former linear march cannot exist by construction |
| 2 Resolve | — | converts `hitUV` → reflected colour by sampling the scene colour; on a miss, falls back to the **prefiltered** cubemap (reserved cube slot 2) weighted by `envFallbackIntensity` |
| 3-4 Blur | — | separable **bilateral** (depth/normal-aware), radius scaled per-pixel by roughness — a polished surface stays mirror-sharp |
| 5 Composite | full-res | see section 4.3 |

Working resolution is FULL-RES by default (owner decision) — `Core/Graphics/ScreenSpace/
Reflection/PixelDoubling` (false), `BlurRadius`, `DepthSigma`, `NormalSigma` drive the quality.
The remaining `thickness` parameter only CLASSIFIES behind-vs-contact at the final hit — it no
longer drives the march. Confidence = `distFade · edgeFade · facingFade · roughnessFade`, with
`roughnessFade = 1 - smoothstep(0.55, 0.85, roughness)` and an early-out at `roughness > 0.85`.

**Cone-traced glossy (Aug 2026 — the second half of the Uludag chapter).** The resolve no
longer fetches the hit color sharp whatever the roughness: a rough surface reflects a CONE,
whose footprint at the hit is `2 · tan(GGX lobe) · marched screen distance` with
`tan(lobe) = roughness²`. A pre-convolved COLOR PYRAMID (half-res base, 4x4-tent chain
rebuilt every frame by compute — the exact Hi-Z build pattern, shared DS/pipeline layouts)
is read at the matching LOD; mirror-sharp rays (cone < 1 trace texel) keep the full-res
fetch — zero sharpness regression, half the memory of a full-res pyramid. The env fallback
already sampled the prefiltered environment at the roughness LOD, so the hit/miss boundary
is now blurred CONSISTENTLY on both sides — the D3 hard frontier softened as a side effect.
The known screen-space occlusion gap remains (rays aimed at geometry hidden behind the
reflector fall back to the environment): the per-pixel fallback chain (SSR miss → probe →
sky) is the successor.
References: Uludag, *Hi-Z Screen-Space Cone-Traced Reflections*, GPU Pro 5; McGuire & Mara,
*Efficient GPU Screen-Space Ray Tracing*, JCGT 2014.

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
  what lets rays pass through foliage and sprite texels. A material qualifies when
  `Material::Interface::isAlphaTest()` says so: `OpacityEnabled`, `BlendingEnabled`, or — since
  Aug 2026 — `AlphaTestEnabled`, the binary-cutout flag that deliberately stays in the OPAQUE
  raster list ([`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) § 5, "Alpha Test — the Binary
  Cutout Contract"). `alphaCutoff` is 0.5, the same fixed value as the raster and shadow discards.
- **Material lookup is per sub-geometry**: `getHitMaterialIndex(instanceIndex, geomIdx)` reads
  `GPUMeshMetaData[2][geomIdx]`, clamping to 0 when the BLAS has more sub-geometries than the
  renderable has material slots (procedural sprite quads).
- **Hit shading is Lambert only**: `albedo * (directLighting + sceneAmbient)`. Each light's
  contribution is gated by a shadow ray, but **only for lights that cast shadows in the raster
  passes** — a light without a shadow map deliberately shines through geometry on screen, and
  the reflection must match the image.
- **On a miss**, the ACTIVE SCENE's prefiltered environment is sampled (bindless reserved
  cube slot 2, roughness-driven LOD) × the sky luminance. The former dedicated `envCubemap`
  binding fell back to the renderer DEFAULT cubemap when the caller passed none — dark sky
  in every reflection (measured, fixed Aug 2026).
- **Hit shading is the ENRICHED UBER-SHADER** (Aug 2026): one parametric BRDF, data-driven
  from `GPURTMaterialData` — no program duplication. Per light (shadow-ray gated): Lambert
  diffuse + GGX/Smith/Schlick specular from the hit material's roughness/metalness (scalar or
  textured). Ambient at hit: scene scalar + sky IBL (irradiance slot 1 diffuse, prefiltered
  slot 2 specular tap) × sky luminance. Emission honored (color × strength × texture).
- **Normal mapping at the hit** (Aug 2026): when the material has a normal texture AND the
  mesh carries tangent space, the geometric normal is perturbed before all lighting.
  No `GPUMeshMetaData` extension was needed: the engine vertex layout is
  `Position(3)-Tangent(3)-Binormal(3)-Normal(3)` whenever TBN is present, so
  `normalOffsetFloats == 9` IS the TBN presence signal (the same layout contract the
  metadata offsets and the skinning mirror already rely on) — tangent at float 3, binormal
  at float 6, both barycentrically interpolated and taken through `objectToWorld`. The
  decode matches the raster exactly: `raw = rgb·2−1`, XY scaled by the material's
  **`normalScale`** — exported to the RT material SSBO in the former std430 padding slot
  (`matBase+6.w`, stride unchanged, RTGI untouched). Validated: specular glints on the
  pavement relief inside the reflection, absent before.
  **Still missing at the hit**: multi-bounce. The long-term path for full fidelity is a
  Saphir-generated SBT (`VK_KHR_ray_tracing_pipeline`) — the per-material codegen already
  exists for raster.

- **Glossy via the reflection pyramid (Aug 2026)**: the traced result used to composite
  SHARP whatever the roughness (the bilateral blur tops out at a few texels — nowhere near
  the ~100 px a 0.45-roughness cone spans). The trace output (PREMULTIPLIED color +
  confidence) now feeds a pre-convolved pyramid (half-res base, tent chain, compute); the
  composite reads it at `LOD = log2(coneWidthScale · roughness²)` — an O(1) blur of any
  width, the /confidence division renormalizing edge bleed. v1 assumes a representative hit
  distance (`coneWidthScale = 2 × hitFraction × trace height`, `hitFraction` 0.15 by default):
  the per-pixel hit-distance term belongs to the stochastic + temporal successor (Frostbite
  SSSR), which will also need an MRT for hit data. **Read § 3.2.1 before judging the sharpness
  of any glossy reflection** — the uniform cone has a documented failure mode on curved
  geometry. Debug note: any LINEAR debug visualization through the photometric exposure is
  unreadable (0.45 nits ≈ black at sunny-16) — use BINARY indicators scaled to 1e6.

#### 3.2.1 The uniform cone is wrong on curved surfaces — and `PixelDoubling` cannot fix it

> [!CAUTION]
> A polished sphere under RTR reads as **pixel doubling**, and turning
> `RayTracing/Reflection/PixelDoubling` off changes **nothing**. Both facts follow from the
> arithmetic below. Do not chase this in the trace pass: it lives entirely in the composite.

The cone width is expressed in **trace texels** and is proportional to the trace height, so the
LOD it selects cancels the resolution gain exactly. Put in closed form — the pyramid base is
half the trace, and the LOD offset is `−1`:

```
effective reflection width = pyramidBaseW / 2^coneLOD
                           = (traceW / 2) / (coneWidthTexels / 2)
                           = traceW / (2 · hitFraction · traceH · α)
                           = aspectRatio / (2 · hitFraction · α)      ← NO resolution term
```

At 16:9, `hitFraction` 0.15 and roughness 0.1 (α = roughness² = 0.01) that is **593 px, whatever
the output resolution**. Verified on the bench (`reflexion-debug --demo-options=0,5,0`: sphere,
AutoPostProcess, Polished), 1920×1080 logical window on a contentScale-1.5 display ⇒ 2880×1620
framebuffer:

| | `PixelDoubling` = true (default) | `PixelDoubling` = false |
|---|---|---|
| trace target | 1440×810 | 2880×1620 |
| `coneWidthScale` = 2·0.15·traceH | 243 | 486 |
| `coneWidthTexels` = scale·α | 2.43 | 4.86 |
| `coneLOD` (offset −1) | 0.28 | 1.28 |
| pyramid base | 720 px | 1440 px |
| **effective reflection width** | **593 px** | **593 px** |

Identical — hence a ×4.86 magnification over a 2880 px frame either way.

Two compounding causes:

1. **The former blend erased the sharp trace far too early.** It was
   `mix(rtrData, coneData, clamp(coneWidthTexels - 1, 0, 1))`, saturating at 2 texels, i.e.
   **roughness ≈ 0.058**. Above that, 100 % of the reflection came from a point lookup into a
   mip whose single texel spans the whole cone, and the full-resolution traced reflection was
   discarded outright. Fixed Aug 2026 by the **cross-fade** (`coneBlendStart` → `coneBlendFull`,
   2 → 24 trace texels by default): a near-mirror keeps most of its traced reflection, brushed
   metal still reaches the pyramid entirely.
2. **A screen-space cone ignores curvature — the deep cause, still open.** On a sphere the whole
   environment is compressed into the silhouette, so `d(reflected direction)/d(screen position)`
   is enormous and the screen footprint of a 0.57° GGX lobe is a fraction of a pixel, not 6.5.
   The `hitFraction = 0.15` heuristic is calibrated for flat-ish reflectors (floor, water),
   where a mirror reflecting the sky legitimately spreads `≈ α × height / (2·tan(fov_y/2))`
   ≈ 23 texels. **There is no single global value that is right for both** — which is exactly
   why lowering `hitFraction` to sharpen the sphere will under-blur the floor. The real fix is
   an empirically-derived footprint (screen-space derivative of the reflected direction) or the
   stochastic + temporal successor with per-pixel hit distance.

Bench knobs — read **once at `create()`**, so a change needs a relaunch (or any swap-chain
recreation), consistent with `PixelDoubling`:

| Setting key (under `Core/Graphics/RayTracing/Reflection/GlossyCone/`) | Default | Effect |
|---|---|---|
| `Enabled` | `true` | `false` zeroes `coneWidthScale`: the pyramid is never read, the composite shows the RAW traced reflection at full trace resolution — **the sharpness reference** |
| `HitFraction` | `0.15` | assumed hit distance as a fraction of screen height; `coneWidthScale = 2 × this × traceHeight` |
| `BlendStartTexels` | `2.0` | cone width under which the reflection is purely the sharp trace |
| `BlendFullTexels` | `24.0` | cone width from which it comes entirely from the pyramid |
| `MaxLod` | `8.0` | hard ceiling on the pyramid LOD, on top of the mip count — caps how coarse a rough surface may get |

**Measured gain of the cross-fade** (same framing, sphere at 4 m, reflected floor tiles inside
the silhouette — crop 550×390 px; sharpness = mean |gradient| and variance of the Laplacian):

| Variant | mean gradient | Laplacian variance | Tenengrad |
|---|---|---|---|
| former hard takeover (`BlendStart` 1, `BlendFull` 2) | 1.879 | 8.63 | 17.86 |
| **new default cross-fade (2 → 24)** | **4.141** | **62.26** | **77.29** |
| `Enabled = false` (raw traced reflection, ceiling) | 4.548 | 81.87 | 94.83 |

**×2.20 gradient energy / ×7.21 Laplacian variance** over the former behaviour, landing at
**91 % of the sharp-trace ceiling** in gradient energy. The residual gap is the cone's remaining
13 % pyramid contribution at roughness 0.1 — deliberate, so the knob stays physically meaningful
instead of being neutralized.

> [!NOTE]
> **Engine cubemap convention.** A world direction `D` samples the cubemap at
> `vec3(D.x, -D.y, D.z)` — engine UP is -Y, cubemaps are stored Y-up. This holds in the skybox,
> in the material reflection code and in both post-process miss branches. Get it wrong and the
> reflection is vertically mirrored.

### 3.3 Skinned geometry in the TLAS — per-frame BLAS refit (Aug 2026)

A skinned mesh's VBO holds the **bind pose**; the visible shape only exists through the bone
matrices applied in the vertex shader. A geometry-level static BLAS therefore puts a frozen
mannequin in the TLAS — and at the WRONG scale: the TLAS instance transform is
`nodeWorld × instanceMatrix`, but for a skinned mesh the real size comes out of the joint
matrices, not out of `bindPose × nodeWorld`. Measured on ReflexionDebug's glTF dragon: a
36 cm statuette (raw bind span 119 units × 0.12 demo scale × 0.0254 inch-to-metre export
node) versus the metres-tall raster dragon — i.e. **totally invisible** in any reflection.

The engine therefore runs the industry-standard refit path (UE/Unity equivalent):

| Piece | Where | Role |
|-------|-------|------|
| Guard | `Geometry::Interface::buildAccelerationStructure()` | refuses to build a static BLAS for `influenceEnabled()` geometry |
| Mirror + BLAS | `RenderableInstance::Abstract::createRTSkinnedGeometryResources()` | **per INSTANCE** (two actors sharing a mesh have different poses): a full-layout mirror vertex buffer, an `ALLOW_UPDATE` BLAS (initial build from the SOURCE VBO — bind pose is valid data, the mirror is garbage until the first dispatch), and the update scratch |
| Compute skinning | `Graphics::SkinnedGeometryProcessor` (renderer-owned, beside the AS builder) | one dispatch per skinned instance per frame: copies the whole vertex, then overwrites position and normal/TBN with the CURRENT-pose skinning (same math as `Saphir::VertexShader`, interleaved `{current, previous}` bones, even slots) |
| Refit | `AccelerationStructureBuilder::recordBLASRefit()` | `vkCmdBuildAccelerationStructuresKHR` mode UPDATE, src = dst, flags matching the original build |
| Orchestration | `SceneMetaData::recordTLASBuild()` | barriers A (previous ray-query reads → compute writes), dispatches, B (mirror writes → AS build reads), refits, C (BLAS writes → TLAS build reads), then the TLAS build |

Load-bearing details:

- **The mirror clones the source layout** (same stride, same attribute offsets), so
  `GPUMeshMetaData` needs no special case — `vertexBufferAddress` simply points at the mirror
  and hit shading reads current-pose normals/UVs. The influence/weight vec4s are always the
  LAST 8 floats of the layout (`influenceOffset = floatsPerVertex - 8`).
- **The bone descriptor sets are SHARED with the raster**: the skinning DS layout declares
  `VERTEX | COMPUTE`, and `flushSkinningMatrices()` (upload deduplicated on the frame cursor)
  is called from the RT recording first — RT and raster skin with the exact same pose section.
- **Pose sync**: the reflection shows the same animation frame as the raster dragon standing
  next to the sphere. If they ever diverge, suspect the flush/cursor contract, not the BLAS.
- ⚠️ **Two instances of the same skeletal mesh build TWO BLAS from the SAME source vertex
  buffer.** That is by design (one pose each) — and it is what exposed the queue family
  ownership defect (Aug 2026): the upload released the buffer ownership once, but every BLAS
  build acquired it, so the second instance's build was rejected outright
  (`VUID-vkQueueSubmit-pSubmits-02207`) and its skinned BLAS never existed. The release and
  the acquire now both live in `BufferTransferOperation`. See
  [`caution-points.md`](caution-points.md) § Vulkan Validation. Symptom to recognize: ONE
  actor of a duplicated skeletal mesh is a bind-pose statue in the reflections, the other is
  fine.
- Refit-ineligible skinned instances (no bone SSBO yet, TriangleStrip) are kept OUT of the
  TLAS rather than shown as statues.
- Barriers use `FRAGMENT | COMPUTE` for ray-query reads — **never** the RT-pipeline stage bit
  (the engine is `VK_KHR_ray_query` only).
- `SceneMetaData` logs the TLAS content (name, position, scale, alpha-test) whenever the
  instance count changes — first thing to read when something is missing from a reflection.

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