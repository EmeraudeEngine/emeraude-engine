## 42. The physical sky — a planet's atmosphere drawn as the background (Oct 2026)

> Owner decisions (2026-10-24): the sky is PHYSICAL and must render extraterrestrial skies (the planet is data);
> it is drawn as a **background material** on the skybox cube; the **CelestialClock** writes the scene's sky state;
> **one** scattering sun; an AI-owned lab demo (`sky-lab`, projet-alpha). Lot 1 of the dynamic sky; lot 2, the sun's
> light from the same atmosphere, is § "The light"; lot 3, the sky's own IBL, is § "The environment" (2026-10-25); lot 4a,
> the distance haze of the same atmosphere, is § "The aerial perspective" (2026-10-25); the moons' discs and their light
> are § "The moons" (2026-10-25); the next lots are the engine items `dynamic-sky-aerial-perspective` (4b: the clouds),
> `dynamic-sky-stars-and-milky-way`, `dynamic-sky-moonlit-sky-glow` and `dynamic-sky-moon-relief`.

### The model
S. Hillaire, *A Scalable and Production Ready Sky and Atmosphere Rendering Technique*, EGSR 2020 (the model of UE's
`SkyAtmosphere`), after his MIT reference implementation; the transmittance parametrisation is Bruneton & Neyret's
(EGSR 2008). A Rayleigh layer, a Mie layer and an absorbing layer (Earth's ozone, a tent profile), all exponential
or piecewise-linear in height. **Units: kilometres, per kilometre; the planet's centre is the origin.**

⚠️ **Every march samples the MIDPOINT of its steps** (owner decision 2026-10-25). The reference marches at 0.3 of a
step (`SampleSegmentT`), and its fixed-count branch measures each step from the previous sample, so the first sample
is worth only 0.3 of a step and the march covers (1 − 0.7/N) of the ray. Against an exact integration (20 000 steps)
of the transmittance to the top from 10 m:
- the reference fixed march (the transmittance table): optical depth **−6 to −15 %**;
- a full step at 0.3: **+2 to +7 %**;
- the midpoint: **−1.7 to 0 %**.

The variable march (the sky-view) at 0.3 reads +6 to +9 %; at the midpoint, −2 to 0 %. The first sample decides
because it dominates the thin Mie layer (1.2 km) at the ground. Found by measuring the aerial perspective against a
CPU integration: its optical depth was ~4.5 % short at 1–9 km. Fixed in `atmIntegrate()` and in the CPU twin
together (`transmittanceToTop()`, still the table's own march), with lot 2's numbers below re-measured. The reference
over-lit the low sun: the light went −1.8 % at 60°, −3 % at 30°, −7 % at 10°, −13 % at 2° and −15 % at the horizon,
and the noon zenith −8 %.

| Table | Size, format | Built | Holds |
|---|---|---|---|
| Transmittance | 256×64 RGBA32F | once, at creation | transmittance to the top, per (height, zenith cosine) |
| Multiple scattering | 32×32 RGBA32F | once, at creation | Hillaire's isotropic second-order transfer f_ms, summed as a series |
| Sky-view | 192×108 RGBA32F | **every frame** | in-scattered radiance **per lux** of the star, around the camera: x the azimuth from the star, y the zenith (non-linear near the horizon) |
| Frame | 16×1 RGBA32F | every frame | texel 0: unit vector toward the star + cos of its angular radius; texel 1: its illuminance above the atmosphere × colour (lux) + the camera's distance to the centre (km); texel 2: the moon count; texels 4 + 3i to 6 + 3i: moon i (§ "The moons") |

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
| `Material::SkyAtmosphereResource` | `Graphics/Material/SkyAtmosphereResource.{hpp,cpp}` | unlit, opaque; reads the sky-view, transmittance and frame textures; the star's disc; the moons' discs and their surface maps; the artistic grading |
| `MoonSurfacePhotometry` | `Graphics/MoonSurfacePhotometry.{hpp,cpp}` | a moon map's near-side mean and its brightness table by sub-solar point, measured once from its pixels (§ "The moons") |
| `Renderable::DynamicSkyResource` | `Graphics/Renderable/DynamicSkyResource.{hpp,cpp}` | the background: owns the atmosphere and the material, the skybox cube |
| `Renderer::renderSkyAtmosphere()` | `Graphics/Renderer.cpp` | the per-frame sky-view submission, BEFORE the shadow maps and the render-to-textures (the probes draw the sky too) |
| `SkyState` | `Scenes/SkyState.hpp` | the scene's sky for the render thread: the sun and moons (direction, illuminance above the atmosphere, unit-luminance LINEAR colour, angular diameter, phase) and the equatorial frame the stars are placed by, written by `CelestialClock::publishStateForRendering()`, read by `Scene::skyStateForRendering(readStateIndex)` |

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
the same change (full-moon illuminance × their brightness × `Y(moon ⊙ T_disc)`, § "The moons"); a moon lights while
the sun's light is off.
`CelestialClock.getState()` says `"model": "atmosphere"` and gives `transmittance`, `color`, `topOfAtmosphereLux`
and the planet's `starDistanceAU`.

| Earth, 45° N, sky-lab (re-measured 2026-10-25 with the midpoint march, Linux) | Sun elevation | Light (lux) | Colour (linear, max 1) |
|---|---|---|---|
| June solstice | 60° | 111 670 | (1, 0.800, 0.646) |
| March equinox (E_toa 134 870 lx, 0.996 AU) | 45° | 112 520 | (1, 0.784, 0.612) |
| | 30° | 104 600 | (1, 0.748, 0.542) |
| | 10° | 68 307 | (1, 0.566, 0.257) |
| | 4° | 34 483 | (1, 0.353, 0.064) |
| | 2° | 18 748 | (1, 0.227, 0.016) |
| | 0° | 3 433 | (1, 0.084, 0.001) |
| | −0.2° | 1 112 | penumbra |
| | −0.4° | 0, off | — |

(The reference march's values, lot 2: 113 690, 114 990, 107 800, 73 388, 38 710, 21 565, 4 058, 1 316 lx.)

**The light agrees with the drawn disc.** Protocol: post-processor off, linear PNG, a 300 mm lens AIMED at the sun
(`Act.lookAt` along `getState`'s sun direction), the disc's centre (r < 15 px) minus an annulus clear of it (110–140
px), k brought until the core reads 200–245. G/R and B/R, disc against light:

| Elevation | G/R disc | G/R light | B/R disc | B/R light |
|---|---|---|---|---|
| 30° | 0.748 | 0.748 | 0.546 | 0.542 |
| 10° | 0.566 | 0.566 | 0.256 | 0.257 |
| 4° | 0.357 | 0.353 | 0.066 | 0.064 |
| 2° | 0.227 | 0.227 | 0.017 | 0.016 |

The B/R differences are 8-bit levels. ⚠️ Two traps of the default lens. The disc is ~11 px across, so a ring at 6–9
px sits on its antialiased edge. And the disc is CAPPED (`StarDiscCeiling`), so the horizon glow can outshine it and
a global maximum picks the glow.

Mars (sky-lab option 0 = 1, 1.39 AU): 69 080 lx above the atmosphere, 30 220 lx at 30°, 7 606 lx at 10° (reference
march: 32 320 and 8 905). The art preset's dust lets more blue through than red, so its direct sunset light turns
blue — data, tunable. 0 VUID. **Validated on the three OS**, with the reference march (engine 291efb25) and again with
the midpoint march (engine 800aacbb, 2026-10-25):
- macOS M6 (MoltenVK), Windows RTX 3500 Ada and the Intel iGPU give the same sweep (Intel 2 lx under at 10°);
- the same disc-vs-light colours, within 0.003;
- the same noon zenith, L 1 626–1 635;
- the same night at −12°, 0.00208 cd/m²;
- 0 VUID.
The M6's twilight zenith reads ~1 % under Linux's, whose readings sat at 40–60 bytes: quantisation.

The Earth's clean Hillaire atmosphere (aerosol depth ~0.005) is far brighter near the horizon than the empirical model
(68 klx against ~36 klx at 10°): a hazier day is the manifest's `MieScattering` (sky-lab option 5).

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

### The aerial perspective (lot 4a, 2026-10-25)
The distance haze of the SAME atmosphere: a mountain turns blue by day and pink at sunset, and it agrees with the sky
behind it. S. Hillaire, EGSR 2020 § 5.4, after his reference code (`RenderSkyRayMarching.hlsl`, MIT). Owner
decisions (2026-10-25):
1. Under a dynamic sky the aerial perspective owns the distance haze; `AtmosphericFog` keeps the LOCAL medium.
2. RGB transmittance.
3. Its own chain slot, `EffectSlot::AerialPerspective`, after the geometry (translucents included, at the depth they
   wrote) and before `Clouds` and `Fog`.
4. Lot 4a = the volume, its application, sky-lab's distant targets and the horizon seam; lot 4b = the clouds lit by
   the atmosphere (item `dynamic-sky-aerial-perspective`).

**The volume** (`SkyAtmosphere::recordAerialPerspective()`, in the sky's command buffer after the sky-view, every
frame):
- 32 × 32 froxels over the screen and 32 slices along the view ray, in Hillaire's squared distribution: slice s ends
  at 128 km × ((s + 0.5) / 32)², 4 km per linear slice unit.
- Each froxel marches `atmIntegrate()` (the sky-view's integrator, Rayleigh + Mie phases, the multiple scattering)
  from the camera to its slice, with 2 × (s + 1) samples.
- Two 3D storage textures (`StorageTexture3D`). The in-scattering is ABSOLUTE radiance in nits, the star's
  illuminance and the sky's grading applied like the drawn sky's, stored in the table format (RGBA32F: a night's haze
  would lose its precision in half floats). The RGB transmittance is RGBA16F.
- The froxel rays come from `invRelativeViewProj = (projection × view rotation)⁻¹` of the read state, the same
  matrix the application reconstructs with.
- The volume is left in SHADER_READ_ONLY_OPTIMAL; its barrier covers the fragment and compute stages, and the
  scene's later submission on the same queue reads it.

**The application** (`Effects/Atmosphere/AerialPerspective`, one full-res RGBA16F pass): `scene · T + L × E_pre` per
geometry pixel; the sky pixels (depth 1) are left alone, since the drawn sky already is the whole atmosphere.
- The distance comes from the matrix reconstruction (the whole chain's since 2026-10-25, when its frozen clip planes
  were retired: [`13-12 § 41`](13-12-post-processing-effects/41-the-view-reconstruction-one-matrix-path-from-depth-to-position.md)).
- The lookup is w = √(distance / 128 km). Under half a slice (2 km), the first sampled slice fades linearly to 0 at
  the camera, as in Hillaire's.
- The material's fog response (alpha's high nibble) scales it: a surface immune to the fog is immune to the haze.

**Lifetime and filing.**
- `Renderer::frameSkyAtmosphere()` names the atmosphere THIS frame submitted; its in-flight slot keeps it alive. The
  chain's `FrameContext::atmosphere` reads it, and a null value means a pass-through.
- `PostProcessStack::syncSceneEffects()` files the effect the first frame a physical sky is submitted, after the
  clouds' model.
- `Core/Graphics/PostProcessing/AerialPerspective/Enabled` (true) declines it.

**Measured** (sky-lab option 7: a white (0.8) and a black (0.03) matte cube at 1, 3, 6 and 9 km to the west, 2.5 %
of their distance wide; Linux, RTX 3070 Ti, 1920×1080).

Protocol:
- 09:00, sun behind the camera; a 150 mm lens aimed at each pair; bloom off.
- AP on and off (`PostProcess.disable("AerialPerspective")` / `select`), three shutters each.
- Linear values recovered through the chain by inverting the tone mapper. ACES (Narkowicz's fit, the shader's `acesFilmic`) then gamma 2.2 fit the
  ×2 shutter series to 3e-5.
- T = (white − black) / (white₀ − black₀) per channel, against a CPU integration of the same medium along the same
  ray:

| Distance | T measured (R, G, B) | T CPU | In-scattering / white₀ |
|---|---|---|---|
| 1 km | 0.996, 0.991, 0.962 | 0.990, 0.982, 0.963 | 0.004, 0.009, 0.025 |
| 3 km | 0.980, 0.957, 0.902 | 0.970, 0.948, 0.894 | 0.012, 0.027, 0.071 |
| 6 km | 0.947, 0.911, 0.810 | 0.941, 0.899, 0.800 | 0.025, 0.055, 0.137 |
| 9 km | 0.917, 0.861, 0.722 | 0.914, 0.853, 0.716 | 0.039, 0.084, 0.202 |

- Within ~1 % from 3 km on. Under 2 km, Hillaire's linear fade stands in for the first slices.
- The first measurement read ~4.5 % less optical depth than the CPU. That is how the reference march's bias was
  found (§ "The model").
- A temporary build that drew the volume's own −ln T at the pixels confirmed it slice by slice.

**The horizon seam.** The haze of a surface at distance d is, by construction, what the sky-view integrates to a
ground hit at the same distance: the same `atmIntegrate()`, medium, star and grading. What stays visible is physical:
a black cube at 9 km reaches 23–30 % of the horizon sky just above it, because that sky holds hundreds of kilometres
of air. A scene's far edge stands out against the sky as a real ridge would on a clear Hillaire day. Hazier days are
the manifest's Mie scattering.

**Cost** (GPU profiler, 5 windows): `SkyAtmosphere/AerialPerspective` (the volume) 0.030–0.032 ms;
`AerialPerspectiveEffect` 0.067 ms at 1920×1080. 0 VUID, 0 SYNC-HAZARD (sky-lab, validation and synchronization
validation, the layer proven loaded).

### The moons (2026-10-25)
Every moon of the clock is DRAWN by the sky material with the reflectance its light is the integral of: the phase, the
terminator, the maria and the earthshine come out of one model, and the light on the ground is the disc in the sky.
Owner decisions (2026-10-25):
1. **An analytic sphere** in the sky material (M1), up to four moons (`SkyAtmosphereResource::MaxMoons`, unrolled).
2. **One reflectance for the disc and its light** (M2): Hapke's lunar BRDF as H. W. Jensen et al. give it, *A
   Physically-Based Night Sky Model*, SIGGRAPH 2001, eq. 2–4 — f = 2 / 3π × B(φ, g) × S(φ) × μ0 / (μ0 + μ), the
   retrodirective function B (the opposition surge, density g = 0.6) and the particles' law S (forward scattering
   t = 0.1). One source in emeraude-base (`Math::Astronomy::lunarReflectance()`), mirrored in GLSL
   (`AtmosphereGLSL::LunarReflectance`).
3. **The surface is a map** (M3), equirectangular, 0° longitude at its centre, east to the right, north up, sRGB: the
   manifest's `"Moons": [{"Surface": "<store texture>"}]`, indexed like the clock's moons. projet-alpha ships NASA's
   SVS CGI Moon Kit colour map (LRO LROC; credit "NASA's Scientific Visualization Studio", svs.gsfc.nasa.gov/4720) at
   2K, 4K and 8K (`Astronomy/Moon2K`, `Moon4K`, `Moon8K`, PNG, resampled in linear light from a 14K source). Without a
   map the albedo is uniform.
4. **The earthshine is physical** (M4), an artistic gain on top (`CelestialClock::Moon::earthshineGain`, 1).
5. **The light follows the TEXTURED disc** (`MoonSurfacePhotometry`): with a uniform albedo the light and the disc
   parted by −10 % to +14 % across the month.

**The disc** (per sky fragment, per moon):
- A sphere of the moon's angular radius at a unit distance; the view ray's nearest hit gives the normal, and the edge is
  antialiased over one pixel like the star's.
- Its frame faces the observer always (the face toward the planet, no libration): toward = −the moon's direction, north
  = the orbit's pole (`Math::Astronomy::moonOrbitPole()`) made perpendicular, east = north × toward. Map coordinates
  u = 0.5 + atan2(n·east, n·toward) / 2π, v = 0.5 − asin(n·north) / π, read with `textureGrad()`.
- Radiance = `E_full × colour / (f(1, 1, 0) × Ω)` × [f(μ0, μ, φ) + e × f(1, 1, 0)] × A(texel) / Ā × T(view):
  - `E_full` is the moon's illuminance when full, above the atmosphere (0.3 lx, `Moon::fullIlluminance`), Ω the disc's
    solid angle;
  - a full disc's reflectance is UNIFORM (μ0 = μ: no limb darkening), so the disc integrated at full is `E_full`
    exactly;
  - `e` is the earthshine, the planet's light from the observer's side (zero phase);
  - A / Ā is the map over its NEAR-SIDE mean, cos²(latitude) × cos(longitude)-weighted, in linear light (the projected
    area a full moon shows);
  - T is the transmittance along the view ray.
- The moon covers the star's disc (an eclipse is drawn), and the sky's scattering lies in front of it.
- The moons reach the frame texture through a uniform block (`SkyAtmosphere::m_moonBuffer`, a header + three vec4 per
  moon), written in the frame's command stream (`vkCmdUpdateBuffer`, the previous frame's reads barred first) and
  copied by the sky-view pass. The clock's state reaches it through `SkyState` (`pole`, `fullIlluminance`,
  `earthshine`).

**The light** = `E_full × [P(φ) × S(λs, βs) + e] × Y(colour ⊙ T_disc)` (`CelestialClock::moonBrightness()`):
- P is the lunar phase law, f integrated over a uniform disc: `B(φ) S(φ) / B(0) S(0) × Φ_LS(φ)`, Φ_LS the exact
  Lommel-Seeliger law (Jensen eq. 5) — no quadrature (`Math::Astronomy::lunarPhaseLaw()`). Quarter: **0.077**, the
  Moon measures 0.083 (V −10.0 against −12.7 full); the former Lambert sphere gave 0.32.
- e = `Moon::earthshine` × Φ_LS(π − φ) × the gain: the planet's geometric albedo × (its radius / the moon's distance)²,
  Earth's 0.434 × (6 371 / 384 400)² ≈ 1.19e-4 (`Math::Astronomy::EarthshineRatio`), ~16 lx on the Moon under a
  134 klx Sun when the Earth is full.
