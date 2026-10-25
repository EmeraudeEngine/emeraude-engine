## 42. The physical sky — a planet's atmosphere drawn as the background (Oct 2026)

> Owner decisions (2026-10-24): the sky is PHYSICAL and must render extraterrestrial skies (the planet is data);
> it is drawn as a **background material** on the skybox cube; the **CelestialClock** writes the scene's sky state;
> **one** scattering sun; an AI-owned lab demo (`sky-lab`, projet-alpha). Lot 1 of the dynamic sky; lot 2, the sun's
> light from the same atmosphere, is § "The light"; lot 3, the sky's own IBL, is § "The environment" (2026-10-25); the
> next lots are the engine items `dynamic-sky-stars-and-milky-way`, `dynamic-sky-moon` and
> `dynamic-sky-aerial-perspective`.

### The model
S. Hillaire, *A Scalable and Production Ready Sky and Atmosphere Rendering Technique*, EGSR 2020 (the model of UE's
`SkyAtmosphere`), after his MIT reference implementation; the transmittance parametrisation is Bruneton & Neyret's
(EGSR 2008). A Rayleigh layer, a Mie layer and an absorbing layer (Earth's ozone, a tent profile), all exponential
or piecewise-linear in height. **Units: kilometres, per kilometre; the planet's centre is the origin.**

| Table | Size, format | Built | Holds |
|---|---|---|---|
| Transmittance | 256×64 RGBA32F | once, at creation | transmittance to the top, per (height, zenith cosine) |
| Multiple scattering | 32×32 RGBA32F | once, at creation | Hillaire's isotropic second-order transfer f_ms, summed as a series |
| Sky-view | 192×108 RGBA32F | **every frame** | in-scattered radiance **per lux** of the star, around the camera: x the azimuth from the star, y the zenith (non-linear near the horizon) |
| Frame | 4×1 RGBA32F | every frame | texel 0: unit vector toward the star + cos of its angular radius; texel 1: its illuminance above the atmosphere × colour (lux) + the camera's distance to the centre (km) |

⚠️ **The three tables are RGBA32F** (2026-10-25, owner decision; `SkyAtmosphere::tableFormat()`): they hold radiance
PER LUX of the star, and half floats flushed everything under 2^-24 per lux — ~0.008 nits once × 134 klx — to zero:
the night sky was EXACTLY 0 below a −12° sun (one fp16 quantum was the only value left at −12°: 0.00733 nits computed,
0.00729 measured). Linear filtering of RGBA32F is optional in Vulkan: it is queried (storage + sampled + linear filter,
optimal tiling); a device without it falls back to RGBA16F with a warning. Cost (sky-lab, RTX 3070 Ti, 5 windows each):
`SkyAtmosphere/SkyView` 0.009 → 0.011 ms; `ScenePass` and the frame within the noise (medians 0.139 → 0.133, 2.37 →
2.24 ms). Memory: < 1 MB for the three. **Three OS** (2026-10-25): RGBA32F stored and filtered everywhere tried — RTX
3070 Ti (Linux), Apple M6 (MoltenVK), RTX 3500 Ada and the Intel Raptor Lake iGPU (Windows); no fallback. SkyView: M6
0.014 → 0.021 ms, RTX 3500 Ada 0.009 → 0.011 ms, Intel iGPU 0.115–0.164 ms (its frame ~21 ms). The night table below
matches on the three OS and both Windows GPUs within 0.3 %.

The sky-view pass integrates WITH the ground (unlike Hillaire's): below the horizon the planet's surface shows, lit by
the star through the air, instead of a black void under the scene's own small floor.

### The pieces
| Piece | File | Role |
|---|---|---|
| `AtmosphereParameters` | `Graphics/SkyAtmosphere.hpp` | the planet: radii, the three layers, Mie g **per channel**, the ground albedo; `earth()`, `mars()`, `preset(name)` |
| `SkyAtmosphere` | `Graphics/SkyAtmosphere.{hpp,cpp}` | the tables and their three compute passes; `createOnHardware()` (idempotent, mutexed) bakes transmittance + multiple scattering with a blocking one-time submission; `recordFrame()` records the sky-view pass |
| `StorageTexture2D` | `Graphics/StorageTexture2D.{hpp,cpp}` | a storage + sampled 2D table (linear, clamped) |
| `AtmosphereGLSL` | `Graphics/Effects/Shared/AtmosphereGLSL.hpp` | ONE source for the block and the functions both sides read: raw GLSL for the compute passes, Saphir declarations for the material |
| `Material::SkyAtmosphereResource` | `Graphics/Material/SkyAtmosphereResource.{hpp,cpp}` | unlit, opaque; reads the sky-view, transmittance and frame textures; the star's disc; the artistic grading |
| `Renderable::DynamicSkyResource` | `Graphics/Renderable/DynamicSkyResource.{hpp,cpp}` | the background: owns the atmosphere and the material, the skybox cube |
| `Renderer::renderSkyAtmosphere()` | `Graphics/Renderer.cpp` | the per-frame sky-view submission, BEFORE the shadow maps and the render-to-textures (the probes draw the sky too) |
| `SkyState` | `Scenes/SkyState.hpp` | the scene's sky for the render thread: the sun and moons (direction, illuminance above the atmosphere, unit-luminance LINEAR colour, angular diameter, phase), written by `CelestialClock::publishStateForRendering()`, read by `Scene::skyStateForRendering(readStateIndex)` |

### The frame
1. The renderer finds a `DynamicSkyResource` background, reads the scene's `SkyState` at the frame's read state
   (no clock: `SkyFrameInputs`' default afternoon sun) and the main camera's world Y (**Y = 0 is the planet's
   ground**; the floor is 10 m, Hillaire's planet-radius offset).
2. It records the sky-view pass in a command buffer of its own (one per frame in flight, from that frame's pool) and
   submits it on the graphics queue. **No semaphore**: every reader is submitted later on the same queue, and the
   pass's barriers (the previous frame's fragment reads → this frame's writes → this frame's reads) order them.
3. The material, per sky fragment: the direction is the cube's vertex position (centred on the camera, never
   rotated — the same contract as the picture skybox); `lightViewCos` from the horizontal projections of the view and
   the star; the sky-view lookup × the star's illuminance; the disc (below); then the grading.

### The star's disc
Its mean radiance is its illuminance over its solid angle, `E / (2π(1 − cos r))`, shaped by a linear limb
darkening normalised to that mean (`(1 − u(1 − μ)) / (1 − u/3)`, u = 0.6 for the Sun) and seen through the
transmittance along the view ray (it reddens and dims on the horizon by itself). Antialiased over one pixel.
⚠️ **Capped at 32 768 in the PRE-EXPOSED colour** (`StarDiscCeiling`): ~1.6e9 nits at noon is beyond fp16 even × E_pre
on a world cache (S = 2^-4). The cap reads `ubView.preExposure`, so every view (main, cubemap probe) is covered; the
disc saturates, it never dims.

### The light: the sun through the same atmosphere (lot 2, 2026-10-25)
With a dynamic sky, `CelestialClock` lights its sun and moons from the sky's own atmosphere instead of its empirical
model (Kasten & Young air mass, 5800 → 2000 K), so the light on the ground and the drawn disc agree. Owner decisions
(2026-10-24):
1. **A CPU twin** of the transmittance table, `AtmosphereParameters::transmittanceToTop(height, zenithCos)`: the
   table's own march sample for sample (40 uniform samples, Hillaire's 0.3 offset, `atmSampleMedium()`'s medium, the
   clamped scale heights of `packParameters()`), in double, every logic cycle. No GPU readback: deterministic, no
   frame latency, no asynchronous job. Unreal does the same on the CPU (`FAtmosphereSetup::GetTransmittanceAtGroundLevel`).
2. **The observer is the ground** (10 m, the sky-view's own floor), never the camera.
3. **The star above the atmosphere is physical**: `SunPhotometry::unitDistanceIlluminance` = **133 800 lx** at 1 AU
   (Darula, Kittler & Gueymard, *Solar Energy* 79(5), 2005, the engineering value; the IES Lighting Handbook 10th ed.
   gives 133.1 klx) over the square of the planet's distance (`Base::Math::Astronomy::starDistance()`,
   `Planet::semiMajorAxis` with the orbit's eccentricity: Earth 0.983–1.017 AU, ±3.4 % in light).
4. **The clock finds the atmosphere through the scene's background** (`CelestialClock::followAtmosphere()`, each logic
   cycle, running or not): a loaded `DynamicSkyResource` → its `AtmosphereParameters`; otherwise the empirical model. **The
   planets are checked** (owner, 2026-10-25): `AtmosphereParameters::planet` (the preset's name, "Earth" / "Mars")
   against the clock's `Planet::name` (emeraude-base); both named and different → a warning (the star's course and the
   light would not match the sky). An empty name claims nothing (`CelestialClock::simpleCourse()`). Proven with sky-lab
   option 6.
5. **The star's colour is LINEAR**: `linearColorFromTemperature(5772 K)` (IAU 2015 B3) at unit luminance, in the
   `SkyState` and in the light. Lot 1 used the sRGB-encoded `colorFromTemperature()` raw.

The light: `E_toa × Y(star ⊙ T_disc)` lux, its colour `star ⊙ T_disc` scaled to a largest component of 1.
`AtmosphereParameters::discTransmittance()` averages the transmittance over the disc in 16 horizontal strips weighted
by their chords, the strips under the ground's horizon counting zero: the light fades as the disc sets (the
penumbra) and goes out once the ground hides it, at −(horizon dip + radius) ≈ −0.37° for the Sun at 10 m. The moons take
the same change (full-moon illuminance × phase × `Y(moon ⊙ T_disc)`); a moon lights while the sun's light is off.
`CelestialClock.getState()` says `"model": "atmosphere"` and gives `transmittance`, `color`, `topOfAtmosphereLux`
and the planet's `starDistanceAU`.

| Earth, 45° N, sky-lab (2026-10-25, Linux) | Sun elevation | Light (lux) | Colour (linear, max 1) |
|---|---|---|---|
| June solstice | 60° | 113 690 | (1, 0.808, 0.668) |
| March equinox (E_toa 134 870 lx, 0.996 AU) | 45° | 114 990 | (1, 0.793, 0.637) |
| | 30° | 107 800 | (1, 0.761, 0.574) |
| | 10° | 73 388 | (1, 0.590, 0.296) |
| | 4° | 38 710 | (1, 0.378, 0.082) |
| | 2° | 21 565 | (1, 0.247, 0.021) |
| | 0° | 4 058 | (1, 0.094, 0.0008) |
| | −0.2° | 1 316 | penumbra |
| | −0.4° | 0, off | — |

**The light agrees with the drawn disc** (disc minus the surrounding sky, post-processor off, linear PNG): G/R and
B/R at 10° 0.588 / 0.295 against 0.590 / 0.296; at 4° 0.375 / 0.079 against 0.378 / 0.082; at 2° 0.253 against 0.247
(the disc's centre against the disc's average, and 8-bit levels). Mars (sky-lab option 0 = 1, 1.39 AU): 69 080 lx above
the atmosphere, 32 320 lx at 30°, 8 905 lx at 10°; the art preset's dust lets more blue through than red, so its
direct sunset light turns blue (0.59, 0.91, 1) — data, tunable. 0 VUID in the three runs. **Validated on the three
OS** (2026-10-25, engine 291efb25): macOS M6 (MoltenVK) and Windows (MSVC, RTX 3500 Ada) give the same sweep (to 3e-5),
the same disc-vs-light colours (within 8-bit levels) and the same zenith, L = 1 771 cd/m².

The Earth's clean Hillaire atmosphere (aerosol depth ~0.005) is far brighter near the horizon than the empirical model
(73 klx against ~36 klx at 10°): a hazier day is the manifest's `MieScattering` (sky-lab option 5).

### The environment: the sky's own IBL, amortised (lot 3, 2026-10-25)
The ambient, the reflections and the traced lanes' misses read the scene's three IBL slots (environment, irradiance,
GGX-prefiltered; `docs/reflection-pipeline.md` § 2.2). A dynamic sky fills them itself — `Graphics::SkyEnvironment`,
owned by the `SkyAtmosphere` — so a shadowed object turns blue at noon, orange at sunset and dark at night by itself.
Owner decisions (2026-10-25):
1. **A compute capture** of THIS frame's sky-view table into a 64² cubemap with its full mip chain
   (`IBLTexture::Role::EnvironmentCapture`), recorded in the sky's command buffer right after the sky-view pass. The
   lookup and the grading are the material's own: `AtmosphereGLSL::SkyViewLookup` and `AtmosphereGLSL::Grade` (one
   source, the material moved onto them), the face convention is `IBLBaker::FaceDirectionGLSL`. The star's DISC is not
   captured (it lights through its directional light, the StarMask rule); its Mie halo is.
