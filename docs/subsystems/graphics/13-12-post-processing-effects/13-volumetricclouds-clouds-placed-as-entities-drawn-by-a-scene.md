## 12. Post-Processing Effects

### VolumetricClouds — clouds placed as ENTITIES, drawn by a scene-driven pass (Sep 2026)

> [!WARNING]
> **The distance LOD (2026-09-25)**: past one base step the view step grows to `FarStepScale` (2) base steps and
> samples the shape's mip log2(k) — −9 % toward the horizon, invisible on a zoom. ⚠️⚠️ Every box entry is DITHERED,
> including one inside a step: stopping exactly on it re-aligned all pixels and drew contour rings on the far
> clouds. Dead ends, measured: a pixel-footprint LOD (nothing sub-pixel before ~20 km on 1-3 km clouds), a
> directional sky ambient (identical colour, +0.6 ms). The cost is the view steps, not the sun march.
> **Sky tint**: `Look::skyTint` (0-1, default 0) multiplies the scattered light by the sky's hue (zenith irradiance
> over its brightest channel) — `terrain` sets 1.0 against its StormyDays; a blue sky would make blue clouds.
> `docs/caution-points.md` § *The cloud march: a distance LOD that works, and two dead ends*.

**Owner decisions (2026-09-24), do not re-litigate:** fly-through clouds (the VOXEL family, not a
weather-map sky layer), a cloud is an ENTITY placed by hand, its shape comes from a PROCEDURAL
generator, a few HERO clouds (≤ 32 drawn), a realistic white-out inside, **scaling a cloud KEEPS
ITS LOOK**, the pass is a post-process effect in its own slot, it is **automatic** (placing a cloud
is enough), and the clouds' shadows on the world are **stage 2**. Item:
`docs/todo/volumetric-cloud-entities.md`.

**The pieces:**

| Piece | Where | Role |
|---|---|---|
| `Graphics::CloudShapeResource` | `CloudShapeResource.hpp/cpp`, container `CloudShapes` | A cumulus grown from `Parameters` (seed, resolution, ratios, puffs, billows, flat base, edge softness) into an `R8G8` 3D texture: **R** density, **G** a conservative distance to the nearest density (the skip channel, in units of `MaxSkipDistance`, rounded DOWN). Deduplicated BY NAME (`resourceName(parameters)`), grown on the thread pool, full mip chain, its own CLAMP sampler. |
| `Scenes::Component::CloudVolume` | `Scenes/Component/CloudVolume.hpp/cpp` | The cloud: a shape + its box half extents + a dimensionless `Look` (optical thickness, erosion, detail frequency, boiling speed, albedo). Registers the shape in the scene's bindless 3D array once it lands; publishes its look per state slot. |
| `Scenes::CloudSet` | `Scenes/CloudSet.hpp/cpp`, `Scene::cloudSet()` | The scene's registry, filled from `AbstractEntity::CloudVolumeCreated/Destroyed`. Its emptiness is THE switch of the feature. |
| `PostProcessStack::syncSceneEffects()` | `PostProcessStack.cpp` | The scene counterpart of `syncCameraEffects()`: the first frame the set is not empty, it files a `VolumetricClouds` into `EffectSlot::Clouds` (settings-gated by `Clouds/Enabled`). An application that filed its own occupant is left alone. |
| `VolumetricClouds` | `Effects/Atmosphere/VolumetricClouds.hpp/cpp` | The march. `requiresCloudVolumes()` keeps it out of the frame — and never materialized — while the scene holds no cloud. |
| `Toolkit::generateCloud()` | `Scenes/Toolkit.hpp` | The authoring entry point: a NON-collidable entity at the cursor + the component, the shape requested by name. |

**The slot:** `EffectSlot::Clouds`, between `VolumetricLight` and `Fog`. A cloud attenuates what lies
behind it (shafts included), and the atmosphere then attenuates the cloud at the depth of the
SURFACE behind it — the known approximation of compositing a volume before a depth-driven fog.
The pass composites `scene · T + L` itself (the VolumetricScattering pattern): no combine snippet.