- S is the textured disc over the uniform one at the sub-solar point (λs, βs): `MoonSurfacePhotometry`, a 73 × 37 table
  (5° steps) measured ONCE from the map's pixels by the sky material before its creation (a loader thread: the
  material's creation takes 54 ms with the 2K map, 148 ms with the 8K, 29–36 ms without a map; the sky is ready
  ~1.1 s after the scene starts either way). B and S do not depend on the surface
  element: they cancel in the ratio, only the Lommel-Seeliger kernel remains (`Math::Astronomy::lommelSeeligerKernel()`).
  1 at full, 1 without a map. The clock takes the table with the atmosphere (`followAtmosphere()`, the sky resource
  LOADED: its status publishes the immutable object), computes the sub-solar point in the shader's frame
  (`MoonSurfacePhotometry::subSolarPoint()`) and gives it in `getState()` as `surfaceBrightness`.

**Not in the environment.** The disc is the visible face of its moon's light — the light-face contract of
[`13-12 § 42`](13-12-post-processing-effects/42-emission-in-the-indirect-diffuse-a-light-s-face-never-re-emits.md):
its light reaches the scene through its directional light, and the IBL capture leaves it out like the star's disc. A
moonlit ambient's blue-grey is the moonlit SKY — the atmosphere scattering the moon's light — which the sky-view does
not integrate yet: item `dynamic-sky-moonlit-sky-glow`.