2. **Absolute radiance**: the capture is in nits (the star's illuminance applied, so the night does not underflow in
   half floats), and the dynamic sky declares a luminance of **1** (`DynamicSkyResource::AbsoluteLuminance`; a manifest
   "Luminance" is ignored with a warning): every consumer that scales a cubemap sample by the environment luminance
   stays right.
3. **Amortised, one stage per frame**: the capture and its mips, then the irradiance, then one prefilter mip per frame
   (`IBLBaker::recordIrradiance()` / `recordPrefilterMip()`, the same recordings the blocking picture-sky bake now uses
   — one code path), then publication: 8 frames a cycle. A cycle starts when the star moved more than **0.25°**
   (`SkyEnvironment::TriggerAngle`), its illuminance or the grading changed by 1 %, or nothing was baked yet.
4. **A ring of MaxFramesInFlight + 1 = 6 sets**, allocated at the first stage (≈ 6 MB): a cycle writes the set
   published five cycles ago, which no frame in flight can still read. The picture skies' bake uses the same ring
   (`Scene::IBLRingSize`; it was a 2-deep ping-pong while up to 5 frames are in flight, and its blocking bake waited on a
   queue that may not be the renderer's — `getGraphicsQueue(High)` rotates — so a re-bake could overwrite textures a
   frame still read; fixed 2026-10-25, proven by four skies swapped in a row, 0 SYNC-HAZARD).
