# Shadow Mapping System

This document details the shadow mapping architecture in Emeraude Engine, including PCF soft shadows, global controls, and per-light configuration.

## Overview

The engine supports shadow mapping for all three light types:
- **Directional lights**: 2D shadow maps (standard or Cascaded Shadow Maps)
- **Spot lights**: 2D shadow maps
- **Point lights**: Cubic shadow maps (6-face cubemap)

## Global Shadow Mapping Control

Shadow mapping can be globally enabled/disabled via the settings system.

**Setting key:** `GraphicsShadowMappingEnabledKey` (`Core/Graphics/ShadowMapping/Enabled`)

| Value | Behavior |
|-------|----------|
| `true` (default) | Shadow maps rendered, shadow-enabled passes used |
| `false` | Shadow maps skipped, base pass types used (no shadow sampling) |

### Implementation Details

When the global setting is disabled:

1. **Shadow map rendering skipped**: `Scene::renderShadowMaps()` checks `Renderer::isShadowMapsEnabled()` and returns early
2. **Base pass types forced**: `Scene::renderLightedSelection()` selects base or `*ColorMap` pass types (no shadow sampling)
3. **Shadow samplers unused**: Descriptor sets still contain dummy shadow textures at binding 1, but the shader never samples them

**Code references:**
- `Scenes/Scene.rendering.cpp:978` - Global setting check in `renderLightedSelection()`
- `Scenes/Scene.rendering.cpp:1003` - Directional light pass selection
- `Scenes/Scene.rendering.cpp:1034` - Point light pass selection
- `Scenes/Scene.rendering.cpp:1064` - Spotlight pass selection
- `Graphics/Renderer.cpp:isShadowMapsEnabled()` - Setting accessor

### Why Global Control Matters

Without the global check, disabling shadow mapping via settings caused Vulkan validation errors:

```
VK_IMAGE_LAYOUT_UNDEFINED (0) but expected VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
```

**Root cause:** Shadow maps are created but never rendered (layout stays UNDEFINED). However, lighting passes still attempted to bind shadow-enabled descriptor sets containing these images.

**Solution:** The rendering code now checks the global setting and selects base pass types (e.g., `SpotLightPass` or `SpotLightPassColorMap`) when shadows are disabled, which generate shaders without shadow sampling code.

## PCF Soft Shadows

Percentage-Closer Filtering (PCF) provides soft shadow edges by sampling multiple points in the shadow map.

### PCF Methods

The engine supports four PCF methods, configured via `GraphicsShadowMappingPCFMethodKey`:

| Setting Value | Internal Method | Description | Sample Pattern |
|---------------|-----------------|-------------|----------------|
| `"Performance"` | Grid | Max FPS, basic quality | Square pattern, (2n+1)² samples |
| `"Balanced"` | VogelDisk | Recommended sweet spot | Circular, evenly distributed |
| `"Quality"` | PoissonDisk | Better visuals | Circular, random-looking distribution |
| `"Ultra"` | OptimizedGather | Best quality, optimized | Uses `textureGather` for efficiency |

**Default:** `"Balanced"` (VogelDisk - best quality/performance balance)

### PCF Radius Per Light Type

Each light type has different default PCF radius values:

| Light Type | Default Radius | Reason |
|------------|----------------|--------|
| Directional | 1.0 | Large coverage, subtle softness |
| Spot | 4.0 | Medium coverage, visible softness |
| Point | Auto (*100) | Depth-based contact hardening |

**Point light auto-calculation:**
```cpp
m_PCFRadius = (1.0F / static_cast<float>(resolution)) * 100.0F;
```

This creates a "contact hardening" effect where shadows are sharper near contact points and softer further away.

### Per-Vertex Lighting (Low Quality) Shadow Constraint

> [!WARNING]
> **GLSL shader inputs are READ-ONLY!**

This trap belonged to the per-vertex (Gouraud) lighting mode, DELETED in Aug 2026 along with the
whole Blinn-Phong machinery: `diffuseFactor` and `specularFactor` were computed in the vertex
shader and arrived as read-only interface-block members. The rule is kept because it applies to
ANY value a generator receives as a shader input — the fragment shader must copy before it
modifies:

```glsl
// Create local copies of read-only shader inputs
float diffuseFactor = svLight.diffuseFactor;
float specularFactor = svLight.specularFactor;

// Now safe to apply shadow factor
diffuseFactor *= shadowFactor;
specularFactor *= shadowFactor;
```

**Historical code reference** (file deleted): `Saphir/LightGenerator.PerVertex.cpp:generateGouraudFragmentShader()`

### PCF Code Generation

Shadow map filtering code is generated in `LightGenerator.ShadowMap.cpp`:

| Function | Purpose |
|----------|---------|
| `generate2DShadowMapCode()` | Non-PCF 2D shadow sampling |
| `generate2DShadowMapPCFCode()` | PCF-enabled 2D shadows |
| `generate3DShadowMapCode()` | Non-PCF cubemap shadow sampling |
| `generate3DShadowMapPCFCode()` | PCF-enabled cubemap shadows |
| `generateCSMShadowMapCode()` | Cascaded Shadow Maps |

**Code references:**
- `Saphir/LightGenerator.ShadowMap.cpp` - All shadow map code generation
- `Saphir/LightGenerator.hpp:PCFMethod` - PCF method enum

### Outside the map is LIT, and that is enforced in the GENERATED CODE

A directional light is a light at infinity: its 2D map covers a finite box, and a fragment outside
that box is not "in shadow", it is **unknown** — the only defensible answer for a sun is **lit**.
`insideShadowVolumeCondition()` guards the lookup on **all three axes** (`x`, `y` and `z` against
`w`, projectively), and `shadowFactor` keeps its `1.0` initial value when the test fails.

> [!CAUTION]
> Only **Z** was guarded until Aug 2026. Lateral overflow fell through to the sampler's address
> mode — and through the `"ShadowMap"` cache-key collision (see `src/Graphics/AGENTS.md`), that mode
> was `CLAMP_TO_EDGE`, not the `CLAMP_TO_BORDER` the shadow map explicitly requested. The edge texel
> ring therefore shadowed the entire exterior: on `reflexion-debug` (coverage 60 ⇒ a 120×120 m box,
> on a 200×200 m floor) the floor went black past the coverage limit. **Both** halves are needed and
> neither is redundant: the border mode absorbs the PCF taps that stray across the edge (a hard
> guard alone would stair-step at the limit), while the guard makes the semantic independent of
> whatever addressing the sampler cache hands out.