**Measured** (sky-lab, Linux, RTX 3070 Ti, 1920 × 1080, the nits protocol below; a 400 mm lens aimed at the moon; the
pixel's solid angle FROM THE LENS — vertical field 2 atan(12 / f) over 1 080 px — and the disc integrated within 1.08
of its radius; the grading raised per phase until the peak reads 160–210; validation on, 0 VUID):

| Moon (clock day) | φ | Elevation | S (textured ÷ uniform) | Disc ÷ light |
|---|---|---|---|---|
| full (1.0) | 6.3° | 41.8° | 0.995 | **1.002** |
| waning gibbous (5.2) | 56.7° | 20.2° | 0.951 | **1.000** |
| last quarter (8.2) | 93.7° | 13.0° | 0.911 | **0.996** |
| waxing crescent (18.8) | 136.6° | 27.5° | 1.145 | **1.000** |
| first quarter (22.8) | 87.9° | 65.8° | 1.092 | **1.003** |

- S is what a light with a uniform albedo missed: the disc and such a light parted by −9 % to +15 % (measured 0.905 to
  1.144 before the table, with the kit's own 2K JPEG; its S matched the PNG's within 0.005).
- `Astronomy/Moon8K`: the full disc reads 1.002 / 1.002 / 1.003 at 400 / 800 / 1 600 mm (the kit's 2K JPEG: 0.997
  at the three): the map's size and mip level do not bias it.
