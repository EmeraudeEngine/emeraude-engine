---
id: underwater-view
title: The view from under the sea — the water volume, the surface seen from below, the waterline
status: in-progress
priority: unranked
scope: Scenes (media, sea level), Graphics/Renderable (seas), Graphics/Material, Graphics/Effects/Atmosphere, Graphics/PostProcessStack
opened: 2026-11-23
tags: [water, ocean, participating-medium, post-process, state-of-the-art]
---

# The view from under the sea — the water volume, the surface seen from below, the waterline

## Why

Owner (2026-11-23, on projet-alpha `water-world`): "j'aimerais qu'on fasse l'effet quand la caméra est sous l'eau".

Measured the same day (Linux, RTX 3070 Ti, window-less test instance, ray-traced and screen-space lanes, 0 VUID):
under the sea level the scene shows NOTHING of the sea. The ocean is drawn with back-face culling, so it does not
exist from below; there is no medium, so an eye at −2 m or at −30 m sees dry sand under a bare sky, the sun's lens
flare included. And the CPU does not know the waves (`OceanResource::getLevelAt()` answers the flat level while the
default sea has Hs ≈ 2 m): near the surface, "is the eye under water" can only be decided per pixel, on the GPU.

## Owner decisions (2026-11-23, AskUserQuestion)