The same code path serves **spot** lights and is correct for them too: outside its cone a spot
already has a zero cone factor, so answering "lit" outside the map adds no light anywhere.

Structural limit, not a bug: past the coverage box there is simply **no shadow information**, so
distant casters cast nothing. The classic map's box is anchored on the **world origin** and never
follows the camera — enlarging `coverageSize` trades texel density for reach. That is what CSM
exists to solve.

## Alpha-Tested Shadows — the shadow pass must honour the opacity mask

The shadow pass renders depth only, so it is tempting to skip texture sampling entirely.
Doing that makes alpha-cut geometry cast the shadow of its **QUAD**: the palm trees of
`basic-scenery` used to drop hard rectangular blocks on the ground instead of leaf shadows.

`Generator::ShadowCasting` therefore generates a discard in the shadow fragment shader when
`needsAlphaTest` is set, delegating to `Material::Interface::generateShadowAlphaTestCode()` so
each material type samples its own opacity source. Any new material type that supports an
opacity mask MUST implement it, otherwise its cut-outs silently cast solid shadows again.

`StandardResource::requiresAlphaTestedShadows()` requires an alpha source texture, then returns
`true` for `MaterialFlagBits::AlphaTestEnabled` **as well as** for `BlendingMode::Normal`.

> [!CAUTION]
> Until Aug 2026 this text described `BasicResource` — the unlit material, **removed** by the
> material merge (`a1619516`) — and it never matched `StandardResource`, which has always gated on
> the flag ALONE. That gap was not cosmetic: a material manifest declaring an `"Opacity"` texture
> routes through `setOpacityComponent()`, which arms `BlendingMode::Normal` and NOT
> `AlphaTestEnabled`, **and there is no JSON key to request a cutout** (`getBlendingModeFromJSON()`
> accepts Normal/Add/Multiply/Screen/None only). Every JSON-authored foliage therefore cast the
> shadow of its QUAD. Measured on `reflexion-debug`: the palm dropped a solid blob, and now casts
> its fronds. The threshold needed no change — the material UBO already ships
> `DefaultAlphaThreshold` = 0.5 whether or not alpha testing was ever enabled.

Genuinely **translucent** surfaces are the deliberate trade-off: a depth-only map stores no partial
occlusion, so a window at a uniform alpha 0.3 now casts NOTHING instead of a solid block. Both are
wrong; nothing is far less conspicuous than an opaque shadow under glass. Coloured or stochastic
shadow maps are the real answer, not a different cutoff here.