- Its mean is 3 133 cd/m² at 41.8° (E / Ω = 3 289).
- Mars (sky-lab option 0 = 1, no map): 0.994–1.007, S = 1.
- The month is asymmetric, as the real one: the maria lie on the western half, lit when the moon wanes. First quarter /
  last quarter = 1.21 here (0.2 mag). ROLO's photometry (Kieffer & Stone, *AJ* 129, 2005) shows the same sense: the
  heterogeneity correction is larger in the waxing phases (*Remote Sensing* 11(22) 2696, 2019); no quarter figure was
  found to compare with.
- Earthshine, waxing crescent (φ 136.6°, 27.5°; the kit's 2K JPEG): the dark side reads a median 0.31 cd/m² (p10
  0.18, p90 0.6 — maria and highlands), ~1e-4 of the lit side; its maria show.
- Cost (GPU profiler, sky-lab pitched 40°, ~70 % of the screen sky, two A/B pairs, medians of 5 windows): `ScenePass`
  0.101 ms with the Moon, 0.100 ms without (sky-lab option 8 = 0) on Linux (RTX 3070 Ti, 1920 × 1080). The Apple M6
  (2560 × 1440) measured 0.640 against 0.526 ms with the first shader (every sky pixel computed the moon's frame:
  trap below); with the analytic gradients, Linux 0.111 / 0.110 ms (on / off).

