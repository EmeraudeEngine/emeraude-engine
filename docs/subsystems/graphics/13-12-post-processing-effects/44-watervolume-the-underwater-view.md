## 12. Post-Processing Effects

### WaterVolume — the view from under the sea (Nov 2026)

> Owner decisions (2026-11-23) and what is still open: the engine item
> [`underwater-view`](../../../todo/underwater-view.md). This file is what the code does and why.
> ⚠️ **State on 2026-11-23: verified on Linux (RTX 3070 Ti) only, not committed.** macOS, Windows and AMD have not
> run it.

A scene whose sea **declares its water** gets an underwater view: under the surface the frame shows water — a veil of
the water's own colour over the distance, the sea surface seen from below — instead of dry ground under a bare sky.
Three pieces, each in the place that owns it.

#### 1. The water is declared by the sea, on its surface material — two families with levels

| Family | Common level | Members |
|---|---|---|
| The participating media (`Scenes/`) | `ParticipatingMedium`: absorption σa and scattering σs per channel in 1/m, the Henyey-Greenstein anisotropy, the enabled state, `transmittance(distance)`. A VALUE type: no virtual, a protected destructor | `AtmosphericMedium` (the air of a scene: a density thinning with the altitude, an albedo, a luminance — the former `ParticipatingMedium`, `Scene::setAtmosphericMedium()`), `WaterMedium` (the water of a sea: homogeneous) |
| The seas (`Graphics/Renderable/`) | `AbstractSea` (`Renderable::Abstract` + `Scenes::SeaLevelInterface`): the level, the surface material and the water it declares, the flat-level answers, the two-sided drawing | `BasicSeaResource` (a flat grid: the lake), `OceanResource` (FFT waves) |

- The common level's coefficients are set by its members only (`setAbsorption()` / `setScattering()`, protected; the
  water publishes the second).
- `WaterMedium` states its absorption the way the materials state a volume (KHR_materials_volume): **the colour the
  light keeps after a distance**, kept as stated — the surface material takes those two numbers, the coefficient per
  metre is derived (σa = −ln(colour) / distance).
- **The ONE statement of a sea's water is on its surface material** (owner decision 2026-11-24):
  `StandardResource::setWaterVolume(water, thickness)`, while the material loads, sets in one call everything
  the surface reads of its water — the transmission through the grab pass absorbed by the water's attenuation, the
  real water column as its thickness (depth-based opacity), water's index, the diffuse reflectance its back face
  shows — and keeps the water. `Material::Interface::waterVolume()` answers it (null by default), and
  `AbstractSea::waterMedium()` answers the scene with it: the column seen from above and the view from below agree
  **by construction**, nothing is typed twice. The seas' `load()` take no water.
  - The sea answers once it is ready for instantiation: its material, a dependency, is then loaded and its statement
    no longer changes. `Renderer::sceneDeclaredSea()` asks every frame from the render thread, behind that acquire.
  - Why: until then a sea's `load()` took the material and the water side by side and the application copied the
    absorption, the reflectance and the index into the material by hand — two waters could disagree without a
    message.
  - ⚠️ The dynamic setters of those values (`setIOR()`, `setAttenuationColor()`, `setAttenuationDistance()`) stay
    free: called after `setWaterVolume()`, they make the surface disagree with the water it declared.
  - ⚠️ `setWaterVolume()` refuses a water without extinction (a default-constructed one declares nothing) and a
    material already loaded.
- `SeaLevelInterface::waterMedium()` is null for a sea that declares no water: that sea is drawn and behaves as it
  always did (its upper side alone, no underwater view).
- A sea whose material declares its water is drawn on **both sides** (`AbstractSea::layerRasterizationOptions()`):
  a pipeline is built with the sides the sea has when it becomes ready, and the material's statement is made before
  that.
- `AbstractSea::getLevelAt(position)` is the ONE customization point of the level (`isSubmerged()`, `getDepthAt()`
  and the three-float `getLevelAt()` follow it, `final`). It still answers the level at rest for the ocean: the CPU
  does not know the waves (engine item `ocean-fft-surface`, step 6).

#### 2. The water between the eye and what it sees — `Effects::Atmosphere::WaterVolume`

- **Scene-driven**: `PostProcessStack::syncSceneEffects()` files it the first frame the scene's sea declares its
  water (`Core/Graphics/PostProcessing/WaterVolume/Enabled`, read once). Slot `WaterVolume`, **after `Fog`**: the
  water is the medium nearest to the eye, everything behind it reaches the eye through it. After the translucent
  pass: the sea surface wrote its depth, which is where the path in the water ends when the eye looks up.