5. **Publication**: the completed set is released after the sky's command buffer is SUBMITTED
   (`SkyEnvironment::onSubmitted()`; a failed submission drops the cycle and starts it over); the scene takes it on the
   logic thread (`Scene::updateDynamicSkyIBL()`) and publishes the three slots, the irradiance under the applyAmbient
   contract. The once-probes are re-signalled when the sun moved more than **2°** since they last were
   (`Scene::ProbeResignalAngle`).
6. **The environment slot (0) gets the capture too**: refraction, the RTGI misses and the irradiance probe volume see
   the sky.

A scene takes its ambient from the sky with `applyBackgroundLighting({.applyAmbient = true, .applyStars = false})`
(sky-lab does; the clock lights the sun). The CPU photometry of a picture sky (`hemisphereIlluminanceFactor()`,
`averageColor()`) is moot there: with the IBL ambient on, the scalar ambient is zero (the anti-double-count rule).

| Measured (sky-lab, Linux, RTX 3070 Ti) | Value |
|---|---|
| `SkyAtmosphere/Environment` (GPU profiler, the clock at ×600: cycles back to back) | avg 0.003–0.015 ms, **max 0.10 ms** per frame; Apple M6 (MoltenVK): avg 0.031–0.034, max 0.14–0.42; RTX 3500 Ada: avg 0.008, max 0.08; ⚠️ Intel Raptor Lake iGPU: avg 0.19, **max 2.1 ms** (its heaviest stage in one frame — a split of the prefilter's mip 0 / the irradiance over two frames is the lever if it matters) |
| Lag behind the sky | one cycle: 8 frames + one logic tick (~0.15 s at 60 FPS), every 0.25° of sun (≈ 1 min in real time). ⚠️ What follows the IBL can be slower: with SSGI owning the indirect diffuse (the ScreenSpace lane), the floor converges with an e-fold of ~1.3 s (macOS-PA, M6; IBL-only: < 0.5 s), and TAA alone smears a sudden global change over ~1 s |
| The floor (albedo 0.5) under the sky alone, by time of day | noon neutral-warm, sunset orange, after sunset the twilight's purple, night dark (the exposure floor) — Mars: butterscotch at sunset |
| Validation (Earth, Mars, four picture skies swapped in a row on `reflexion-debug`) | 0 VUID, 0 SYNC-HAZARD (synchronization validation requested with `VK_KHRONOS_VALIDATION_VALIDATE_SYNC=true`) |

### The manifest (Backgrounds store)
`"Planet"` (a preset name, default `"Earth"`), `"Atmosphere"` (inline overrides: `GroundRadius`, `TopRadius`,
`RayleighScaleHeight`, `MieScaleHeight`, `AbsorptionLayerWidth` in km; `RayleighScattering`, `MieScattering`,
`MieAbsorption`, `AbsorptionExtinction` per km as `[r, g, b]`; `MiePhaseG` as `[r, g, b]` or one number;
`GroundAlbedo`), the artistic keys (`Tint`, `Intensity`, `Saturation`, `StarDisc`, `LimbDarkening`) and the
background photometry (`AbstractBackground::parsePhotometry()`). From code: `DynamicSkyResource::load(parameters)`.

### The presets
- **Earth**: Hillaire's / Bruneton's constants (Rayleigh 5.802, 13.558, 33.1 × 1e-3 /km, H 8 km; Mie 3.996e-3
  scattering + 0.444e-3 absorption, H 1.2 km, g 0.8; ozone 0.650, 1.881, 0.085 × 1e-3 over a 25 km tent; albedo 0.3).
- **Mars** (an ART approximation, the test of the extraterrestrial decision): a thin CO2 Rayleigh layer (~1.5 % of
  Earth's), a dust of optical depth ~0.5 that absorbs the blue (butterscotch day), H 11.1 km, and a Mie anisotropy
  **per channel** (0.50, 0.66, 0.88): the forward peak narrows in the blue (diffraction by ~1.5 µm grains), which
  makes the blue aureole of its sunsets. A single scalar g (Hillaire's, UE's) cannot.

### Console (`Core.SceneManagerService.*`)
`getDynamicSky()` (JSON: planet, radii, tint, intensity, saturation, star disc, limb darkening),
`setDynamicSkyGrading(r, g, b, intensity)`, `setDynamicSkySaturation(s)`, `setDynamicSkyStar(enabled, u)`. The sun's
elevation and illuminance: `CelestialClock.getState(...)`.

### Measuring a sky in nits (the protocol)
There is no HDR readback; this is the absolute reading:
1. `Core.RendererService.setPostProcessorEnabled(0)` — the direct swap-chain path: no exposure, no tone mapping,
   **E_pre = 1** (`latchPreExposure()`: no tone mapper; the override does not apply there).
2. `setDynamicSkyGrading(1, 1, 1, k)` with k = 2^-14 (or 2^-13): the sky is scaled into [0, 1].
3. Screenshot; ⚠️ **the PNG of the direct path is LINEAR** (a UNORM swap-chain: doubling k doubles the bytes exactly
   — checked), so nits = byte / 255 / k, no sRGB decoding.

### Measurements (2026-10-24, Linux, RTX 3070 Ti, 1920×1080)
| What | Value |
|---|---|
| Sky-view pass (`SkyAtmosphere/SkyView`, GPU profiler) | **0.010 ms** (max 0.040) |
| Earth zenith, sun 45°, 124 600 lx above the atmosphere, sRGB-encoded star colour (lot 1) | RGB (835, 1677, 3765) nits, L ≈ 1 650 cd/m² |
| Earth zenith, sun 45°, **134 870 lx** above the atmosphere, LINEAR 5772 K (lot 2, 2026-10-25) | RGB (964, 1799, 3870) nits, **L ≈ 1 771 cd/m²** (a very clear sky by Kittler's zenith formula: ~2 000 at turbidity 2.5); 2^-13 doubles the bytes |
| Earth dusk, 45° N equinox (sun 12.7°, 5.1°, 0.1°, −3.0°, −7.5°) | halo → yellowing → orange horizon under a purple vault → pink-violet → deep blue with an orange band (frames in the `sky-lab` notes) |
| Mars | butterscotch day, a white-to-blue aureole core at sunset (tone-mapped: the core saturates) |

### The night (2026-10-25, RGBA32F tables)
Earth, 45° N, new moon, zenith (post-processor off, the grading raised until the bytes read; the doc's nits protocol):

| Sun | 0° | −3° | −6° | −9° | −12° | −15° | −18° | −21° | −30° |
|---|---|---|---|---|---|---|---|---|---|
| Zenith (cd/m²) | 187 | 31 | 1.04 | 0.041 | 0.0024 | 8.7e-5 | 7.6e-6 | 3.7e-7 | 0 |

Civil twilight ends at ~1 cd/m², as measured skies do. Below −18° the zenith column is in the planet's shadow up to
327 km (the atmosphere is 100 km thick): only multiple scattering from the sunlit horizon remains, and at −30° nothing
— a PHYSICAL zero of this model. A real moonless night floor (~2e-4 cd/m²) is airglow, zodiacal light and starlight,
none of which the sun's scattering gives: item `dynamic-sky-stars-and-milky-way` § "The night sky's own glow".

### ⚠️ Traps
- A background WITHOUT an environment cubemap is legitimate (`AbstractBackground::environmentCubemap()` answers
  nullptr): `Scene::getRenderableInstanceReadyForRendering()` dereferenced it on the first frame (fixed 2026-10-24,
  the scene keeps the cubemap it has). The dynamic sky publishes its own environment instead (§ "The environment").
- `fwidth()` in non-uniform control flow is undefined: the disc's antialiasing derivative is taken BEFORE the
  ground / disc branch.
- The sky-view table depends on the camera altitude: the main camera's world Y is fed as the height above the
  ground. A flight scene whose ground is not at Y = 0 must say so (not wired yet).
- The descriptor writes of the tables state `SHADER_READ_ONLY_OPTIMAL` explicitly: the atmosphere moves them with its
  own barriers, the image's tracked layout does not follow.
- ⚠️ **A barrier recorded in a command buffer does not update `Image::currentImageLayout()`**: the bindless table
  describes a texture with its TRACKED layout. The amortised bake sets SHADER_READ_ONLY_OPTIMAL on the three textures
  when a set completes (as `IBLBaker::bakeEnvironment()` does after its wait); without it, 10 ×
  `VUID-VkWriteDescriptorSet-descriptorType-04150` (layout UNDEFINED) at the first publication.
- ⚠️ The IBL textures are read by fragment, compute AND ray-tracing stages (RTGI misses): the bake's discard and
  publish barriers use `ALL_COMMANDS` (`IBLBaker::CubeBarrier`).
- ⚠️ **Two sources for one integral**: `AtmosphereParameters::transmittanceToTop()` (C++) must follow
  `atmSampleMedium()` and the table's march (`AtmosphereGLSL.hpp`, `SkyAtmosphere.cpp` pass 1) — a change to one is a
  change to the other, re-measured (the light's colour against the disc, § "The light").
- Per-channel g is not in Hillaire's paper: Earth sets one value three times; the integration is unchanged
  (`atmMiePhase()` returns a vec3).