**Loading.** A moon's map is a dependency of the sky material: the sky appears when the map is loaded. On a COLD
texture cache the engine block-compresses the map first (CPU BC7): Windows-PA measured 15.6 s for the 8K on the Intel
iGPU (8192 × 4096, 14 mips), 1.8–1.9 s for the 2K; warm, 0.23 s. Items `sky-body-surfaces-non-blocking` and
`astronomy-maps-gpu-ready` (owner decision 2026-10-25: both).

**Failure paths.** A surface that is not a store texture fails the sky's load, like any malformed manifest key. A
surface without CPU pixels (a block-compressed store image) or with a black near side is traced and drawn with a
uniform albedo; its light then stays uniform too.

### The stars (lot S1, 2026-10-25: the catalogue)
The stars the sky draws as points come from a **star catalogue resource** (`Graphics::StarCatalogueResource`, data store
`StarCatalogues`, container "Star catalogue manager"), in ONE canonical schema whatever the source (owner decision
2026-10-25: a canonical format and a converter):

```json
{ "format": "EmeraudeStarCatalogue", "version": 1, "name": "...", "source": "...", "credit": "...",
  "frame": "ICRS / J2000 equatorial, epoch 2000.0", "fields": ["ra_deg", "dec_deg", "vmag", "bv", "id", "bvEstimated"],
  "optionalFields": ["bvEstimated"], "stars": [[101.287083, -16.716111, -1.46, 0.0, "HR 2491"], ...] }
```