Expect **hard, ragged shadow edges**: the cutout is binary, so the texture's antialiased border
texels (8.7 % of the palm's mask) collapse to all-or-nothing at the map's texel scale. PCF softens
them and is off by default.

The shadow discard uses the **same fixed 0.5 cutoff** as the colour pass, so the two agree by
construction (the third path, `GPURTMaterialData::alphaCutoff`, agrees too). The cutoff is not
configurable on purpose — the reasons live with the flag's contract in
[`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) § 5, "Alpha Test — the Binary Cutout Contract".

**Code reference:** `Saphir/Generator/ShadowCasting.cpp` (fragment shader generation),
`Graphics/Material/StandardResource.cpp:requiresAlphaTestedShadows()`.

## Cascaded Shadow Maps (CSM) — STATUS: WORKING, NOT YET POLISHED (Aug 2026)

> [!IMPORTANT]
> **A CSM light lights and casts.** Verified on `reflexion-debug` with the exposure pinned: the palm
> casts, and the four witness cubes at (±75, ±75) — 106 m from the centre, far outside any classic
> map's box — cast too. Four independent defects had to fall for that, and NONE of them was the
> cause recorded for months as defect #4:
>
> | | Defect | Fix |
> |---|---|---|
> | 4a | `updateCascades()` filled `m_CSMBuffer` every frame but **never called `requestVideoMemoryUpdate()`**, and `AbstractLightEmitter::updateVideoMemory()` uploads nothing without that flag | The only upload ever performed was the one from `createOnHardware()`, when the buffer is **all zeros** — black colour, zero intensity, null matrices. Hence "no light at all", permanently, for a static light. Ground mean 16.2 vs 73.7 classic; 69.8 after | **FIXED** `c7eef938` |
> | 4b | `computeCascadeProjection()` passed `minZ - margin` / `maxZ + margin` as the ortho near/far, but those are **view-space z coordinates** while `orthographicProjection()` expects positive **distances** | Cascade depth outside [0,1]: casters clipped out of the map, and the `projCoords.z` guard failing at sampling time, leaving `shadowFactor = 1.0`. No shadow, ever — independently of 4a | **FIXED** `8c6417f0` — `near = -maxZ - margin`, `far = -minZ + margin`. ⚠️ `frustumCenter` is still computed and never used: the light camera sits at the world origin, contrary to its own comment |
| 4c | The shadow-pass `distance > viewDistance` culling measured from the light ENTITY while the cascades are fitted to the MAIN CAMERA | On `reflexion-debug` the sun sits 999 m away against a 500 m viewDistance, so EVERY caster was rejected and the map rendered empty. The frustum half of the same condition was already skipped for CSM; the distance half was not | **FIXED** `882e5f29` — ⚠️ TODO(perf): CSM now walks every caster; the right filter is the union of the per-cascade frustums |
| 4d | `cascadeScale` (the light's `csmScale`) was unreachable from `BackgroundLightingOptions`, pinning coverage to the whole camera frustum | Shadows present in the map, **sub-texel on screen** — reads as "still broken" once 4a-4c are fixed. Cost this investigation two wrong conclusions | **FIXED** `864e6582` — covered depth = viewDistance / cascadeScale; pick it from the scene |
> | 4c | The shadow-pass `distance > viewDistance` culling measured from the light ENTITY (999 m away) against a camera-derived 500 m, so EVERY caster was rejected and the map rendered empty. The frustum half was already skipped for CSM; the distance half was not | `882e5f29` |
> | 4d | `cascadeScale` was unreachable from the sky derivation, pinning the coverage to the whole camera frustum — shadows present in the map, sub-texel on screen | `864e6582` |
>
> ⚠️ The original defect #4 blamed an early return on a null shadow map. That was never it (with
> `shadowMapResolution > 0` the map exists and the function completes). **Do not act on the old
> text**; it is preserved below only as a record of the wrong lead.
>
> ⚠️ **`cascadeScale` is not optional in practice.** Covered depth = camera view distance /
> cascadeScale, split across at most 4 cascades. Leaving it at 1 makes a working CSM look broken.
> Pick it from the scene: `reflexion-debug` uses 4 (500 m → 125 m of coverage for action inside
> 106 m).
>
> ✅ **Texel snapping and a rotation-invariant fit landed Aug 2026** — see § *Cascade fit stability*
> below. Still missing, and what the rest of the polish pass owes: inter-cascade blending, a
> per-cascade depth bias, and a single source of truth for the light-space transform. ⚠️ That last one is computed at **FOUR** sites, not three as this line
> claimed until Aug 2026: `DirectionalLight::updateLightSpaceMatrix()`, `DirectionalLight::move()`,
> `ViewMatrices2DUBO::updateOrthographicViewProperties()` **and `DirectionalLight::createOnHardware()`**,
> which open-codes the same frame recipe a second time with a *different* coverage fallback
> (`m_coverageSize > 0.0F ? m_coverageSize : getDistanceOrFar() * 0.5F`). Latent on top of that:
> `updateLightSpaceMatrix()` hard-codes `-m_coverageSize, m_coverageSize` while
> `ViewMatrices2DUBO` computes `halfSide = (far * 0.5F) * getAspectRatio()` — the two agree **iff the
> map is square**, which every map is today. A non-square shadow map would silently misregister.
>
> `Toolkit::generateDirectionalLight` selects CSM purely by ARGUMENT COUNT — `(name, colour, lux,
> 2048, 4, 0.5F)` is CSM, `(name, colour, lux, 2048, 140.0F)` is the classic map. The two are
> easy to confuse and the failure is silent.

### What was broken, and what each defect cost

| # | Defect | Symptom | Status |
|---|---|---|---|
| 1 | `Scene::prepareRenderPassTypes()` never emitted `DirectionalLightPassCSM`, while `renderLightedSelection()` selects it as soon as `light->usesCSM()` | Program lookup missed at draw time → the whole directional pass was skipped. No diffuse, no specular, NO shadow. Only the ambient pass survived, so the scene looked flatly ambient-lit | **FIXED** |
| 2 | `generateCSMShadowMapCode()` emitted `ubView.viewMatrix`, which the 2D view block does not declare (the view matrix travels as a PUSH CONSTANT, vertex stage only) | The CSM fragment shader could not compile. `setBroken()` on the instance then removed the renderable from rendering entirely — a disappearing ground and a frozen animated material | **FIXED** — reads `svPositionViewSpace.z`, requested `ToNextStage` in all four light generators |
| 3 | `LightSet::initialize()` sized the shared directional UBO on the CLASSIC block (~120 B) while a CSM light always uploads the CSM block (324 B) | Cascade splits, cascade count, shadow bias and the light's own colour / direction / intensity were truncated away and never reached the GPU | **FIXED** — sized on `max(classic, CSM)` |
| 4 | ~~The CSM block's colour / direction / intensity are written **only** inside `DirectionalLight::updateCascades()`, which returns early when `m_shadowMap == nullptr`~~ **WRONG CAUSE — re-attributed Aug 2026 to 4a-4d** | Measured live after fixes 1-3: the ground still receives no sun from a CSM light. Switching the same scene to the classic constructor lights it correctly — an A/B on `water-world` with everything else unchanged | **CLOSED** — the symptom was real, the cause was not. Kept as a record of the wrong lead |
| 4a | `updateCascades()` fills `m_CSMBuffer` every frame but **never calls `requestVideoMemoryUpdate()`**, and `AbstractLightEmitter::updateVideoMemory()` uploads nothing without that flag. The early return is NOT the cause: when `shadowMapResolution > 0` the map exists and the function runs to completion | The only upload ever performed is the one from `createOnHardware()`, at a point where `updateCascades()` has not run yet and `m_CSMBuffer` is **all zeros** — black colour, zero intensity, null matrices. Hence "no light at all", permanently, for a STATIC light | **OPEN** — falsifiable prediction that distinguishes this cause from any other: a CSM light in **continuous motion** should light correctly, because `move()` raises the flag every frame and publishes the buffer filled on the previous frame |
| 4b | `ViewMatricesCascadedUBO::computeCascadeProjection()` passes `minZ - margin` / `maxZ + margin` as the ortho near/far, but those are **view-space z coordinates** (negative in front of the camera) while `orthographicProjection()` expects positive **distances** | Cascade depth lands outside [0,1]: casters are clipped out of the map at render time, and at sampling time the `projCoords.z >= 0 && <= 1` guard fails, leaving `shadowFactor = 1.0`. No shadow, ever — independently of 4a | **OPEN** — correct bounds are `near = -maxZ - margin`, `far = -minZ + margin` (sign **and** order inverted). Note `frustumCenter` is computed in the same function and never used: the light camera stays at the world origin, contrary to its own comment |

### The CSM ⊕ colour-projection contract is now enforced in code

CSM and colour projection are mutually exclusive: the light-space position is resolved per cascade
in the fragment shader and cannot address a single projection texture, and `getUniformBlockCSM()`
declares no `colorProjectionBoost` / `ColorProjectionIndex` member. `DirectionalLightPassFullCSM`
therefore names a shader that **cannot compile**.

- `prepareRenderPassTypes()` deliberately does NOT pre-generate `DirectionalLightPassFullCSM`.
- `renderLightedSelection()` falls back to `DirectionalLightPassCSM` when a CSM light also carries a
  colour projection texture: the shadow is the load-bearing half, the projection is dropped.

Previously, asking for both produced a compile failure that broke the entire renderable instance.

### Resolution budget — CSM coverage comes from the CAMERA, not the scene

Cascade splits are derived from the main camera's near/far (`ViewMatricesCascadedUBO`, practical
split scheme), never from the scene bounding box. With the engine default `Core/Graphics/ViewDistance`
of 10000 m, 4 cascades and `lambda = 0.5`, cascade 0 alone spans **~1250 m**, so its light-space box
covers kilometres and 2048 px resolves about **1.2 m per texel** — every shadow detail below a metre
is sub-texel and simply cannot appear, even with a fully working CSM.

Two knobs: `csmScale` on the light (16-32 concentrates the cascades near the camera, shadows stop
beyond ~1/scale of the far plane), and `Camera::setDistance()` — which reaches the render target
through `updateAllVideoDeviceProperties()` and is what `Scene::updateCSMCascades()` reads. For a
small world, the classic constructor is simply the better tool: its coverage is an explicit
half-extent in world units, so the texel density is fixed and independent of where the camera looks.

### Per-cascade depth bias (Aug 2026) — the knob that did nothing

⚠️⚠️ `shadowBias` was uploaded by `DirectionalLight` and `SpotLight`, declared in their uniform
blocks, and **read by nobody on either 2D path** — its only generated consumers were the point-light
cubemap ones. Setting it changed nothing at all, silently, for both the classic and the cascaded map.

It is now wired on the CSM path and means **world units — metres**. The rasterizer bias on the cast
pass cannot do this job: it is per-PIPELINE, and all cascades go through that one pipeline via
multiview, so it structurally cannot vary per cascade — while cascades differ in texel size by more
than an order of magnitude.

The per-cascade scale is **derived in the shader** rather than uploaded:

```glsl
const float cascadeInverseRadius = length(vec3(cascadeMatrix[0][0], cascadeMatrix[1][0], cascadeMatrix[2][0]));
projCoords.z -= ubLight.shadowBias * cascadeInverseRadius / 3.0;
```

After the bounding-sphere fit the orthographic X scale is exactly `1/radius` and the light view is a
rotation plus a translation, so `length(row0)` recovers `1/radius`; the depth range is `3 × radius`
by construction. That keeps the uniform block untouched — its layout is described by hand in **three**
places and a silent truncation has already shipped from editing one of them.

⚠️ **Two dead bias slots existed; one is now live.** The light-side `CSM_ShadowBiasOffset` is wired.
The view-side `ViewMatricesCascadedUBO::ShadowBiasOffset` is still dead — written by nobody, exposed
as `cascadeProperties.y`, read by nothing — and deliberately NOT deleted: removing a member shifts
every offset after it in a layout maintained by hand in three files. It is inert and waits for a
change that does the whole layout at once.

@todo Normal-offset shadows would beat a pure depth bias on grazing surfaces, but they need the
world-space normal, which no fragment shader here interpolates: the light pass synthesizes only the
VIEW-space normal, and later than the shadow block. Its own change.

### The old note on the absence of a bias

The CSM sampling path compares `projCoords.z` raw. This matches the 2D **PCF** path (which also has
no explicit bias and relies on the hardware comparison sampler); only the non-PCF 2D path and the
cubemap path apply `max(ShadowBias, 0.005)`. The shadow-map render pass itself uses
`ProgramType::ShadowCasting` with `RenderPassType::SimplePass`, which leaves `depthBiasEnable` off
unless the material's options ask for it.

## Temporal coherence — where the two-state contract stops (Aug 2026)

> [!CRITICAL]
> **The two-state system freezes VALUES on the CPU. It does not freeze the GPU memory those values
> are uploaded into, and the shadow path is the only place in the engine where that difference is
> visible.** Investigated after a report of "shadow maps flicker, and only when the camera moves".

### The asymmetry that explains "why only the shadows"

The main pass carries its matrices in **push constants**, read from `m_renderState[readStateIndex]`
and baked into the command buffer at record time (`RenderableInstance/Unique.cpp`). Once recorded,
the GPU cannot see them change. The 2D view UBO deliberately holds no frame-varying value and says
so in `ViewMatrices2DUBO.cpp`:

```
/* [CAUTION] NEVER write a frame-varying value in here. This uniform buffer object is
 * SINGLE-buffered (one UBO, not framesInFlight() copies), so a value that changes every
 * frame races the GPU: the raster of frame N can read what frame N±1 wrote. ... */
```

CSM **cannot** use push constants: multiview indexes the cascades by `gl_ViewIndex` from a UBO
(`RenderableInstance/Abstract.cpp`, the `PerView` set is bound for a CSM target *because* of it). So
the cascade matrices are read **at GPU execution time**, out of `ViewMatricesCascadedUBO`, whose
first 256 bytes are `mat4[4] cascadeViewProjectionMatrices` — refit to the camera frustum on every
logic tick. It is the same single-buffer upload path as the 2D UBO, minus the warning, carrying
exactly what the warning forbids. Point-light cubemap shadows share the shape (6 views from a UBO).

### The four defects, ranked (OPEN unless marked)

| # | Defect | Status |
|---|---|---|
| A2 | The light UBO (`SharedUniformBuffer`) was single-instance, host-written every frame while up to `framesInFlight()-1` submitted frames were still reading it; and the scene upload ran **before** the frame's in-flight fence. | **FIXED** — one UBO region per frame-in-flight, folded into the existing dynamic offset; the upload moved behind the fence. The cascaded VIEW UBO carries one buffer and one descriptor set per frame-in-flight (it is not a dynamic-offset buffer, so the frame dimension cannot be folded into an offset). The 2D and 3D views keep ONE region and declare the frame index explicitly ignored: they carry no frame-varying value, which their own `[CAUTION]` block enforces. |
| A1 | **FIXED.** Not CSM-only: EVERY light's sampling matrix was outside the contract. The CSM matrix existed **twice, on two different regimes**: the rasterising copy is published (`m_renderState[readStateIndex]`), the sampling copy (`DirectionalLight::m_CSMBuffer`) is not. `cascadeViewProjectionMatrix()` has **no `readStateIndex` overload** and serves `m_logicState`, unlike every other accessor in that class. **No light emitter has ever overridden `publishStateForRendering()`** — the contract covers entities and render targets only. | **OPEN** |
| A3 | `m_renderStateIndex` was loaded **independently four times** inside one rendered frame. A frame straddling a publish rasterised the shadow map with tick N and the colour buffer with tick N+1. | **FIXED** — `Scene::beginRenderFrame()` latches once, every consumer reads the latch |
| A4 | `m_CSMBuffer` was written by the logic thread and memcpy'd by the render thread with no synchronisation. | **FIXED** by A1 — the render thread now reads a published slot, and the publish/`m_renderStateIndex` release-acquire pair is the edge |
| A5 | `Scene::updateCSMCascades()` tested a mutex-guarded container, released the lock, then walked `LightSet::directionalLights()` **unguarded**. | **FIXED** — `LightSet::forEachDirectionalLight()` |

### ✅ FIXED (Aug 2026) — and what the measurement taught

> **Acceptance test, passed:** `global-illumination`, flashlight on (**F**), maximum movement speed.
> The lit pool on a wall **stays round**. Validation layers on, zero VUID.

⚠️⚠️⚠️ **The CPU half alone did NOT fix it, and that is the load-bearing lesson.** With the light's
block correctly published — the two-state contract finally covering lights — the halo still cut. What
closed it was the GPU half: the light UBO was a single instance, host-written every frame while
frames in flight were still reading it. **Publishing a value coherently is worthless if the memory it
is uploaded into is overwritten under the GPU's feet.** Anyone reasoning about a temporal artefact in
this engine has to check both sides of the contract, not just the CPU one.

⚠️⚠️ **A plausible frame is not a correct frame.** While converting the light UBO, a region loop
bounded by a compile-time maximum instead of the buffer's real region count wrote past the end and
left every bind carrying a dynamic offset outside the allocation. The image looked right the whole
time; only `VUID-vkCmdBindDescriptorSets-pDescriptorSets-01979` said otherwise. Read the FIRST VUID
of a burst — the "set not bound" errors that followed were consequences — and never accept "it looks
stable" as the verification for a synchronisation change.

⚠️ **`screenshot()` cannot capture this class of artefact.** The capture lands after the console
command is processed, with the camera already parked for at least a frame and everything reconverged.
A 20-pose sweep at 1 cm steps and a series of teleport-and-capture runs all came back clean while the
defect was plainly visible to the eye. For a temporal artefact the instruments are
`Core.toggleRecording()` or a human watching the screen — not a still.

### ⚠️⚠️ CONFIRMED ON SCREEN: any light that MOVES desyncs, classic map included

> **The acceptance test for this whole chapter:** in `global-illumination`, with the player's
> flashlight on (key **F**), move violently. **The lit pool on a wall must stay ROUND.** When it is
> cut by a straight chord — a crescent instead of a disc — the sampling matrix and the map disagree.
> That straight edge IS the boundary of the stale light frustum. Reproduced and captured Aug 2026.

The player's flashlight is a `Component::SpotLight` parented to the **head node**
(`projet-alpha/src/Actor/Player.cpp`, a 512² map), so it moves *exactly as fast as the camera*,
every logic tick. That makes it the fastest reproducer in the project — much faster than any sun.

**Negative control, run Aug 2026 — the colour projection is EXONERATED.** `updateLightSpaceMatrix()`
is called when `isShadowCastingEnabled() || hasColorProjectionTexture()`, and the flashlight has
**both**, so the same stale matrix feeds two consumers: the shadow lookup and the projected cookie.
Either could cut a beam with a straight edge. With `Core/Graphics/ShadowMapping/Enabled = false` and
nothing else changed, the owner moved as fast as he could and **the halo stayed perfectly round** —
while the cookie projection was still running on the same stale matrix. The cut is therefore the
**shadow depth comparison**, not the projection. (⚠️ The control is not perfectly symmetric — the
scene is brighter without shadows — but "round at maximum speed" is not a contrast effect.)

The mechanism, end to end:

| | Path | Thread | State |
|---|---|---|---|
| Rasterises the map | shadow render target's view matrices | render | **published**, `readStateIndex` |
| Samples the map | `SpotLight::m_buffer[LightMatrixOffset]`, written by `updateLightSpaceMatrix()` from `move()` | written on **logic**, uploaded on **render** by `LightSet::updateVideoMemory()` | **live logic state, no index, no publish** |

`onVideoMemoryUpdate()` does `UBO.writeElementData(...)` into a **single-instance** `SharedUniformBuffer`,
before the frame's in-flight fence. So the two matrices come from **different logic ticks** the moment
the light moves. Where the two frusta still overlap the depth comparison passes; past the divergence
it fails and the region reads as shadowed. Hence a straight-edged bite out of a round beam, moving
frame to frame — what reads to the eye as *"zone-wise z-fighting between a light and a dark version"*.

⚠️ **The invariant is about the LIGHT, not the map type.** An earlier revision of this section said a
classic map's image "cannot change when only the camera moves" and invited the conclusion that
classic maps were exempt. That is true **only for a STATIC light** — and a head-mounted torch is the
opposite of static. Restated correctly:

- **Static light, static geometry, camera moves:** the map's *content* genuinely cannot change (light
  frame anchored and rewritten only on light movement; the pass re-cleared and re-rendered
  unconditionally every frame; ambient never reaches a shadow map — `Scene::refreshAmbientLightProperties`
  walks views and textures only; every field of both UBO blocks constant). Look at the sampling side.
- **Light that moves — a torch, a lamp on a vehicle, an animated sun, ANY CSM light (refit to the
  camera every tick by construction):** the sampling matrix is outside the publish contract, so it
  desyncs. This is A1 + A2, and it is the confirmed cause.

**When the light is static and the artefact is still there, look at:**

1. **The main-pass cull reading the LIVE logic state** — `populateRenderLists()` used the
   no-argument `position()` / `frustum(0)` overloads, which serve `m_logicState`, while the logic
   thread rewrote it every tick. A torn frustum culled entities in and out for single frames,
   **only while the camera moved**. **FIXED** — both render lists now read `readStateIndex`.
2. **The PCF kernel rotation is a screen-space hash** — `LightGenerator.ShadowMap.cpp`,
   `fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453)`, in the 2D VogelDisk and
   PoissonDisk paths and in the three cubemap paths. **No frame index or time is mixed in**, so the
   field is *fixed in screen space*: surfaces slide **through** a stationary noise pattern, giving
   **crawl/swim, not flicker** — and **TAA is structurally unable to attenuate it**, because
   averaging a value that is constant in time returns that constant. `generateCSMShadowMapCode()`
   does **not** use it. Engine default is PCF *off*; a machine that enables it opts into this.
3. **The stochastic screen-space stack.** TAA, RT ContactShadows, RTAO, RTGI (animated noise), RT
   Reflection, motion blur and DoF are all temporally-accumulated estimators that converge on a
   parked camera and boil under motion. **ContactShadows is itself a shadow term** and is
   indistinguishable by eye from shadow-map flicker. Eliminate this confound **first**, one effect
   at a time, before blaming anything above.

### Amplitude laws — how to tell the classes apart on screen

- **Class B (unsnapped cascade fit) — FIXED Aug 2026:** amplitude was one shadow texel,
  ∝ 1/resolution, **independent of camera speed**, deterministic in camera pose. Kept here because
  the amplitude law is what tells this class apart from a state desync, and because the same law is
  how you would recognise a regression.
- **Class A (state desync):** amplitude = camera displacement per logic tick, ∝ camera speed,
  **independent of resolution**, non-deterministic frame to frame.
- **The PCF hash:** fixed in screen space; freezes the instant the camera parks, crawls the instant
  it moves, and survives TAA.

### Cascade fit stability — sphere fit and texel snapping (Aug 2026)

`computeCascadeProjection()` used a light-space **AABB** of the frustum slice and snapped nothing.
Two causes, one signature — shadow edges crawling continuously under camera motion, frozen at rest:

- an AABB of a **rotating** slice is not rotation-invariant, so the world size of a texel changed
  when the camera merely turned;
- nothing aligned the light-space origin to the texel grid, so the grid slid under the geometry.

Now: **bounding-sphere** fit (radius depends only on the slice geometry, constant per split for a
fixed FOV), radius **quantised** so float wobble in the split maths cannot make the texel size
breathe and defeat the snapping, light-space centre **rounded down to a whole texel**, camera
anchored on that snapped centre. References, technique only: Michal Valient, *Stable Rendering of
Cascaded Shadow Maps*, ShaderX6 (2008); Microsoft's *Cascaded Shadow Maps* D3D sample.

`frustumCenter` finally does something: it had been computed since the first revision and never
read, while a comment three lines below claimed the light camera was positioned — it was not, the
view was a pure rotation about the world origin.

The depth margin, hard-coded at 100 m, becomes **a fraction of the radius**. It was ~5× too large
for cascade 0, meaningless for cascade 3, and refit every tick, so the depth range breathed with the
fit. At `margin = radius` the range is exactly `3 × radius`, a pure function of the split. Anchoring
the camera also deletes the trap that once cost whole cascades: `orthographicProjection()` takes
positive **distances**, not view-space Z coordinates, and the near plane is now trivially the camera.

⚠️ **A sphere wastes map area against a tight AABB.** Measured on `reflexion-debug`, pinned pose and
pinned exposure: no visible loss (palm shadow band mean 68.42 → 68.50), because `cascadeScale = 4`
already concentrates the cascades. **Any CSM demo that looks blockier after this needs its
`csmScale` re-checked** — that is the expected trade, not a regression.

⚠️ Verification is temporal, so a still frame cannot show it: move **slowly** along a shadow edge.
Before, the edge crawled continuously; after, it either holds still or jumps one clean texel.

### The PCF kernel rotation is anchored to the WORLD, not to the screen (Aug 2026)

`generate2DShadowMapPCFCode()` (VogelDisk and PoissonDisk) and the three cubemap paths rotated
their sampling kernel by `fract(sin(dot(gl_FragCoord.xy, …)))`. **No frame index was ever mixed in**,
so the field was fixed in SCREEN space: a surface slides *through* a stationary noise pattern as the
camera moves. That reads as **crawl**, and — the decisive part — **a temporal filter is structurally
unable to remove it**, because averaging a value that is constant in time returns that constant. TAA
could never have helped.

Now hashed from `svPositionWorldSpace.xyz`. The rotation belongs to the surface: identical every
frame for a given world point, so nothing crawls. World space rather than light space on purpose —
a light-space anchor is stable only while the LIGHT is still, and a carried torch is the opposite.

⚠️⚠️ **A varying referenced must be a varying requested.** The world position was only ever
forwarded on the CSM path; hashing it from the 2D and cubemap PCF paths without adding
`requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, ToNextStage)` for them emitted
`'svPositionWorldSpace' : undeclared identifier`. A fragment shader that cannot compile calls
`setBroken()` on the instance, which removes the renderable from the scene **entirely** — a
disappearing object, not an error the eye can attribute to a shader. The request now covers every
PCF path.

⚠️ The log wording for this is **`Unable to compile shader '<name>'`** (`Saphir/ShaderManager.cpp`).
Grepping for "failed to compile" finds nothing and reads as success.

⚠️ Engine default is PCF **off** (`DefaultGraphicsShadowMappingEnablePCF`), so a machine that turns
it on opts into this path. `generateCSMShadowMapCode()` never used the hash at all.

⚠️ Amplitude caveat, unchanged: with `PCFSamples = 4` the kernel is 81 taps inside a **one-texel**
radius, a grossly over-converged estimator, so the rotation moves the result very little either way.
What the change removes is the screen-space anchoring, not a large error.

### B3 — inter-cascade blending: DONE (Aug 2026)

`generateCSMShadowMapCode()` selected one cascade with a break-on-first-hit loop and applied a single
matrix, so the boundary was a hard plane **locked to the camera**, between two texel grids that are
not aligned with each other. A static object's shadow switched grid in one frame as the camera
advanced — a localised pop travelling with you along the split distance, not a shimmer.

The per-cascade sample is now emitted **once**, as a GLSL function, and called twice inside a
cross-fade band: `mix(sample(n), sample(n+1), blend)` where `blend` ramps over the last
`CascadeBlendRatio` fraction of the cascade's own depth range — a fraction, so the band scales with
the split instead of being a fixed number of metres.

`Core/Graphics/ShadowMapping/CascadeBlendRatio`, default **0.1**, clamped to [0, 0.5]. **0 emits
nothing at all** — no branch, no second sample, no cost — matching how `PCFSamples`/`EnablePCF` are
already baked as GLSL literals at generator construction.

⚠️ **COST:** inside the band a fragment pays the PCF kernel **twice**. The band is a thin shell so the
average is small, but it is not free; lower or zero the ratio on a fill-bound scene.

⚠️⚠️ **EVERY input of the helper is a PARAMETER, and that is not style.** A function is emitted at
FILE SCOPE and the generator declares it BEFORE the uniform blocks: a body naming `ubLight` or the
sampler directly compiles to `'ubLight' : undeclared identifier`. Six shaders failed that way on the
first attempt — and a fragment shader that cannot compile calls `setBroken()`, which removes the
renderable from the scene entirely, so the demo simply fell apart rather than reporting a shader
problem. GLSL allows an opaque sampler as a function parameter; use that.

⚠️ Honest limit of the verification: the artefact this fixes was never visible on `reflexion-debug`
to begin with, so what is demonstrated here is that the mechanism is emitted, that every shader
compiles, that there is zero VUID and that the image outside the band is unchanged (palm shadow band
68.50 -> 69.24 at a pinned pose and exposure). The seam itself still awaits a scene that shows it.

### The old note, kept for its recognition signature


Deliberately left, at the project owner's call, until it is seen on screen. Recording what it is and
how to recognise it, so the next session does not have to rediscover it:

- **What:** `generateCSMShadowMapCode()` selects one cascade with a break-on-first-hit loop and
  applies a single matrix. The boundary is a hard plane **locked to the camera**, between two texel
  grids that are not aligned with each other.
- **How you would notice:** a static object's shadow switching grid in one frame as the camera
  advances — a localised pop travelling with you along the split distance, not a shimmer. Most
  visible on a long shadow crossing a split, at a low `cascadeScale` where the splits sit close.
- **Cost of the fix:** a second PCF kernel inside the blend band. With `PCFSamples = 4` that is 81
  extra taps for the fragments in the band.
- **Shape:** factor the per-cascade sample into one emitted helper, then sample cascade *n* and
  *n+1* in the band and `mix()` them by the normalised distance to the split. The band width belongs
  in a settings key baked as a GLSL literal — matching how `PCFSamples`/`EnablePCF` already work — so
  a ratio of 0 compiles the branch away entirely and costs nothing when off.

## Per-Light Shadow Configuration

Each light component can independently configure shadow mapping:

### Shadow Map Resolution

```cpp
light->setShadowMapResolution(1024);  // Power of 2 recommended
```

Resolution of 0 disables shadow mapping for that light.

### Shadow Bias

```cpp
light->setShadowBias(0.005F);  // Prevent shadow acne
```

Bias offsets depth comparison to prevent self-shadowing artifacts.

### PCF Radius

```cpp
light->setPCFRadius(2.0F);  // Filter radius in texels
```

Larger radius = softer shadows but more blurring.

## Light Descriptor Sets

Each light uses one of two descriptor set configurations:

**Without shadow map** — shared UBO-only descriptor set (from `SharedUniformBuffer`):

| Binding | Content |
|---------|---------|
| 0 | Light UBO (dynamic offset) |

**With shadow map** — dedicated per-light descriptor set:

| Binding | Content |
|---------|---------|
| 0 | Light UBO (dynamic offset) |
| 1 | Shadow map sampler (2D, Cube, or 2DArrayShadow) |

**Color projection** is handled via the global `BindlessTextureManager` descriptor set, not via per-light descriptor sets. The light UBO carries a `ColorProjectionIndex` field (`uint` encoded as `bit_cast<float>`) that indexes into the bindless 2D or Cube texture array. When no texture is assigned, the sentinel value `0xFFFFFFFF` causes the shader to skip sampling (`projectionColor = vec3(1.0)`).

**Why bindless for color projection?** Per-light descriptor sets use `UNIFORM_BUFFER_DYNAMIC` at binding 0, which does not support `UPDATE_AFTER_BIND_BIT`. This makes deferred texture writes unsafe with frames-in-flight. The bindless set uses `UPDATE_AFTER_BIND_BIT` + `PARTIALLY_BOUND_BIT`, allowing textures to be registered asynchronously after resource loading completes via `ObserverTrait` notification.

**Code references:**
- `Scenes/Component/SpotLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Scenes/Component/PointLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Scenes/Component/DirectionalLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Scenes/Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` - Bindless registration
- `Graphics/BindlessTextureManager.hpp` - Global bindless descriptor set

## Render Pass Types

The shader system generates different programs per pass type. Each `RenderPassType` is a combinatorial variant encoding light type + shadow mode + color projection:

### Directional Light

| Pass Type | Shadow | Color Projection | Use Case |
|-----------|--------|-------------------|----------|
| `DirectionalLightPass` | No | No | Base directional, no extras |
| `DirectionalLightPassShadowMap` | 2D | No | Standard shadow map |
| `DirectionalLightPassCSM` | CSM | No | Cascaded Shadow Maps |
| `DirectionalLightPassColorMap` | No | Yes | Color projection only |
| `DirectionalLightPassFull` | 2D | Yes | Shadow + color projection |
| `DirectionalLightPassFullCSM` | CSM | Yes | CSM + color projection |

### Point Light

| Pass Type | Shadow | Color Projection | Use Case |
|-----------|--------|-------------------|----------|
| `PointLightPass` | No | No | Base point light |
| `PointLightPassShadowMap` | Cube | No | Cubemap shadow |
| `PointLightPassColorMap` | No | Yes | Color projection only |
| `PointLightPassFull` | Cube | Yes | Shadow + color projection |

### Spot Light

| Pass Type | Shadow | Color Projection | Use Case |
|-----------|--------|-------------------|----------|
| `SpotLightPass` | No | No | Base spotlight |
| `SpotLightPassShadowMap` | 2D | No | Standard shadow map |
| `SpotLightPassColorMap` | No | Yes | Color projection only |
| `SpotLightPassFull` | 2D | Yes | Shadow + color projection |

### Helper Functions

| Function | Purpose |
|----------|---------|
| `renderPassUsesShadowMap(type)` | Returns true for `*ShadowMap`, `*CSM`, `*Full`, `*FullCSM` |
| `renderPassUsesCSM(type)` | Returns true for `*CSM`, `*FullCSM` |
| `renderPassUsesColorProjection(type)` | Returns true for `*ColorMap`, `*Full`, `*FullCSM` |

**Code reference:** `Graphics/Types.hpp` — Enum definition and helper functions

## Color Projection

Color projection allows a light to project a texture onto surfaces (like a gobo/light mask). It works **independently** of shadow maps — a light can project colors without the overhead of rendering a shadow map.

### How It Works

1. **Texture assignment:** `light->setColorProjectionTexture(texture)` assigns a 2D or cubemap texture
2. **Pass type selection:** `Scene.rendering.cpp` selects `*ColorMap` or `*Full` pass type based on `hasColorProjectionTexture()`
3. **Shader generation:** The `LightGenerator` generates bindless sampling code using the UBO's `ColorProjectionIndex`
4. **Projection coordinates:** Uses the light's `ViewProjectionMatrix` (from UBO) to project fragment position into light space

### UV Coordinate Convention

> [!WARNING]
> **ScaleBiasMatrix is already baked into ViewProjectionMatrix!**
>
> The `RenderTarget::ScaleBiasMatrix` transforms clip-space [-1,1] to UV [0,1]. It is pre-multiplied into the light's `ViewProjectionMatrix` stored in the UBO.
>
> Shadow maps use `textureProj()` which handles perspective divide + bias automatically.
> Color projection does the perspective divide manually (`projCoords = .xyz / .w`), so UVs are already in [0,1].
>
> **Do NOT apply additional `* 0.5 + 0.5` to color projection UVs** — this causes a double-bias offset.

```glsl
// CORRECT — ScaleBiasMatrix already in ViewProjectionMatrix
const vec3 projCoords = svPositionLightSpace.xyz / svPositionLightSpace.w;
uint cpIdx = floatBitsToUint(uLight.ColorProjectionIndex);
if ( cpIdx != 0xFFFFFFFFu ) {
    projectionColor = texture(uBindlessTextures2D[nonuniformEXT(cpIdx)], projCoords.xy).rgb;
}