- **The model** (S. Premože, M. Ashikhmin, "Rendering Natural Waters", CGF 20(4), 2001 — the analytic single
  scattering along a view path under a downwelling light that decays with depth). A pixel whose ray starts under the
  surface becomes `scene · T + L` over its path `s` in the water:
  - `T = exp(−σt · s)`;
  - `L = S · exp(−Kd · z₀) · (1 − exp(−(σt − Kd · ωy) · s)) / (σt − Kd · ωy)`, `z₀` the depth of the path's start,
    `ωy` the ray's vertical component (looking up runs toward the light). The rate can reach zero or go negative
    when looking up: the closed form holds, the shader keeps the division away from zero;
  - `S = σs / 4π · E`, `E` the illuminance that ENTERED the water — the sun through the surface (Schlick at its
    incidence; a disabled main light lights nothing) and the sky (the ambient the shading uses when it is a scalar,
    else π × the background's luminance, white);
  - `Kd = (σa + B · σs) / μ`, `B` the backward share of the scattering (from the anisotropy: 5 % at g = 0.8), `μ` the
    cosine of the refracted sun, never under 0.5.
  The water's colour comes out of its coefficients: no colour is authored in the effect.
- **Three cases a frame, read from `FrameContext::seaImmersion`** (`Graphics/SeaImmersion.hpp`) — THE answer to "is
  the eye under water?", decided once a frame where the chain's context is built
  (`PostProcessor::executeIndirectPostProcessEffects()`), on the CPU, from the near plane's four corners against the
  sea level and how far its surface can stand from it (owner decision 2026-11-24: until then the water volume and the
  sun's shafts each decided on their own, differently — the near plane against the moving surface, the eye's height
  against the level at rest). `Clear`, the near plane stands clear of the highest the surface can be — **the effect
  is not executed**; `Submerged`, it stands under the lowest the surface can be — the variant AT REST, every pixel
  under water; `Astride` — the MOVING variant (the variant at rest, against the level, for a surface that does not
  move). Two pipeline variants rather than a branch in the shader. Clear whenever the scene has no sea that declares
  its water.
  - ⚠️ `Astride` is a BOUND, not a sighting: `OceanSurfaceParameters::waveReach` is 8 m by default, so an eye 5 m
    above a calm sea records the moving variant on every frame although no pixel of it is under water: 0.064 ms a
    frame at 720p on an RTX 3070 Ti (the measurements, below).

#### 3. The waterline — a pixel is stood against the MESH

A pixel is under water when its point on the **near plane** (`emRelativePosition(uv, 0)`) stands below the drawn
surface. For a sea at rest that surface is its level. For the ocean it is the mesh, and nothing else will do:

> ⚠️ **1 mm of mismatch at the lens is 9 pixels at 720p** (the near plane is 0.1 m away, a pixel covers 0.11 mm of
> it). A height read from the cascades' textures is not where the surface is DRAWN: the vertex stage fades a cascade
> finer than its step (`ocCascadeWeight()`: with the default 0.5 m cell the 8 m cascade is not in the mesh at all, it
> only feeds the per-pixel normals), samples at its vertices only and is flat in between.

**Where the mesh stands is the OCEAN's own answer** (owner decision 2026-11-24: one source). `Graphics::SeaSurfaceWaves`
(answered by `SeaLevelInterface::surfaceWaves()`, null for a surface at rest and until an ocean's first update) hands
the pass the very resources the ocean's stages bind — the displacement cascades, the shelter map, the surface's
uniform block of the frame — and the moving variant compiles the ocean's own GLSL over them
(`Saphir::Generator::getOceanSurfaceStandingSource()`, the declarations `declareOceanSurface()` gives the ocean).
`ocSurfaceRiseAt(xz)` reads the height on **the displaced triangle that covers the point**: the vertex stage's own
sum (`ocDisplacementAt()`, the cascades faded and sheltered by `ocCascadeWeight()`) at the four corners of a finest
cell, the cell cut along its (0,0)–(1,1) diagonal (the patch's index order), found by walking from the cell a first
guess lands in (two rounds of `x − D(x)`), four steps at most. A folded cell answers its first corner. Nothing of the
mesh is restated in the effect: until that decision it was — the cascades' fade computed again in C++, the sum
written again in GLSL — and a change of the ocean's vertex stage would have moved the waterline silently
(graphics doc 44, `44-the-oceans-sheltered-water.md`, part 3). Where the lens meets the water, a meniscus: a 2 px dark line on both sides (`MeniscusHalfWidth`,
`MeniscusTransmission`).

#### 4. The surface seen from below — the material, on its back face

`StandardResource::enableDepthBasedOpacity()` makes a surface the interface of a volume that reaches the scene behind
it; its back face is seen from INSIDE that volume:

- **no water column** there (`gpWaterColumnThickness` is 0 on a back face): what lies behind the surface is the air,
  and the sky behind it read as kilometres of column — a full absorption, a black underside;
- **no foam** (the column being zero, the whole underside would be a shore);
- **the refraction** takes the index the other way round (n instead of 1 / n) and the normal facing the ray;
- **the Fresnel split** (`LightGenerator::declareSurfaceVolumeInterface()`, `interfaceCosineExpression()`): Schlick's
  term takes the cosine of the TRANSMITTED angle, which is zero at the critical angle — the term is then exactly 1,
  the total internal reflection around Snell's window;
- **what that mirror shows is the water below, never the sky above**: the volume's DIFFUSE REFLECTANCE
  (the water's, written by `StandardResource::setWaterVolume()` — the material block's `volumeReflectance`), lit from the OTHER side of
  the surface — by the sky in the ambient pass (reflectance × the environment's luminance), by each direct light in
  its own pass (reflectance / π × its radiance × the cosine on the surface's OUTWARD normal), both weighted by the
  interface's reflectance. The surface's own diffuse lobe could not carry it: KHR_materials_transmission gives the
  whole energy to the transmission. The water answers that reflectance (`WaterMedium::diffuseReflectance()`): the
  irradiance reflectance of ocean optics, **R = 0.33 · bb / (a + bb)** with bb the backscattering (H. R. Gordon,
  O. B. Brown, M. M. Jacobs, Applied Optics 14(2), 1975; A. Morel, L. Prieur, Limnology and Oceanography 22(4),
  1977) — (0.014, 0.060, 0.078) for the lagoon.
  ⚠️ A first formula, σs / 4 (σt + Kd / 2) — the isotropic single scattering of the veil restated as a reflector —
  gave (0.085, 0.178, 0.193) and a mirror BRIGHTER than the sky in the window: what looks down into a water sees far
  less than what looks up, its scattering being forward.

A material that does not enable the depth-based opacity generates the code it always did.

#### Measured (2026-11-23, Linux, RTX 3070 Ti, `water-world`, window-less 1280 × 720, validation layer loaded)

- 0 VUID, 0 `[Error]`, no shader that fails to compile, in both lighting lanes, over eight underwater poses, three
  eye heights at the waterline and bursts of 30 captures at the sea level.
- The mirror around Snell's window (the darkest quarter of the underside, eye at −2 m looking seaward, 0-255):
  (57, 65, 69) when the total reflection returned black, (63, 98, 107) with the water's reflectance — the water's
  colour, darker than the sky in the window.
- Before: an eye at −2 m and at −30 m saw dry sand under a bare sky, the sun's lens flare included. After: the seabed
  fades into a turquoise veil over a few tens of metres, the abyss shows the water's colour, Snell's window shows the
  sky with the wave normals breaking its edge, and a frame the waterline crosses is split along the mesh.
- The flat level alone (the variant at rest, on the ocean) veiled an eye that stood at the sea level in a wave's trough,
  in the AIR: the capture of that pose lost its veil with the moving variant.
- The air side of the media refactor (`light-and-shadow-debug`, six captures before, six after): a before and an
  after capture differ by 0.024 to 0.064 / 255 on average, two captures of one run by 0.039 to 0.064; the band means
  agree to three decimals.
- **The rewiring of 2026-11-24** (the water stated on the surface material, the frame's one immersion state) changes
  one thing on screen, the one it was made for. Same day, same machine, three series of 12 poses, 30 captures at the
  sea level and the sun's shafts on / off — two with the binary of before, one with the binary of after — mean
  absolute difference per channel level (0-255), before × after against before × before:
  - the abyss (−30 m, level and up) 0.000 and 0.006; the seabed seen from −2 m 0.002; the underwater poses that show
    the moving surface 0.25 to 0.49, under the 1.0 to 2.3 of the two launches of before;
  - the sun's shafts: +24.47 luma under the water (24.44 and 24.48 before), and above it, on the sky's rows, an image
    identical to the last digit with the shafts on as off — the term the Astride frame now enables changes nothing
    there;
  - ⚠️ the eye at +0.4 m under a crest: 5.4 to 6.0 against 1.6. The shafts now reach that frame's underwater pixels;
    the test on the eye's height cut them at the level at rest, under the very surface the water volume was drawing.
  - 0 VUID, 0 `[Error]`, no shader that fails to compile, in the nine launches of the binary of after.
- **The pass's cost** (2026-11-24, RTX 3070 Ti, window-less 1280 × 720, GPU profiler on, validation off, the machine idle,
  three alternated on / off runs of 3 s — the `WaterVolumeEffect` zone of `getGPUTimings()`, a frame of 4.1 to 4.6 ms):
  0 ms on a `Clear` frame (eye at +20 m: not recorded); **0.064 ms on an `Astride` frame** — at +2 m on the beach, no
  pixel under water, as at the waterline; 0.077 ms at −2 m, every pixel under water; 0.022 ms on a `Submerged` frame
  (−9.5 m, the variant at rest). The moving variant costs three times the variant at rest and is paid from 8 m above
  the sea to 8 m under it (`waveReach`): 1.5 % of that frame, and it grows with the pixel count (16 times the pixels
  on a 5K screen).
- **Verified on macOS and Windows** (2026-11-24, the peers, engine 94632582: builds, 0 VUID, the five poses; the
  cut along the mesh seen on Linux and macOS, not caught in Windows's twelve frames) — the detail is in graphics doc
  44 (`44-the-oceans-sheltered-water.md`), the measurements. Not run on AMD.
- ⚠️ NOT measured: the mismatch between the waterline and the mesh
  in pixels (judged on magnified captures: the line follows the surface's edge), an orthographic camera.

#### 5. The sun's shafts under the sea — `VolumetricLight` sees through the surface

`VolumetricLight` builds its occlusion mask from the depth: sky pixels emit, geometry blocks. The sea's underside is
drawn and writes its depth: read alone it is a wall, and the shafts vanished the instant the eye went under (owner,
2026-11-23; measured facing the sun at a pinned exposure: the shafts add 62 luma above the water, 2.2 under it —
inside the 10 of the waves' own spread). **A surface that lets the sky through does not block the sun**: on every
frame whose view is not clear of a sea that declares its water (`FrameContext::seaImmersion`, part 2), the occlusion
pass also reads the depth from BEFORE the translucent pass (the material grab pass's, binding 3) — where that one is the sky, the
pixel is lit for what the surface transmits from below (Schlick at the transmitted angle, nothing past the critical
angle, the surface taken at rest; the pixel's ray through the view reconstruction, binding 4). The water volume, which
runs after, attenuates the shafts like everything else. After: +26.7 luma under the water, +62.3 above (unchanged),
0 VUID.
⚠️ These are the AIR's shafts seen through the window — a radial blur around the sun's screen position. The beams the
waves break, coming down from the surface, are the item `underwater-light-shafts`.
⚠️ On an `Astride` frame the eye may be in the air. The term then changes nothing where the sky or an opaque surface
is seen, and nothing on the sea seen from above (a ray that goes down is let through by no amount). What it wrongly
lights is a translucent surface OTHER than the sea, seen more than 41° above the horizon in front of the sky — the
surface is taken for the sea's underside (same item).

#### ⚠️ Not what a sea shows yet

- **The veil's source is isotropic**: the light the water scatters toward the eye is the same in every direction.
  Looking down it is brighter than a forward-scattering water gives (the mirror's reflectance above is the measured
  kind; the veil is not yet).
- **The window shows the sky straight through** (the grab pass, offset by the material's thickness): the hemisphere
  is not compressed into the 97° cone.
- **The light's own path through the water** (the sun dimming and turning blue with depth on what it lights), the
  caustics, the beams: their own items (`underwater-caustics-and-depth-light`, `underwater-light-shafts`); the sound:
  audio doc 15. `VolumetricLight` still draws the air's shafts under water.
- The fog response of a surface (the material properties' alpha) is not read: under water nothing is exempt.
- **The air's media under water**: the fog, the aerial perspective, the volumetric scattering and the clouds still
  treat the part of a view path that lies under water as air (item `air-media-under-water`, the owner's decision of
  2026-11-24 for this lot). `water-world` runs none of them by default; with its physical sky the aerial perspective
  measured invisible under water (−0.3 to +0.9 level on a channel's mean, under the spread of two captures). A local
  fog together with such a sea is not measured: no demo has both.
- A dither on the edges of the water surface seen from above at grazing closeness shows along the waterline too; it
  is on the captures taken before this work (cause not investigated).

**Code references:** `Scenes/ParticipatingMedium.hpp`, `AtmosphericMedium.hpp`, `WaterMedium.hpp`,
`SeaLevelInterface.hpp`; `Graphics/Renderable/AbstractSea.hpp`; `Graphics/SeaSurfaceWaves.hpp`;
`Graphics/Geometry/OceanSurfaceResource.cpp` (`createSurfaceResources()`); `Graphics/Effects/Atmosphere/WaterVolume.hpp/cpp`;
`Graphics/PostProcessStack.cpp` (`fileWaterVolumeEffect()`); `Graphics/Material/StandardResource.cpp` (the grab-pass
transmission, the shore foam); `Saphir/LightGenerator.cpp` (the reflection + transmission branch).