1. **Reference: a realistic lagoon** — very clear water, 30 to 40 m of visibility, blue-cyan, the red going first with
   the distance, the light falling with the depth. Measured coefficients (Jerlov's water types) are the target; they
   are compared ON SCREEN with the owner at the tuning step, because they change the from-above look he approved on
   2026-09-28 ("une belle plage claire"). Until then the lagoon's water starts from that approved absorption.
2. **This lot is the base**: the water volume (absorption, scattering, the water's own glow dimming with depth), the
   sea surface seen from below (Snell's window, total internal reflection around it) and the waterline decided per
   pixel when a wave crosses the lens. Caustics, underwater light shafts and the muffled sound are wanted and come
   next, one item each: `underwater-caustics-and-depth-light`, `underwater-light-shafts`; the sound is done
   (`docs/subsystems/audio/15-the-listeners-medium.md`).
3. **The sea level owns the water's optics**: the scene's sea declares its water once; the underwater effect AND the
   surface material read it, so above and below agree by construction. The effect is filed by the scene (a sea that
   declares its water is enough), as the clouds are.
4. **The media are a family with levels**: `Scenes::ParticipatingMedium` becomes the common level (absorption and
   scattering per channel in 1/m, phase anisotropy); below it `AtmosphericMedium` (the former content: height
   profile, luminance, the `ClearAir` / `Fog` presets) and `WaterMedium` (homogeneous). The air side is refactored and
   proven unchanged.
5. **The seas are a family with a level**: `Renderable::AbstractSea` between the contract (`Renderable::Abstract` +
   `Scenes::SeaLevelInterface`) and the two seas, on the model of `AbstractBackground`: the level, the material, the
   water, the common answers and the two-sided drawing; each sea keeps its geometry and its height source (flat, or
   the FFT cascades).
6. **The light's own path through the water is NOT in this lot**: the sun dimming and turning blue with depth on what
   it lights comes with the caustics, in the lighting, for both views. This lot handles what lies between the eye and
   what it sees; the lagoon seen from above stays unchanged. Known consequence until then: a deep seabed seen at
   close range under water keeps its sunlit sand colour.
7. **The ONE statement of a sea's water is on its surface material** (2026-11-24, amends 3 — the integration reread
   on his request: a sea's `load()` took the material and the water side by side, the application copied one into the
   other by hand, nothing tied them): `StandardResource::setWaterVolume()`, answered by
   `Material::Interface::waterVolume()`; the sea reads it to answer `waterMedium()`, its `load()` takes no water.
   The effect still asks the sea. Graphics doc 13-12 § 44, part 1.
8. **"Is the eye under water?" is established once a frame, as three states** (2026-11-24): where the near plane
   stands against the sea — `Clear`, `Astride`, `Submerged` (`Graphics::SeaImmersion`,
   `FrameContext::seaImmersion`), decided where the chain's context is built. Every effect reads it; only the water
   volume decides per pixel, on an Astride frame. A per-pixel mask any effect could read was the other option: not
   taken (a pass and a target more). Same doc, part 2.
9. **The air's media are left as they are in this lot** (2026-11-24): the fog, the aerial perspective, the
   volumetric scattering and the clouds still treat the part of a path under water as air — their own item,
   `air-media-under-water`.
10. **One source for where an ocean's surface stands** (2026-11-24, with the sheltered water — engine item
    `ocean-sheltered-water`, decision 4): the moving variant compiles the ocean's own GLSL over the ocean's own
    resources. The debt recorded here on the rereading (the effect restated the mesh) is closed: graphics doc 44,
    part 3.

## Defaults stated to the owner (2026-11-23, no objection)

- The effect is `Effects::Atmosphere::WaterVolume` (that directory is the participating media's), in a new chain slot
  `WaterVolume` AFTER `Fog`: the water is the medium nearest to the eye, it composites last. Above the water it is not
  executed.
- The waterline: per pixel, the water height evaluated at the pixel's point on the NEAR plane (the FFT cascades for
  the ocean, the flat level for the lake), plus a meniscus that covers the gap with the drawn mesh — that gap is to
  be measured.
- The surface from below: the sea is drawn two-sided; on the water side the material inverts the index, applies the
  total internal reflection outside Snell's window, and applies neither the water column nor the foam. The total
  reflection returns the water's own colour; the seabed's mirror image is not in this lot.

## The plan

1. The two families (a pure refactor, proven unchanged): the media, the seas.
2. `WaterVolume` with the flat level: scene-filed, a pass-through above the water.
3. The surface seen from below.
4. The waterline with the waves, and the meniscus.
5. Tuning on screen with the owner (visibility, colours; the measured preset against the approved one).

## Progress (2026-11-23, Linux, RTX 3070 Ti — NOT committed, the other OS not run)

**Step 1 done — the two families.** `Scenes::ParticipatingMedium` is the common level (σa and σs per channel, the
phase anisotropy, the enabled state; a value type, protected destructor), `Scenes::AtmosphericMedium` the former class
(`Scene::setAtmosphericMedium()` / `atmosphericMedium()`, `FrameContext::medium`), `Scenes::WaterMedium` the water
(stated as the colour kept after a distance — the form the surface material consumes —, a scattering per channel, the
presets `Lagoon` / `DeepOcean` / `PeatLake` carrying the absorptions the demos had). `Renderable::AbstractSea` holds
the level, the material, the water, the flat-level answers (`getLevelAt(position)` the one customization point) and
the two-sided drawing of a sea that declares its water; `BasicSeaResource` (now `final`, its unused public
`setWaterLevel()` removed: it moved the answer, never the grid) and `OceanResource` keep their geometry.
`SeaLevelInterface` gained `waterMedium()` and `surfaceWaves()`.
Proof on the air side (`light-and-shadow-debug`, six captures before and six after, one launch each): the mean abs
difference between a before and an after capture is 0.024 to 0.064 / 255, inside the 0.039 to 0.064 of two captures of
ONE run; the three band means agree to three decimals; 1 183 of the 1 313 pixels that differ by more than 2 / 255
between the averaged images sit on the animated character (the control, the two halves of the before run: 1 609).
The surface material receives the same two numbers through the water (`WaterMedium::Lagoon()` keeps the colour and
the distance as stated): not re-measured per pixel — the sea moves.

**Step 2 done — `Effects::Atmosphere::WaterVolume`**, slot `WaterVolume` after `Fog`, filed by
`PostProcessStack::syncSceneEffects()` the first frame the scene's sea declares its water
(`Core/Graphics/PostProcessing/WaterVolume/Enabled`), `FrameContext::sea`. The analytic single scattering of the
class comment; the source is the sun through the surface plus the sky. 0 VUID, the seabed fades into a turquoise veil
over a few tens of metres, the abyss shows the water's colour instead of the sky.

**Step 3 done — the surface from below.** `StandardResource`: on the back face of a depth-based-opacity material
the water column is zero, the foam is off, the refraction takes the index the other way round and the normal facing
the ray; `LightGenerator::declareSurfaceVolumeInterface()`: Schlick's term takes the cosine of the transmitted angle
(total reflection past the critical angle), and what that mirror shows is the water's diffuse reflectance lit from
above (`StandardResource::setVolumeReflectance()`, `WaterMedium::diffuseReflectance()`: R = 0.33 · bb / (a + bb)), by
the sky in the ambient pass and by the sun in its light pass. Snell's window shows the sky, the wave normals break
its edge, the mirror around it is the water's colour and darker than the window. ⚠️ ONE thing is not what a sea
shows: the window shows the sky STRAIGHT through (the grab pass, offset by the material's half metre of thickness) —
the hemisphere is not compressed into the 97° cone. A lookup by the refracted DIRECTION would do it (the environment
cubemap past the screen's edges).

**Step 4 done — the waterline.** `Graphics::SeaSurfaceWaves` (the cascades, the sampler, the weights the mesh gives
each cascade at its finest cell, that cell, the reach) answered by `OceanSurfaceResource::surfaceWaves()` once the
surface has been updated; the effect has two pipeline variants (the surface at rest / the moving surface) and three
cases a frame from the near plane's corners (clear of the water: not executed; deeper than the reach: at rest; in
between: moving). The moving variant stands each pixel's near-plane point against the MESH: the displaced triangle of
the finest cell that covers it (the vertex stage's sum at the cell's corners, the cell cut along its (0,0)-(1,1)
diagonal — `OceanSurfaceResource.cpp`, the patch's indices), found by walking from a first guess. A meniscus of 2 px.
⚠️ 1 mm of mismatch at the lens is 9 px at 720p (the near plane is 0.1 m away): anything but the mesh's own
interpolation shows.

Seen on captures: an eye standing at the sea level in a trough, in the AIR, lost the veil the flat level gave it;
of a burst of 30 captures at the sea level, three are cut by the waterline and the veil stops on the surface's edge
(`water-world`; the ocean's clock starts with the process, the burst replays the same waves at every launch).
⚠️ The mismatch with the mesh was judged on magnified captures, not counted in pixels. A dither on the edges of the
water surface seen from above at grazing closeness shows along the waterline too; it is there with the depth of field
and the motion blur off, and on the captures taken before this work — cause not investigated.

**The gate** (`gate.py commit`, three runs): GCC Release and clang 0 error / 0 warning, cppcheck 0,
include-what-you-use 0 at the third run, clang-tidy 18 → 1, the one kept on purpose (the chain's pass-through
contract: `docs/static-analysis-ledger.md`, the lot's row and § 7g). The docs are written: graphics doc 13-12 § 44
(new), § 09 and § 12, scenes doc 02, the material doc § Depth-based opacity, the changelog, projet-alpha's
`water-world` doc.

**The owner on screen (2026-11-23): "I'am bluffed ! C'est trop beau, mais j'ai repéré deux bugs".**
- The shore foam turned into a thick white line through the waves for an eye at the sea level. A defect of the foam
  that PREDATES this work (he said so himself after a second look; a capture of the build before it shows the band
  straight to 1.9 px). His decision: the foam is kept only where the line of sight lands near where it entered the
  water (two to four foam widths of horizontal run). Done; graphics doc 05 § Depth-based opacity and shore foam holds
  the numbers. ⚠️ The band thins at moderate heights too (−71 % of its white pixels from 2.5 m up): his to judge.
- The sun's shafts vanished under the water — caused by this work (the underside writes its depth and hid the sky
  from `VolumetricLight`'s mask). His decision: let them through the surface now, the wave-broken beams staying
  their own item. Done; graphics doc 13-12 § 44, part 5.
- The total reflection's missing term: his decision, after those two. Done (step 3 above).

## What remains

- Snell's window compressed (step 3 above); the veil's isotropic source (graphics doc 13-12 § 44).
- AMD. **Verified on the three OS (2026-11-24, the peers, on base 7824170 / engine 94632582 / projet-alpha 7c172234).**
  macOS (Apple M6, MoltenVK, AppleClang 21; the screen-space lane alone) and Windows (RTX 3500 Ada, MSVC 14.44 /WX; both
  lanes): the cascade in Release and Debug, 0 error, 0 warning; the base suite as expected (2867 and 2864 ran, 2860
  passed on each); `water-world` with the validation layer loaded, 0 VUID, no shader that fails to compile. The log's
  shelter line gives the SAME areas on the three OS — 15 381.2 m² of 5.90067e+06 m² — in 39 to 44 ms on the M6 and
  166 ms on Windows. The five poses show what Linux shows: the veil and the surface from below, Snell's window, a
  basin that does not move (mean difference between two frames a second apart: 0.26 to 0.34 on the pond, 4 to 16 on
  the open sea), a straight waterline in the pond. ⚠️ Windows caught no frame cut by the waterline in its twelve (each
  was all air or all water): the cut along the mesh is seen on Linux and macOS only. ⚠️ On the M6 the water volume
  joins the chain about 10 s after the scene is up (the sea answers its water once it is ready for instantiation).
  NOT run: AMD (no session on the AMD laptop that day). `gate.py feature` ran on Linux the same day (the Debug build, the base suite under ASan /
  UBSan and TSan, 0 report).
- ⚠️ An `Astride` frame is a BOUND: with the default `OceanSurfaceParameters::waveReach` of 8 m, an eye up to 8 m
  above a calm sea records the moving variant (up to 18 evaluations of the displacement a pixel) on every frame with
  no pixel under water — `water-world` keeps that default. Measured 2026-11-24: 0.064 ms a frame at 720p on an RTX
  3070 Ti, 1.5 % of the frame, growing with the pixel count (graphics doc 13-12 § 44, the measurements). A tighter
  bound needs the surface's real height near the eye, which the CPU does not have (`ocean-fft-surface`, step 6): to
  weigh against that cost on a large screen, never measured there.
- Step 5, with the owner on screen: visibility, colours, the meniscus, the measured coefficients against the approved
  ones; which demos declare their water (only `water-world` does).

## ⚠️ Traps

- ⚠️ `ocCascadeWeight()` fades a cascade whose texel is finer than the vertex step: with the default 0.5 m cell the
  8 m cascade is NOT in the mesh (it only feeds the per-pixel normals). A height evaluated with every cascade is not
  where the surface is drawn.

- The water material's depth-based opacity reads the scene BEHIND the surface: from below that is the sky (the clear
  depth), i.e. kilometres of "water column" and full absorption. The back side must not take that path.
- The translucent pass writes the sea's depth: the effect's distance is the sea surface where the eye looks up, the
  seabed elsewhere — the fog's rule ("translucents at the depth they wrote").
- `VolumetricLight` and the air's effects run before the water volume and know nothing of it: under water their
  shafts are those of the air (item `underwater-light-shafts`).
- The scene colour holds nits × pre-exposure: a water colour is a chromaticity scaled by a luminance, never a [0, 1]
  constant (graphics doc 13-12 § 12, the fog's own mistake).

## References

- S. Premože, M. Ashikhmin, "Rendering Natural Waters", Computer Graphics Forum 20(4), 2001.
- N. G. Jerlov, "Marine Optics", Elsevier, 1976 (the water types).
- R. M. Pope, E. S. Fry, "Absorption spectrum (380–700 nm) of pure water. II. Integrating cavity measurements",
  Applied Optics 36(33), 1997.
- D. Akkaynak, T. Treibitz, "A Revised Underwater Image Formation Model", CVPR 2018 (the coefficients per Jerlov
  type, per camera band).
- Crest Ocean System (Wave Harmonic), its underwater renderer: the two-sided surface, the per-pixel mask, the
  meniscus — https://crest.readthedocs.io/en/4.23.0/user/underwater.html (the method only; no code reused).
- Unreal Engine, the Single Layer Water shading model: absorption and scattering coefficients carried by the water
  body — https://dev.epicgames.com/documentation/unreal-engine/single-layer-water-shading-model-in-unreal-engine