**The march, per pixel** — box tests against every drawn cloud (≤ 8 hits kept, front to back),
then ONE march over the UNION of the intervals, from the first entry to `min(last exit, scene
depth)` (a tree inside a cloud is buried in it): at every step, every cloud whose interval holds the
sample adds its extinction and its source — overlapping clouds are one medium. ⚠️⚠️ A first version
marched the sorted clouds ONE AFTER THE OTHER, and two overlapping clouds swapped their compositing
order along the line where their box entries meet: a STRAIGHT SEAM through both (owner-reported on
`forest`, 2026-09-24, once the clouds were 70-130 m and overlapped). Per cloud and per step: empty air skipped by the G channel at mip 0; density = the shape eroded at its shell by a
32³ tileable billowy Worley texture (`Base::Algorithms::WorleyNoise`, owned by the effect, drifting
UP — the boiling); the sun = an optical depth marched toward it INSIDE the cloud on mip 1.5 × the
CSM visibility (the trees shadow the inside of a cloud) × a dual-lobe Henyey-Greenstein (0.8 / −0.3,
70 %) × 4 multiple-scattering octaves (Wrenninge et al. 2013: energy, extinction and anisotropy
halved per octave); the ambient = the baked irradiance cubemap's +Y × the sky luminance on top, the
Lambertian ground bounce (`GroundAlbedo`) underneath, blended by the height in the box; the
energy-conserving step integration (Hillaire 2016). Nits in, nits out: no gain knob. The origin
dither is the static `emInterleavedGradientNoise` (MarchDitherGLSL rule).

⚠️⚠️ **"Keep the look" is ONE line of `execute()`:** the extinction at full density is
`opticalThickness / (2 · world half height)` — recomputed every frame from the ENTITY's published
scale. A fixed extinction in 1/m would turn a shrunk cumulus into mist (a real cumulus, ~0.05 m⁻¹,
is opaque over a kilometre and barely there over 15 m). The detail noise coordinates are in SHAPE
units for the same reason.

⚠️ **The step follows the OPTICAL depth in the dense core** (`MaxStepOpticalDepth` 0.4): the
distance-driven step (1.5 % of the distance, between an eighth of `baseStep` and `baseStep` =
diagonal / `StepCount`) stays in the shell where the density varies. Measured on `forest`, camera
INSIDE a 100 m cloud, 2880×1620, validation on: **13.3 → 5.3 ms**; the opening view 2.0 → 1.8 ms,
the cloud region unchanged (mean |Δ| 0.35/255).

⚠️ **The shape is a DOME**, never a pile of spheres: an ellipsoid filling the box, cut flat at the
base, buds grown on its upper surface (Bouthors & Neyret 2004). The sphere-only growth filled 5-8 %
of the voxels and read 3-5× smaller than its box (a 6.4 m cloud in a ~25 m box); the dome fills
21-23 %. `CloudShapeResource::occupancy()` is traced at every growth.

**Traps met while building it (all fixed in the engine, 2026-09-24):**

- ⚠️⚠️ **A 3D image uploaded ONE slice**: `ImageTransferOperation` copied `imageExtent.depth = 1`
  and its mip blit wrote `z = 1` on both ends. Nothing had ever uploaded a 3D image (the
  `Texture3D` resource path has no registered `VolumetricImages` container): a cloud whose first
  slice is its empty margin sampled as NOTHING, with no error. Both now use the full depth.
- ⚠️⚠️ **An occupant filed after the stack was created was never created.** `addEffect()` files an
  effect ENABLED (every effect is at construction) and `createAll()` covered it only for a stack
  built before the scene starts; `syncSlotSelection()` saw "selected == enabled" and took its
  "nothing changes" early-out forever, and the executor skipped the un-created effect in silence —
  `getStatus()` listed `Clouds: VolumetricCloudsEffect` while nothing was drawn. The early-out now
  also requires `isCreated()`, so any late-filed occupant is materialized the next frame.
- ⚠️ **A silent pass cannot be debugged**: the effect traces a CENSUS when it changes —
  `Clouds drawn: N of M (a without a bindless shape slot yet, b whose shape is not on the GPU, c with
  a degenerate box)`. No census line at all means `execute()` never ran.
- ⚠️ The forest demo's AUTO exposure puts the sky and the clouds in the tone mapper's shoulder
  (cloud 247/255 top AND bottom, sky 232): judge the clouds' shading at a PINNED exposure
  (`Camera.setExposure(<entity>, <camera>, 16, 0.01, 100)`, sunny-16), never on the auto-exposed frame.

#### The clouds' shadow on the world — a Beer shadow map carried by the sun (stage 2 lot 1, Sep 2026)

