## 45. The sun under a sea — its light dimming with depth, and the caustics of the waves (Nov 2026)

Until this lot a point under the sea level was lit as in the air: the underwater view (graphics doc 13-12 § 44)
handles what lies between the EYE and what it sees, nothing of the LIGHT's own path. A deep seabed kept its sunlit
sand colour, and no wave drew anything on it.

Owner decisions:
- 2026-11-23: the light's own path — the sun dimming and turning blue with depth on what it lights — is handled in
  the LIGHTING, for both views (from above through the surface, from under it), with the caustics; not as a
  screen-space factor of the underwater effect.
- 2026-11-25 (AskUserQuestion): the caustics' pattern comes from the sea's OWN waves (not an animated texture); the
  reference is a **realistic lagoon** — a sharp net in 1 to 3 m of water, softer below, gone by 15 to 20 m; the
  lights CARRY what they need through the bindless table, as the sun carries the clouds' shadow; and the waves'
  curvature is produced per cascade by the ocean's FFT pass (a window map around the camera was his first pick and was
  dropped with him: at 1024² over 256 m its texel is 25 cm, the 8 m cascade's is 3 cm — the ripples that draw the
  net of a shallow water would not be in it).

### 1. What is computed (the PBR light pass of a directional light)

`Saphir::LightGenerator`, `generatePBRFragmentShader()`, beside the clouds' shadow: `seaLight`, a factor of the
light's radiance, for a point under the surface of the scene's sea.

- **The surface crossed**: Schlick's transmission (F0 = 0.02) and the refraction (n = 1.33).
- **The absorption along the light's own path**, per channel: `exp(-attenuation × depth / cos(refracted angle))`,
  the attenuation being `WaterMedium::diffuseAttenuation(1)` — the absorption plus the backscattering, per metre of
  path.
- **The caustics**: a ray that crosses the surface at x lands at x' = x + path × (the refracted direction − k ∇h(x)),
  k = 1 − 1/n = 0.248, so the irradiance is `1 / |det(I − k × path × H)|`, H the Hessian of the wave height. The
  factor is bounded at 8 (|det| ≥ 0.125), weighed by the strength and by `exp(-depth / fadeDepth)`.