// WRONG — double bias, pattern is offset
projectionColor = texture(uBindlessTextures2D[nonuniformEXT(cpIdx)], projCoords.xy * 0.5 + 0.5).rgb;
```

### Light Type Specifics

| Light Type | Projection Method | Bindless Array |
|------------|-------------------|----------------|
| Spot | 2D projection via ViewProjectionMatrix | `sampler2D[]` (binding 1) |
| Directional | 2D projection via ViewProjectionMatrix (non-CSM only) | `sampler2D[]` (binding 1) |
| Point | Cubemap lookup via DirectionWorldSpace | `samplerCube[]` (binding 3) |

**Note:** CSM directional lights cannot use color projection (CSM computes light-space position per-cascade in the fragment shader, which is incompatible with a single projection texture).

### When No Texture Assigned

The `ColorProjectionIndex` in the UBO is set to `0xFFFFFFFF` (sentinel). The shader checks `cpIdx != 0xFFFFFFFFu` before sampling — when no texture is assigned, `projectionColor` remains `vec3(1.0)` (hardcoded default). No bindless texture access occurs, and the SPIR-V compiler may optimize out the multiplication entirely.

**Code references:**
- `Saphir/LightGenerator.PerFragment.cpp` — Color projection sampling (all 4 shading variants)
- `Scenes/Component/AbstractLightEmitter.hpp:setColorProjectionTexture()` — Texture assignment
- `Scenes/Scene.rendering.cpp:renderLightedSelection()` — Pass type selection logic
- `Graphics/Types.hpp:renderPassUsesColorProjection()` — Helper function

## Image Layout Lifecycle

Shadow map images follow this layout progression:

1. **Creation:** `VK_IMAGE_LAYOUT_UNDEFINED`
2. **Still at creation, in `ShadowMap::createImages()`:** transition to `TRANSFER_DST_OPTIMAL` →
   `clearDepthImage(1.0F)` → transition to `DEPTH_STENCIL_READ_ONLY_OPTIMAL`. The descriptor set
   is written as soon as the light is created while the first shadow pass only runs on the next
   rendered frame; a light pass in between would sample whatever the allocation happened to hold,
   and undefined depth below the receiver's reference reads as SHADOW — a one-frame black flash
   on scene load. 1.0 is the far plane, i.e. "nothing occludes".
3. **Shadow pass (render pass):** `initialLayout` and `finalLayout` are both
   `DEPTH_STENCIL_READ_ONLY_OPTIMAL`; the subpass flips to `DEPTH_STENCIL_ATTACHMENT_OPTIMAL`
   while writing, with `loadOp = CLEAR`.
4. **During lighting pass:** sampled as texture, from `DEPTH_STENCIL_READ_ONLY_OPTIMAL`.

> [!CAUTION]
> Step 2 is why the depth image **must** carry `VK_IMAGE_USAGE_TRANSFER_DST_BIT` on top of
> `DEPTH_STENCIL_ATTACHMENT_BIT | SAMPLED_BIT`. It was missing between `64f71ade` and the Aug 2026
> fix: both barriers and the clear were rejected, the image never left `UNDEFINED`, the render
> pass `initialLayout` above became unreachable, and **`vkQueueSubmit` returned
> `VK_ERROR_VALIDATION_FAILED_EXT` for the shadow pass on every frame** — directional shadows
> silently gone. The NVIDIA driver hid it completely with the validation layers off. Full
> post-mortem: `docs/caution-points.md` § Vulkan Validation.

**Critical:** If shadow rendering is skipped (global setting disabled), the images are still
cleared and left in `DEPTH_STENCIL_READ_ONLY_OPTIMAL` at creation, so binding them to descriptors
is safe — they simply read as "nothing occludes".

## Settings Summary

| Setting Key | Type | Default | Description |
|-------------|------|---------|-------------|
| `GraphicsShadowMappingEnabledKey` | bool | true | Global shadow mapping enable |
| `GraphicsShadowMappingEnablePCFKey` | bool | **false** | PCF soft shadows enable |
| `GraphicsShadowMappingPCFMethodKey` | string | "Balanced" | PCF sampling method ("Performance", "Balanced", "Quality", "Ultra") |
| `GraphicsShadowMappingPCFSamplesKey` | int | 2 | PCF sample count (for Grid) |
| `GraphicsShadowMappingViewDistanceKey` | float | 5000.0 | ⚠️ **DEAD KEY** — no code reads it through `Settings`. Only the compile-time default is used, as a spot/point radius fallback (`SpotLight.hpp`, `PointLight.hpp`). Changing it in a config file does nothing |

**Code reference:** `SettingKeys.hpp` - All shadow mapping setting keys