> **The map sizes itself on the clouds (2026-09-25, owner: automatic).** `Scenes::CloudSet::recordShadowMap()`
> sets its side to `AutomaticCoverageFactor` (8) × the drawn clouds' mean horizontal width, never below
> `Clouds/ShadowCoverage` (the floor, 1024 m — `forest`'s 70-130 m clouds keep exactly 1024 m), re-evaluated
> when the number of drawn clouds changes, never per frame (a texel size that breathes makes the shadows
> crawl). The range the texel rays search along the light encloses every cloud, recomputed each frame from
> their boxes, never below `CloudShadowMap::MinimumDepthRange` (2000 m): the fixed ±2000 m lost the shadow of a
> cloud 1500 m up as soon as the sun dropped under ~45° (4.4 km along the light at 20°). `terrain`, 20 cumulus
> of 300-1000 m: 4 930 m, 4.8 m per texel. An existing map is recorded every frame even with no cloud drawn
> (an empty record clears it).

**Owner decisions (2026-09-24):** the shadow term is **carried by the light** (its matrix and its
bindless slot live in the directional light's uniform block), and the **lit materials come first** —
the volumetric scattering, the clouds among themselves and the ray-traced lanes come later (item).

**The map** — `Graphics::CloudShadowMap` (`CloudShadowMap.hpp/cpp`), a Beer shadow map (S. Hillaire,
*Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite*, SIGGRAPH 2016): an `RGBA16F`
square seen along the sun, one texel ray per texel from the sun side to the far side, **R** = the
depth along the light where the clouds START, **G** = their mean extinction over their thickness,
**B** = their total optical depth. A receiver at depth `d` sees `exp(-min(B, G · max(d − R, 0)))` —
1 above the clouds, the whole optical depth under them, a RAMP inside one (a treetop in a low cloud is
half shadowed, not black — a single transmittance per texel cannot say that). The texel march is the
view march's union rule (every cloud whose interval holds the sample adds its extinction), 48 steps,
the shape at **mip 1 without the detail erosion** — a shadow is soft, and the wisps a receiver could
tell apart are not worth a noise fetch per step. The two passes read ONE description of a cloud:
`Effects/Shared/CloudVolumeGLSL.hpp` holds the std140 `CloudBlock` and its GLSL twin `EmCloud` plus the
three box helpers (the `EMEN_CSM_SAMPLING_GLSL` technique), and `VolumetricClouds::gatherClouds()` fills
the array for both.

**Who does what:**

| Step | Thread | Where |
|---|---|---|
| The map's frame: an orthonormal frame around the propagation direction, centred on the camera and **snapped to the texel grid** (a map sliding by a fraction of a texel re-samples every cloud edge each frame, the shadows crawl); depth measured from the camera's plane, so a half float carries it | logic | `DirectionalLight::updateCloudShadow()`, called by `Scene::updateCloudShadows()` after the CSM update — the MAIN sun only, every other directional light gets `disableCloudShadow()` |
| The matrix and the slot are published with the light block (triple-buffered state) | logic → render | `DirectionalLight` layouts: classic `CloudShadowMatrixOffset` 32 / `CloudShadowIndexOffset` 48 (52 floats), CSM 84 / 100 (104 floats = `AbstractLightEmitter::MaxUniformBlockElementCount`) |
| The map is created (settings, once), registered in the scene's bindless 2D array, and recorded **before the scene pass**, only on a frame whose PUBLISHED sun block already reads it | render | `CloudSet::recordShadowMap()` via `Scene::recordCloudShadowMap()`, profiler zone `CloudShadowMap`, in both `Renderer::renderFrame*` paths after the TLAS build |
| The lookup | GPU | every PBR directional pass (classic and CSM) when bindless is on: one `PositionWorldSpace` varying, a UNIFORM branch on `floatBitsToUint(cloudShadowIndex) != 0xFFFFFFFF`, the transmittance multiplies the radiance (`LightGenerator.PBR.cpp`) |

⚠️ **No new pass type, on purpose.** The `RenderPassType` variants are precompiled per renderable, so a
"directional + cloud shadow" family would have doubled the directional programs. The lookup is a
branch inside the EXISTING directional variants; a scene without clouds pays one uniform test per
fragment. ⚠️ The directional block now ALWAYS declares its full layout (colour projection and shadow
members included) so the two cloud members sit at a fixed offset in every variant.

**Measured (forest, 3070 Ti, 2880×1620, validation ON, 2026-09-24):** the map pass **0.20 ms** (max
0.27); the lookup is below the run-to-run noise of the scene pass (8.61 ms with, 9.12 ms without). A/B
from straight above at a pinned sunny-16, `Clouds/ShadowsEnabled` true vs false: the ground in a cloud
shadow reads **0.31-0.35** of its sunlit luminance (scene-referred: gamma, then the inverse ACES fit —
the sky ambient stays), and **1.004** outside any shadow (the control). 0 VUID.

- ⚠️⚠️ **A LOW sun draws a cloud shadow as a long band with STRAIGHT, PARALLEL sides** — that is the
  geometry, not a clipped map. At 29° of elevation a 60 m-high cloud's shadow is stretched ×2 along
  the sun's horizontal direction, so every bud of its silhouette becomes a streak along it. Checked by
  projection on `forest`: `Cloud145` (centre (70.7, 72.3, 66.8)) lands exactly on the band the A/B
  shows. The signature of a real map-edge defect is different: an edge along the sun's horizontal
  direction at the `u = 0 / 1` bound means a coverage that is too small, an edge PERPENDICULAR to it
  means the depth comparison (`d` vs `R`) is wrong. Measure the edge's direction before suspecting
  either.
- ⚠️ **Judge the shadow by a DIFFERENTIAL capture**, never on one frame: the demo's trees and their
  CSM shadows hide it. Same pose, same pinned exposure, `ShadowsEnabled` toggled across TWO runs (the
  setting is read once), luminance difference image. The clouds themselves also differ in that image
  (boiling time) — only the ground is the measurement.

#### The light shafts and the lens flare see the clouds — the cloud transmittance pairing (Sep 2026)

**The defect (owner report, 2026-09-24): "god rays and flare pass through the clouds".** Both find
their occluders in the DEPTH buffer — `VolumetricLight` counts a far-plane pixel as a light source,
`LensFlare` probes 16 taps around the projected sun — and the clouds write no depth.

**The fix (owner decision: per-pixel transmittance):** the cloud pass already computes the view
transmittance `T` of every pixel; it now writes it to a SECOND render target (`VC_Transmittance`,
`R16_SFLOAT`, full resolution, 1 = clear) in the same pass — a two-attachment render pass built on the
`DenoisePass` model, recorded through the new multiple-target overloads of
`IndirectPostProcessEffect::createFullscreenPipeline()` / `recordFullscreenPass()`. The consumers
multiply what they read in the depth by `T` at the same place: `isLit *= T` in the shafts' occlusion
mask, each sky tap of the flare's probe weighted by `T`.

| Piece | Where |
|---|---|
| Protocol | `IndirectPostProcessEffect::cloudTransmittanceTexture()` (producer), `consumesCloudTransmittance()` + `setCloudTransmittanceSource()` (consumers) — the shape of the occlusion lane |
| Wiring | `PostProcessStack::syncSlotPairings(clouds)`, every frame: the enabled `Clouds` occupant → the enabled `VolumetricLight` and `LensFlare` occupants |
| Order | `EffectSlot::VolumetricLight` moved AFTER `Clouds` (was before): a consumer must run after the producer. `LensFlare` already did (camera phase) |
| Fallback | no pairing ⇒ nullptr ⇒ the depth is bound in its place and a push-constant flag keeps the old depth-only test, bit for bit |

⚠️⚠️ **A consumer must never read a transmittance nobody wrote this frame.** Two rules hold it: the
pairing exists only while the scene holds clouds — the executor's own condition to run the producer
(`canOccupantRun()` / `PostProcessor::execute()`); and the producer no longer returns early when 0
clouds are drawn (shapes still growing on the thread pool) — it runs with 0 clouds and writes `T = 1`.

**Measured (forest, 2026-09-24):** with the sun behind `Cloud145`, no flare and no shaft from behind the
cloud, the shafts come only from the sky between the trees; the earlier build drew both at full
strength through it. 0 VUID. Costs: one R16F full-res write in the cloud pass, and one more generated
combine pass — `VolumetricLight` no longer shares the combine group of the indirect terms.
⚠️ A shaft passing IN FRONT of a cloud is no longer attenuated by it (the shafts are added after the
clouds); a shaft can no longer START behind one, which is what shows.
⚠️ The rainbow streaks the flare drew outdoors were NOT this: its bright pass compared a threshold of
0.8 with the chain colour in NITS, so the whole sky fed its ghosts (fixed the same day, § LensFlare).

**Stage 2 lot 1 limits (by decision, see the item):** a cloud does not shadow another (each one's sun
optical depth is marched inside ITSELF only), the volumetric scattering ignores the clouds' shadow, a
transparent object in front of a cloud is drawn behind it (the fog's limitation), and the ray-traced
lanes (RTR/RTGI hits, their sun term) see neither the clouds nor their shadow.