- **The low-pass with depth**: each cascade's curvature is read at the mip whose texel is
  `softening × depth × the cascade's weight` (and never finer than the pixel's footprint on the ground). The waves
  shorter than that have crossed their focus above the receiver and their light is spread again: it is what turns
  the sharp net of a shallow water into the soft dapple of a deep one. ⚠️ ONE branch of the refraction map is
  evaluated: past the first fold the true irradiance is a sum over several surface points, and that low-pass is what
  keeps the single branch honest.
- **The shelter map**: a cascade weighs `mix(shelteredShare, 1, map)`, like the surface itself (graphics doc 44). A
  sheltered water's waves are that much flatter, so they focus that much deeper: the weight NARROWS the low-pass too
  (without it the calm basin of `water-world` showed no caustics at all — its ripples were averaged out).
- **Where the surface really stands** (owner, 2026-11-25, on screen: "quand on est au dessus de l'eau, les
  caustiques s'affichent alors que les vagues se retirent"): within the waves' reach of the level, the depth is
  taken from the displacement cascades, read at the lattice point one round of x − D(x) lands on; a beach a wave has
  just left is in the air and gets neither the caustics nor the absorption, a point under a crest above the level
  gets both. Deeper than the reach, the level at rest is used (the rise fades between half the reach and the reach).

The sea's OWN surface material is not under itself (`m_surfaceIsVolumeInterface`). Every directional light carries
the sea (the moon's light is absorbed and focused like the sun's). References: J. Guardado, D. Sánchez-Crespo,
"Rendering Water Caustics", GPU Gems, ch. 2, 2004; E. Wallace, "WebGL Water", 2011 (the irradiance as the area ratio
of the refracted wavefront).

### 2. Who carries what

| Step | Where |
|---|---|
| The curvature of each cascade — (∂²h/∂x², ∂²h/∂z², ∂²h/∂x∂z) in 1/m, central differences of the slopes over the wrapping tile — written by the resolve pass into one more RGBA16F 2D array **with a mip chain rebuilt every update** (eight linear blits, GENERAL layout); one 2D view a cascade, and one a cascade over the displacement | `Graphics::OceanWaves` (`CurvatureMipLevels`, `curvatureView()`, `displacementLayerView()`) |
| The seven textures a table can hold (three curvatures with a repeat / trilinear sampler, the shelter map, three displacements), the cascade sizes, the shares and the map's rectangle of the frame, the reach, the tuning | `Graphics::SeaCaustics`, answered by `Scenes::SeaLevelInterface::caustics()` — the ocean; a sea at rest answers none and gets the absorption alone |
| A texture over objects that already exist (an image, one view, a sampler) | `Graphics::ImageViewTexture` (the imposter atlas' local copy of it was replaced by it) |
| RENDER THREAD, every frame before the bindless sync: the textures filed in the scene's bindless set (re-filed when the sea or its GPU resources change), the sea's light noted | `Scenes::Scene::syncSeaLight()`, called by `Graphics::Renderer` |
| LOGIC THREAD, every tick: every directional light takes it | `Scenes::Scene::updateSeaLight()` → `Component::DirectionalLight::updateSeaLight()` |
| Seven vec4 at the END of both directional blocks (classic: floats 52-79, cascaded: 104-131; `MaxUniformBlockElementCount` 104 → 132) | `Scenes::SeaLight`, `Saphir::Keys::UniformBlock::Component::SeaWater` … `SeaSurface` |
| The traced lanes (RTR, RTGI, the irradiance probes): the transmission and the absorption, WITHOUT the caustics (a traced hit takes their mean, 1) and from the level at rest; one more vec4 a traced light entry (nine → ten) | `Scenes::LightSet::GPULightData`, `emTracedLightThroughSea()` (`TracedLightGLSL.hpp`) |

VRAM: the curvature array is 256² × 3 layers × RGBA16F with its mips, about 2.1 MiB. A lit pixel under the sea
fetches the shelter map and three curvature mips; within the waves' reach of the level, the shelter map once more and
six displacement texels.

### 3. Tuned while the scene runs

| Command (`Core.SceneManagerService.`) | Effect |
|---|---|
| `getSea()` | adds `lightThroughWater` (bool) and `caustics` — `strength`, `softening`, `fadeDepth`, `temporalResponse` — `null` for a sea that draws none |
| `setSeaCaustics(strength, softening, fadeDepth, temporalResponse)` | the next frame. Refused unless the strength is 0-1, the softening 0 or more, the fade depth more than 0, the temporal response 0-1 (FOUR arguments since 2026-11-25) |
| `setSeaLight(enabled)` | the whole term on or off (the absorption and the caustics): off, what lies under the sea is lit as in the air — the comparison the look is judged with |

`Scenes::SeaCausticSettings`, `SeaLevelInterface::causticSettings()` / `setCausticSettings()`;
`Scene::enableSeaLight()`. Not saved: `OceanSurfaceParameters::causticStrength` / `causticSoftening` /
`causticFadeDepth` / `causticTemporalResponse` are the values at LOAD (1, 0.15, 8 m, 1).

### 4. Verified (2026-11-25, Linux, RTX 3070 Ti, driver 620.70.10, `water-world`, ray-traced lane)

Window-less test instances, the validation layer loaded and proven, **0 VUID**, no shader that fails to compile,
exposure pinned at f/11 – 1/250 s – ISO 100; sheets of twelve captures (four poses × off / absorption alone / with
the caustics):
- the calm basin from under water (1 to 2.5 m of water): soft patches of light on the sand with the caustics, none
  without; the sand dimmer and less red with the absorption;
- the slope toward the open sea from under water (2 to 5 m): the pattern is there, discreet; 6 to 13 m: a faint
  dapple;
- from above the beach: the net shows through the surface near the shore, the submerged sand is darker and greener.
- The beach a wave leaves: six captures 1.6 s apart show no pattern on the dry sand (judged on thumbnails).
- The calm basin from under water on the final binary, one capture a state, the sand's half of the frame (rows
  360-720, luma / 255): term off 109.5 (standard deviation 7.65); absorption alone 102.0 (6.91, 99th percentile 114);
  with the caustics 103.0 (9.12, 99th percentile 125). The caustics move the contrast, not the mean (+1 %): the
  bounded single branch keeps the energy there. The frame's mean red falls from 98.4 to 90.4 with the term, its
  green from 105.2 to 103.0.
- The screen-space lane, same pose and exposure (a settings copy, 0 VUID): off 104.58 (7.87), absorption alone 97.69
  (7.31), with the caustics 98.85 (10.10); two captures of the absorption alone differ by 0.02 on average, the
  absorption and the caustics by 4.99 (99th percentile 23.6). The pattern's contrast against the softening, mean
  difference with the absorption alone: 0 → 18.82, 0.05 → 14.30, 0.15 → 5.00, 0.6 → 0.79, 2 → 0.30.
- `setSeaCaustics(1, -1, 8)` and `setSeaCaustics(1, 0.15, 0)` are refused, the values unchanged.
- `gate.py commit`: every step OK (the ledger's row of the lot).

**The owner on screen the same day**: "ça marche bien", then, after the beach was fixed, "c'est parfait"; with the
term switched off and on again in front of him (`setSeaLight`): "ça change tout ! C'est trop beau". The lagoon seen
from above is darker over its deep sand than the look he approved on 2026-09-28 — seen by him in that comparison.

### ⚠️ Limits

- **The three OS (2026-11-25)**: Linux (RTX 3070 Ti), macOS (Apple M6, MoltenVK, screen-space lane) and Windows
  (RTX 3500 Ada, ray-traced lane, MSVC Release and Debug) build without a warning, run with 0 VUID and no shader
  failure, answer the same `getSea()`, refuse the same values, and give the same means lane for lane (screen space:
  off 104.4 to 104.6, absorption 97.5 to 97.7; ray-traced: off 108.7 to 109.5, absorption 102.0 to 102.1). NOT run: AMD.
- ⚠️ **Temporal anti-aliasing halves the caustics' contrast**: the pattern moves over a still receiver, no motion
  vector follows it, the history averages it. Found as "macOS and Windows draw soft patches where Linux draws a net":
  the Linux owner settings have `TemporalAA/Enabled` false, the two others true. Linux with the key set (settings
  copy, screen-space lane), mean difference with the absorption alone at softening 0 / 0.05 / 0.15: 13.99 / 9.18 /
  2.21, against 18.82 / 14.30 / 5.00 without, and macOS 14.34 / 9.38 / 1.54, Windows 13.68 / 8.85 / 1.97 to 2.15
  (standard deviation at softening 0: 14.6, against 28.0 without; macOS 13.3, Windows 14.1). Ruled out by
  measurement on the way: the lane, the frame rate (capped at 30 a second: same numbers), the screen derivative
  (below). ⚠️ The owner judged the look and kept the first values with the anti-aliasing OFF.
- **The answer (owner decision, 2026-11-25: a GRADED reactive mask)**: the sun's light pass writes, into the temporal
  passes' reactive mask, `0.49 × temporalResponse × the caustics' weight` (their strength, their fade with the depth —
  the weight, not the frame's gain: the dark gap a bright line has just left gives up its history too). The resolve
  then keeps less of its history there, without throwing it away (graphics doc 13-12 § 06, the two bands of the
  mask). Linux, the anti-aliasing on, screen-space lane, mean difference with the absorption alone (99th percentile)
  against the response — softening 0.15: 0 → 2.20 (9.6), 0.3 → 4.17 (18.8), 0.6 → 4.40 (18.1), 1 → 4.59 (23.7), where
  the anti-aliasing off gives 5.00; softening 0: 0 → 14.45, 0.3 → 16.35, 0.6 → 17.92, 1 → 18.22 (off: 18.82). `beams`,
  `water-world` and `default` run with it on the ray-traced lane, 0 VUID. **`temporalResponse` is 1**: the owner's value, chosen on
  screen the same day (a window on the ray-traced lane, the anti-aliasing on, 0 / 0.3 / 0.6 / 1 switched in front of
  him). **macOS and Windows the same day (engine a2acc53b)**: builds without a warning, 0 VUID on `water-world`,
  `beams` and `default` (the MAX blend on the reactive attachment runs on MoltenVK), the old three-argument command
  and a response of 1.5 refused; softening 0.15, response 0 → 1: macOS 1.71 → 4.49, Windows 2.13 → 4.54 (Linux 2.20 →
  4.59); softening 0, response 1: a sharp net on both (standard deviation 25.1 and 25.5, Linux 26.7).
- The pixel's footprint (`dFdx` / `dFdy` of the world position) is taken BEFORE the term's branches since 2026-11-25:
  a derivative inside non-uniform control flow is undefined. It changed no number on Linux nor on macOS.
- The traced lanes' factor was compiled and run (0 VUID); its effect on the picture was
  not measured.
- The term is in the PBR light pass only, like the clouds' shadow: a non-PBR material under the sea is lit as in the
  air.
- The surface read near the level is the cascades' sum, not the drawn mesh's interpolation
  (`Generator::ocSurfaceRiseAt()`), with ONE round of x − D(x) and without the mesh's fade of a cascade with the
  distance: in a strong swell the edge of the caustics can stand off the drawn water's edge. Not measured.
- The curvature is taken at the UNDISPLACED lattice point: the choppy displacement is ignored by the focusing.
- The first values (strength 1, softening 0.15, fade 8 m) are KEPT by the owner (2026-11-25, on screen, the tuning
  commands at hand: "c'est parfait").
- The cost of the term in the light pass (2026-11-25, RTX 3070 Ti, driver 620.70.10, `water-world` window-less
  1280×720, ray-traced lane, no validation layer, the machine idle, the calm basin from under water — sand over half
  the frame; five alternated 8 s windows a state, milliseconds a frame from `renderedFrame`): term off 8.805 median
  (8.737 to 9.100), absorption alone 8.868 (8.841 to 9.161), with the caustics 9.118 (8.948 to 9.236). The absorption
  is inside the noise; the caustics cost about 0.3 ms (3.5 %) at that pose, the ranges overlapping. ⚠️ NOT in that
  figure: the curvature written by the resolve pass and its eight blits an update, which `setSeaLight(false)` does not
  stop; no measurement at the window's real size (5K).

### 5. The sun's shafts in the water (2026-11-25) — the same caustics, seen in the volume

The caustics are what the waves make of the sun's light on the seabed; on its way down that light is already
concentrated and spread, and the water scatters it toward the eye: the shafts. `Effects::Atmosphere::WaterVolume`
draws them (graphics doc 13-12 § 44 holds the rest of that effect).

Owner decisions (2026-11-25, AskUserQuestion): the reference is the **realistic lagoon** (discreet beams, seen
mostly near the surface and toward the sun); what cuts them is **the waves and the shadows**; the march lives **in
`WaterVolume`** (the one owner of the water between the eye and what it sees — neither a separate effect, nor a
family with the air's march); the scattering starts **the same in every direction** (the brightness he approved),
the forward share being his to raise on screen; a console command to compare (`setSeaShafts`). **On screen the same
day** (a window, open water, the shafts off, then on with an anisotropy of 0, 0.3 and 0.6 switched in front of him):
he keeps **strength 1, anisotropy 0.6** — the beams gathered toward the sun, the water darker looking away from it,
knowingly over the even brightness approved before. What the air's
`VolumetricLight` still draws under water (a glow around the sun through the surface) is judged on screen once the
shafts exist — open.

**What is computed.** The closed form of the water's own light assumes the same sun at every point of a depth. Two
more pipeline variants (the surface at rest or moving × the sun's share in closed form or MARCHED) take the SUN's
share out of it — the sky's stays in the closed form — and add what changes it along the pixel's path in the water:

`L_sun = P(θ) · S_sun · e^(−Kd z₀) · [ ∫ e^(−r t) dt + strength · Σ (Mᵢ − 1) · ∫ᵢ e^(−r t) dt ]`

- 24 steps (`WaterVolume::ShaftSampleCount`) over the path, at most six extinction lengths; each step weighs its EXACT
  share of the integral, so a water nothing modulates gives the closed form to the bit, and the part of a long path
  beyond the march keeps it. The origin is dithered by a fraction of a step (`MarchDitherGLSL.hpp`).
- `M` at a point = the sun's cascaded shadow there (`CSMSamplingGLSL.hpp`) × the caustics' gain: the point's depth,
  the place its light crossed the surface up-sun, the three curvature mips and the shelter map — the light pass's
  own term, from ONE statement both shaders take (`Graphics/Effects/Shared/SeaCausticsGLSL.hpp`: the generated light
  pass declares the two functions from its body macros, the effect splices them). The low-pass is never finer than a
  pixel at that distance nor than half a step: what the march cannot resolve is averaged, not aliased.
- `P(θ)` = Henyey-Greenstein over the isotropic 1/4π the source holds: 1 everywhere at `anisotropy` 0.
- A marched variant is taken when the sea draws shafts (`SeaCaustics::shaftStrength` > 0, its curvature and shelter
  textures there) and the sun is up, lit and casts cascaded shadows; else the closed form, with the sun in the one
  source as before. A marched variant binds the cascades, three curvatures, the shelter map and two blocks more
  (`WaterVolume::ShaftBlock`, seven vec4).

| Command (`Core.SceneManagerService.`) | Effect |
|---|---|
| `getSea()` | adds `shafts` — `strength`, `anisotropy` — `null` for a sea that draws none |
| `setSeaShafts(strength, anisotropy)` | the next frame. Refused unless the strength is 0-1 and the anisotropy 0-0.95. Strength 0 = the closed form (the before / after) |

`Scenes::SeaShaftSettings`, `SeaLevelInterface::shaftSettings()` / `setShaftSettings()`. Not saved:
`OceanSurfaceParameters::shaftStrength` / `shaftAnisotropy` are the values at LOAD (1, 0.6).

**Verified (2026-11-25, Linux, RTX 3070 Ti, `water-world`, ray-traced lane, window-less 1280 × 720, exposure
pinned)**: 0 VUID, no shader failure, with the surface at rest and moving; the refusal works.
- Open water (the eye 6 m under the level at (900, 900), looking 20° up in four directions): soft beams come down
  from the surface with the shafts on, none without; the frame's mean rises by 1 to 4 levels. With an anisotropy of
  0.6 the beams gather into a bright glow toward the sun and the water darkens away from it (mean 93 → 51 looking
  away): a look of its own, not the approved one.
- The calm basin (1 to 2.5 m of water): no beam to see — a short path in a water that scatters little, under small
  ripples.
- Cost, the whole frame under water, GPU-bound (five alternated 8 s windows a state): 3.364 ms a frame without,
  3.968 ms with — **0.60 ms at 1280 × 720**. ⚠️ It grows with the pixel count (24 steps × five fetches a pixel):
  about 1.4 ms at 1920 × 1080 by the area, NOT measured; a march at half the resolution is the known answer if a
  large window needs it. At the basin's pose the frame is not GPU-bound (8.09 ms either way): no reading there.

The screen-space lane gives the same picture and numbers at the open-water pose (0 VUID), and the caustics' own numbers are unchanged by the shared functions (softening 0.15: 4.86; 0: 18.74).

**⚠️ Limits.** NOT run: macOS, Windows, AMD. The owner judged the open water; a view near an island's shadow was
not looked at. The shadow
is read along the sun's direction in the AIR: the bend of the light at the surface is not in it. The depth of a
point is taken from the level at rest (the caustics' gain is 1 at the surface anyway). A sea at rest draws no shaft,
even from what shadows it, and a sun without cascaded shadows none either: both fall back on the closed form. No
temporal accumulation of its own: without the anti-aliasing the dither is the grain one sees.