- The resource keeps, per star, the unit vector in the J2000 equatorial frame (x toward the vernal equinox, z toward the
  north celestial pole), Johnson V and B−V; the file is a trust boundary: an entry with a missing, non-finite or
  out-of-range position, V (−27 to 30) or B−V (−0.6 to 6) is refused, counted and traced. The neutral resource is an
  empty catalogue.
- projet-alpha ships `StarCatalogues/YaleBSC5` — the Yale Bright Star Catalogue, 5th revised ed. (CDS V/50, Hoffleit &
  Warren 1991): 9 096 stars with a position, V −1.46 (Sirius) to 7.96, 310 B−V estimated from the spectral class
  (flagged), the whole set 2.0e-4 lx above the atmosphere. Converter: projet-alpha `tools/star-catalogue/`. Loaded on
  sky-lab through `loadResource(StarCatalogueResource, YaleBSC5)`: 9 096 stars, 0 refused, 0 VUID.
- ⚠️ **Licences**: ESA's Hipparcos, Tycho-2 and Gaia are CC BY-NC 3.0 IGO (non-commercial) — not used (owner decision);
  the fainter stars will come from UCAC4 (USNO, public domain). Item `dynamic-sky-stars-and-milky-way` § The plan.

### The stars (lot S2, 2026-10-25: the points)
A sky whose manifest names a catalogue (`"StarCatalogue": "YaleBSC5"`, `DynamicSkyResource::JKStarCatalogue`; a
dependency, the sky loads with it) under a celestial clock gets its stars drawn by **`Effects::Atmosphere::StarField`**,
a chain effect in **`EffectSlot::Stars`, right after the TAA** (owner decision 2026-10-25: a star is smaller than a pixel;
a temporal resolve smears a jittered point into a dim blur or rejects it as an outlier) and before the camera's optics
(the defocus, the motion blur and the glare spread a star like any point of light).

- **The frame**: the clock publishes the planet's equatorial axes in world vectors (`SkyState::equatorialFrame`:
  toward right ascension 0° and 90° on the equator, and the north celestial pole; console `CelestialClock.getState` →
  `equatorialFrame`). A catalogue direction (x, y, z) is the world vector x·[0] + y·[1] + z·[2]. The renderer records
  the catalogue and the frame with the atmosphere each frame submits (`Renderer::frameStarField()`, one slot per frame
  in flight keeps the catalogue alive); `FrameContext::stars` hands it to the chain; `PostProcessStack::syncSceneEffects()`
  files the effect the first frame it exists (`Core/Graphics/PostProcessing/Stars/Enabled = false` declines it).
- **The pass**: one instanced draw at full resolution, a quad of ±3.5σ per star (σ = 0.7 px), additive, into an RGB
  float target (`B10G11R11_UFLOAT` when the device blends into it — 30 MB at 3840×1990 —, else `RGBA16F`). The vertex
  stage reads the star buffer (an SSBO of `{direction, E·colour}`, built once per catalogue, retired through the deferred
  destructor), projects the direction with the **unjittered** projection × the view rotation, and drops the star
  behind the camera, off the screen, below the horizon (the ray meets the planet: the sky's own test) or behind the
  star's or a moon's disc (the sky's frame texture, read in the VERTEX stage: its barriers now include that stage).
- **The photometry**: E = E₀ 10^(−0.4 V), **E₀ = 2.69e-6 lx** — the engine's own Sun (133 800 lx at 1 AU, V −26.74), so
  stars and Sun obey one scale (2.54e-6 lx is the figure often quoted). The colour is the black body of
  T = 4600 (1/(0.92 (B−V) + 1.7) + 1/(0.92 (B−V) + 0.62)) K (Ballesteros, EPL 97 34008, 2012) at unit Rec.709
  luminance. × the transmittance toward the star (the sky's table). The footprint is a unit Gaussian whose peak is
  E / (2π σ² Ω_px), Ω_px = (2 tan(fovX/2)/W)(2 tan(fovY/2)/H) cos³θ: Σ L·Ω over the pixels is E, at any focal length
  (a long lens raises a star's radiance, as a real one does). In chain units (× the pre-exposure), the brightest
  channel bounded at 60 000.
- **The combine**: `em_Color.rgb += stars` where the depth is the sky's (1.0 exactly), × the clouds' view transmittance
  when the scene holds clouds (the cloud transmittance pairing now feeds `Stars` too; a second combine variant, the
  signature counts the samplers). No star is drawn over the ground, a mountain or a cloud's opaque core.

**Measured** (2026-10-25, Linux, RTX 3070 Ti, 1920×1080, sky-lab `0,0,0,15,2,0,0,0,0,1`, day 280 at midnight, 45° N,
the camera on Alnilam at 35 mm; validation layer loaded, 0 VUID):
- **Positions**: nine named stars (Orion, Sirius, Aldebaran) land within 0.2 px of the position predicted from the
  catalogue and the clock's `equatorialFrame` (a uniform −0.5 px is the pixel-centre convention of the prediction);
  at 200 mm the belt still lands within 0.4 px. Betelgeuse and Aldebaran orange, Rigel blue-white, Sirius white.
- **Magnitudes**: the PNG inverted (gamma 2.2, then Narkowicz's ACES fit) on a dim frame (the curve's toe): the
  instrumental magnitude of the 11 stars brighter than V 2.5 follows V with a slope of **1.023**, rms 0.14 mag (the
  extinction and the colours included). On a pinned frame (f/2.8, 0.5 s, ISO 3200: exposure 1.70, the reported
  E_pre), the transmittance each unsaturated star implies, Σx / (E / Ω_px × 1.70), **rises with its elevation: 0.80 at
  23°, 0.83 at 30°, 0.86 at 36°, 0.92 at 42°** — the atmosphere's extinction on an absolute scale (outliers above 1 are
  close doubles inside the 9×9 window).
- **Daylight**: the same frame at noon (auto exposure), with and without the stars: no local excess anywhere (the
  difference is a uniform +2 levels, the metering converging) — the stars vanish in the sky's light by themselves.
- **Cost**: `StarFieldEffect/internal` 0.009 ms (9 096 instances, the clear included) + one more combine pass (the TAA
  splits the overlay group): frame median 1.595 ms against 1.547 ms without stars (`0,0,0,40,2,0,0,0,0,0`), 5 windows.

### The manifest (Backgrounds store)
`"Planet"` (a preset name, default `"Earth"`), `"Atmosphere"` (inline overrides: `GroundRadius`, `TopRadius`,
`RayleighScaleHeight`, `MieScaleHeight`, `AbsorptionLayerWidth` in km; `RayleighScattering`, `MieScattering`,
`MieAbsorption`, `AbsorptionExtinction` per km as `[r, g, b]`; `MiePhaseG` as `[r, g, b]` or one number;
`GroundAlbedo`), the artistic keys (`Tint`, `Intensity`, `Saturation`, `StarDisc`, `LimbDarkening`), the moons'
appearance (`"Moons"`: an array indexed like the clock's moons, each an object with an optional `"Surface"`, a store
texture — § "The moons"), the star catalogue (`"StarCatalogue"`: a store `StarCatalogues` resource name — § "The
stars"; ⚠️ NOT `"Stars"`, the photo sky's light-carrying bodies of `parsePhotometry()`) and the background photometry
(`AbstractBackground::parsePhotometry()`). From code:
`DynamicSkyResource::load(parameters)` (no moon map).

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
| Earth zenith, sun 45°, **134 870 lx** above the atmosphere, LINEAR 5772 K (lot 2, 2026-10-25) | RGB (964, 1799, 3870) nits, L ≈ 1 771 cd/m² with the reference march; **RGB (890, 1649, 3585) nits, L ≈ 1 628 cd/m² with the midpoint march** (a very clear sky by Kittler's zenith formula: ~2 000 at turbidity 2.5); 2^-13 doubles the bytes |
| Earth dusk, 45° N equinox (sun 12.7°, 5.1°, 0.1°, −3.0°, −7.5°) | halo → yellowing → orange horizon under a purple vault → pink-violet → deep blue with an orange band (frames in the `sky-lab` notes) |
| Mars | butterscotch day, a white-to-blue aureole core at sunset (tone-mapped: the core saturates) |

### The night (2026-10-25, RGBA32F tables)
Earth, 45° N, new moon, zenith (post-processor off, the grading raised until the bytes read; the doc's nits protocol):

| Sun | 0° | −3° | −6° | −9° | −12° | −15° | −18° | −21° | −30° |
|---|---|---|---|---|---|---|---|---|---|
| Zenith (cd/m²), midpoint march | 174 | 29.1 | 1.00 | 0.036 | 0.0021 | 8.6e-5 | 7.9e-6 | 3.6e-7 | 0 |
| Zenith (cd/m²), reference march | 187 | 31 | 1.04 | 0.041 | 0.0024 | 8.7e-5 | 7.6e-6 | 3.7e-7 | 0 |

Civil twilight ends at ~1 cd/m², as measured skies do. Below −18° the zenith column is in the planet's shadow up to
327 km (the atmosphere is 100 km thick): only multiple scattering from the sunlit horizon remains, and at −30° nothing
— a PHYSICAL zero of this model. A real moonless night floor (~2e-4 cd/m²) is airglow, zodiacal light and starlight,
none of which the sun's scattering gives: item `dynamic-sky-stars-and-milky-way` § "The night sky's own glow".

### ⚠️ Traps
- ⚠️ **A star's manifest key is `"StarCatalogue"`, never `"Stars"`**: `AbstractBackground::parsePhotometry()` already
  reads `"Stars"` as the array of a photo sky's light-carrying bodies, and a catalogue name there failed the sky's load
  ("The 'Stars' key must be an array", 2026-10-25).
- ⚠️ A pass that samples the sky's frame texture in a NEW pipeline stage must add that stage to the texture's barriers
  (`SkyAtmosphere::recordFrame()` and the bake): the star field's vertex reads were outside a fragment/compute-only
  dependency.
- ⚠️ A star's brightness on a screenshot is NOT linear: invert the gamma (2.2) and the ACES fit, and measure on the
  curve's toe — on a pinned bright frame the shoulder's 8-bit inversion compressed the magnitude slope to 0.83.
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
- ⚠️ **Two sources for the moons, too**: `Math::Astronomy::lunarReflectance()` (C++, the light's phase law and the
  photometry table) and `AtmosphereGLSL::LunarReflectance` (the disc); `MoonSurfacePhotometry::subSolarPoint()` and the
  material's moon frame (toward, north from the pole, east, the map's u and v). A change to one is a change to the
  other, re-measured (the disc against the light, § "The moons").
- ⚠️ **The moons' derivatives.** Implicit derivatives are undefined in non-uniform control flow, and the first version
  took the map's `dFdx` / `dFdy` in the uniform branch on the moon COUNT — so EVERY sky pixel computed the sphere hit,
  the frame, `atan` and `asin`: +0.11 ms of `ScenePass` on the Apple M6 at 2560 × 1440 (+22 %; Linux +0.001 ms). Now
  (owner decision 2026-10-25, analytic gradients): only `skyDir`'s screen derivatives and the edge's `fwidth()` stay in
  uniform flow; everything else waits for a covered pixel, and the map's gradients come from the chain rule — the hit
  (|p| = R: dt = −t (p · dd) / (p · d)), the normal (dn = (dp − n (n · dp)) / R), the longitude and latitude (du = (x dy
  − y dx) / 2π(x² + y²), dv = −dz / π√(1 − z²)) — read with `textureGrad()`. Same anisotropic filtering: the 8K full disc
  integrates to 1.0014 / 1.0009 / 1.0020 at 400 / 800 / 1 600 mm (1.0023 / 1.0015 / 1.0025 before); the pixels differ
  by 0.2–0.5 of a byte inside the disc, at most 39 at the limb (the hardware's per-quad derivatives against the exact
  per-pixel ones; p · d → 0 there, clamped). The equirectangular seam (u wraps at the far side) is never on screen.
  Checked: the chain rule matches the finite differences of the exact coordinates within 0.1–1 % (a numerical model of
  the 1 600 mm view); the texel-scale contrast of the disc (mean squared neighbour difference) is 1.03 / 1.00 / 1.02 × the
  hardware-derivative version on Linux (centre / middle / edge rings). The Apple M6 (MoltenVK) reads it 0.84 / 0.88 /
  0.97: its LOD from explicit gradients differs from its own derivatives' by a few tenths of a mip, invisible at 1:1
  (macOS-PA, 2026-10-25). Cost after the fix: the M6's `ScenePass` +0.04 ms with the Moon (median of 5 pairs, within its
  ±0.05 ms launch noise; +0.114 ms before).
- ⚠️ **A descriptor set allocates EVERY binding of its layout**, written or not. The sky's pool asked 4 uniform buffers /
  5 samplers / 6 storage images for four sets of a layout that needs 2 / 2 / 2 each (fixed 2026-10-25 with the moon
  binding, `SetCount × 2`); a driver that enforces the counts answers `VK_ERROR_OUT_OF_POOL_MEMORY` on the third set.
- ⚠️ **Check a new surface map's ENCODING before using it.** The 14K source of projet-alpha's maps was tagged sRGB
  but held linear values: decoded as sRGB its highlands / maria ratio read 6.1 (the Moon: ~2; the kit's own 2K JPEG
  2.39), the maria near black and the light's month asymmetry exaggerated with them. Read as linear values it gave 2.44.
  Compare the mean of a few maria (Tranquillitatis, Imbrium, Procellarum) with the highlands in linear light.
- ⚠️ **Measuring a moon's disc**: the pixel's solid angle comes from the LENS (`Camera.getState()`'s
  `fieldOfViewDegrees` is the vertical field), never from a thresholded radius — the outermost pixel above a threshold
  overestimates the radius, and the first full-moon reading was 0.939 instead of 0.997. Dim phases need a raised
  grading: a peak of 23 bytes quantised the crescent's integral by 1 %. And read the clock's state only once it
  follows the sky (`getState()`'s `sun.model` is `atmosphere`): before the sky is loaded the light comes from the
  empirical model, and an 8K run read 1.157 for 1.002.
- The moon lights carry NO shadow map (`Toolkit::generateCelestialClock()`): two shadowing directional lights would double
  the shadow cost at dusk; a shadowed moon would be an option to add. And a CSM directional light adds no light at all
  when shadow mapping is off (item `sun-unlit-without-shadow-mapping`): the moons inherit it.
