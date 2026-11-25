## 44. The ocean's sheltered water — where the land calms the sea (Nov 2026)

`Renderable::OceanResource` is ONE infinite plane displaced by the same FFT cascades everywhere. On a ground with
hollows under the sea level — `water-world`'s archipelago: wide islands whose middle dips below zero — the plane
showed there as a pond, as rough as the open sea. Owner (2026-11-24): "il n'y a finalement aucune force physique qui
peut faire bouger ce niveau d'eau conscrit dans une île. […] Comme une espèce de map qui indique à quel endroit la
mer bouge et à d'autres où ça reste calme ?"

The answer is that map — the **shelter map** — and four owner decisions (same day): the ENGINE makes it from the
relief; a sheltered water keeps small ripples (the swell goes, the smallest waves stay, much attenuated); a basin
joined to the open sea by a passage narrower than a threshold stays calm; and where the surface stands has ONE source,
read by the ocean and by the underwater view.

### 1. The contract — the scene presents its ground to its sea

`Scenes::SeaLevelInterface::adaptToGround(ground, boundary)` (a pure virtual). `Scene` calls it whenever it holds a
ground and a sea — its construction, `setGroundLevel()`, `setSeaLevel()`, `setBoundary()`
(`Scene::presentGroundToSea()`). Synchronous,
on the thread that builds the scene. `Renderable::AbstractSea` answers nothing (a surface at rest has nothing to
adapt); `OceanResource` builds its map:

- it reads `ground.getLevelAt(x, z, 0)` at the centre of each cell of a square of
  `OceanSurfaceParameters::shelterMapSide` cells a side (1024) over `[-boundary, boundary]` on X and Z;
- base `VertexFactory::buildShelterMap()` tells how open each cell's water is (base doc `vertexfactory/02`): the sea
  is beyond the square's border and spreads through the water; it does not enter a passage narrower than
  `shelterMinimumPassage` (30 m); what it does not reach is sheltered, with a fade of `shelterFade` (25 m) past such
  a passage;
- the map (`Graphics::SeaShelterMap`: one byte a cell and its square) goes to the surface,
  `Geometry::OceanSurfaceResource::setShelterMap()`, and one line of the log tells what it found.

⚠️ The ground is read ONCE, when presented: a ground that is still loading then, or that changes afterwards without
being set again, is not followed. A ground that is not a heightfield is read through the same interface.

### 2. On the GPU — one texture, read through the ocean's one weight

- `OceanSurfaceResource` owns an `R8_UNORM` image of `shelterMapSide` texels a side (1 MB at 1024), clamp-to-edge,
  linear: beyond its square the map's edge holds — the sea around the scene. Binding 5 of the surface's set
  (`OceanSurface::ShelterBinding`), `ocShelter` in GLSL.
- `setShelterMap()` may be called from any thread. The render thread takes the map at the next
  `updateSurfaceVideoMemory()` (a mutex, a generation counter) and records its upload in the frame's own command
  buffer, AHEAD of the FFT: staging buffer → image, the image ending `SHADER_READ_ONLY_OPTIMAL`. The first update
  uploads the open sea everywhere when no map was given. The map is kept: it is uploaded again after the surface's
  GPU resources are created anew.
- The uniform block (`OceanSurface::Uniforms`) gains `shelter` (the square's first corner and the inverse of its
  sides; all zero without a map) and `shelteredShares` (what a sheltered water keeps of each cascade —
  `OceanSurfaceParameters::shelteredWaveShares`, largest cascade first).
- **Every read of the cascades goes through `ocCascadeWeight(cascade, step, open)`**, `open = ocShelterAt(xz)` at
  the UNDISPLACED point: the weight a cascade already had (its fade where the sampling step cannot hold it) times
  `mix(shelteredShares[cascade], 1, open)`. The displacement of a vertex, the previous displacement (its velocity),
  the normal of a vertex and of a pixel, the whitecaps: one function, so nothing of the sea ignores the map. A
  cascade weighed by nothing is not fetched.

The values: `shelteredWaveShares` = (0, 0.04, 0.35) for the 512 m, 64 m and 8 m cascades, a 30 m passage, a 25 m
fade — the first ones, KEPT by the owner on screen (2026-11-25, `water-world`, Linux, from the shore of a sheltered
basin, the commands of § 2b at hand: "ça me paraît super"; nothing was changed). With the default 0.5 m cell the 8 m cascade is not in the mesh at all (it feeds the per-pixel
normals, and a pixel far away fades it like everywhere): **seen from afar a sheltered water is a mirror, its ripples
show at close range.**

### 2b. Tuned while the scene runs (2026-11-25)

The look is tuned on screen, so the values are changed on a running scene (owner decision 2026-11-25: the shares AND
the passage / fade; the water's optics stay fixed at load):

| Command (`Core.SceneManagerService.`) | What it does |
|---|---|
| `getSea()` | JSON: the `level` at rest and `shelter` — `minimumPassage`, `fade`, `swellShare`, `waveShare`, `rippleShare` — or `null` for a sea the land does not calm |
| `setSeaShelteredShares(swell, waves, ripples)` | what a sheltered water keeps of each cascade, largest first; applied to the next frame |
| `setSeaShelter(minimumPassage, fade)` | which water the swell reaches; the shelter map is rebuilt at once, under the scene's exclusive access |

- The contract is the family's: `Scenes::SeaShelterSettings`, `SeaLevelInterface::shelterSettings()` /
  `setShelterSettings()` (a surface at rest answers nothing and takes nothing), `Scene::seaShelterSettings()` /
  `setSeaShelterSettings()` — the scene presents its ground again when the passage or the fade changed.
- The ocean keeps the live values in its surface (`OceanSurfaceResource::ShelterTuning`, under the shelter map's
  mutex): the uniforms of each frame read the shares there, `adaptToGround()` reads the passage and the fade there.
  ⚠️ `OceanSurfaceParameters`' three shelter members are the values at LOAD.
- Refused, nothing changed: a share outside 0 to 1, a negative distance (`SeaShelterSettings::isValid()`); `nan` and
  `inf` are refused by the console's own argument parsing.
- ⚠️ Nothing is saved: the values tuned are written back into the demo's `OceanSurfaceParameters` by hand.

**The shore foam follows the map too** (owner decision 2026-11-25: "l'écume ne devrait pas se dessiner là où c'est
calme"). On an ocean geometry the shore band is multiplied by `ocShelterAt()` at the pixel's lattice point
(`StandardResource::generateShoreFoamFragmentShader()`): none in a sheltered basin, fading with the waves past a narrow
passage, unchanged on a shore of the open sea — a land cell of the map takes the openness of the water nearest to it,
so the filtered read does not eat the band at an open beach. The whitecaps already went through the map.

Measured (2026-11-25, Linux, RTX 3070 Ti, `water-world`, window-less 1280 × 720, ray-traced lane, pinned exposure,
validation layer loaded, 0 VUID, 0 `[Error]`, no shader that fails to compile; six captures 0.6 s apart a state):

- The shares, the basin at (−401, −189) seen from 70 m above, standard deviation of a pixel over time (0-255) on the
  basin: 0.17 at (0, 0.04, 0.35), **5.51 at (1, 1, 1)**, 0.01 at (0, 0, 0) — applied without a relaunch.
- The passage: `setSeaShelter(0, 25)` rebuilds the map in 49 ms and the log's line gives 7 325 m² sheltered instead of
  15 381 m²; `setSeaShelter(30, 25)` gives 15 381.2 m² again (46 ms).
- The foam, near-white pixels of a capture against the binary of the day before at the same poses: the basin from
  above 43 800-47 000 → 12 700-18 600 (what is left is the open pond beside it, which keeps its band), the second
  basin 62 300-66 700 → 25 100-30 800, the basin from its shore 1 700-2 080 → 320-910; the open sea 1 460-2 490 →
  2 190-3 540 (its whitecaps: another instant of the sea, not a change).
- NOT run: macOS, Windows, AMD.

### 3. One source for where the surface stands

The underwater view stands each pixel against the DRAWN surface (graphics doc 13-12 § 44, part 3). Until 2026-11-24
its shader restated the ocean's mesh on its side — the cascades' fade computed again in C++, the displacement sum
written again in GLSL: a change of the ocean's vertex stage would have moved the waterline with no compile error and
no VUID. Now:

- the declarations are built ONCE, in `Saphir/Generator/OceanSurfaceHelper.cpp`: the displacement and shelter
  samplers, the uniform block, `ocShelterAt()`, `ocCascadeWeight()`, `ocDisplacementAt()` and `ocSurfaceRiseAt()` —
  how high the drawn surface stands above the sea level over a world XZ, read on the displaced triangle of the finest
  cell that covers the point;
- `declareOceanSurface()` declares them in the ocean's own stages; `getOceanSurfaceStandingSource(bindings)` returns
  the SAME declarations as GLSL text for a shader written by hand;
- `Effects::Atmosphere::WaterVolume`'s moving variant compiles that text and binds the resources the ocean's stages
  bind (`Graphics::SeaSurfaceWaves`: the displacement, the shelter map, the uniform block of the frame). Its push
  constants no longer carry anything of the mesh (48 bytes).

⚠️ What stays a convention stated once and implemented twice: the diagonal a patch cell is cut along — the patch
indices of `OceanSurfaceResource` (C++) and the walk of `ocSurfaceRiseAt()` (GLSL). It is written in
`Graphics/Geometry/OceanSurface.hpp` (THE MESH); change the two together.

### Measured (2026-11-24, Linux, RTX 3070 Ti, `water-world`'s archipelago, window-less 1280 × 720, validation layer loaded)

- The map: **15 381 m² of water sheltered, of the 5.9 km² of water** in the scene's 2560 m square (1024 cells of 2.5 m a
  side, passages under 30 m closed), read and built in 54 to 73 ms on the thread that builds the scene.
- **A sheltered basin is still, the open sea is not.** Seen from 70 m above, six captures 0.6 s apart, the standard
  deviation of a pixel over time (0-255): 0.17 over the basin at (−401, −189) and 0.15 over the one at (−511, −129),
  against 15.9 over the open sea — a hundred times less. A pond joined to the sea by a narrow neck calms down just
  past its mouth.
- From its shore and at its surface the basin is flat with faint ripples and the sun's glints; an eye at its level
  sees a straight waterline (the underwater view stood against the sheltered mesh); from under it, Snell's window
  is calm.
- **The open sea is unchanged**: in a burst of 30 captures at the sea level, the frames whose instant matches the
  earlier binary's differ from it by 0.12 to 0.46 level on average (590 pixels of 921 600 off by more than 16 on the
  closest pair) — the waterline included, now read through the ocean's own GLSL. The sun's shafts add 62.2 luma above
  the water and 24.5 under it, as before.
- 0 VUID, 0 `[Error]`, no shader that fails to compile, over the launches of the series.
- **Verified on the three OS (2026-11-24, the peers, on base 7824170 / engine 94632582 / projet-alpha 7c172234).**
  macOS (Apple M6, MoltenVK, AppleClang 21; the screen-space lane alone) and Windows (RTX 3500 Ada, MSVC 14.44 /WX; both
  lanes): the cascade in Release and Debug, 0 error, 0 warning; the base suite as expected (2867 and 2864 ran, 2860
  passed on each); `water-world` with the validation layer loaded, 0 VUID, no shader that fails to compile. The log's
  shelter line gives the SAME areas on the three OS — 15 381.2 m² of 5.90067e+06 m² — in 39 to 44 ms on the M6 and
  166 ms on Windows. The five poses show what Linux shows: the veil and the surface from below, Snell's window, a
  basin that does not move (mean difference between two frames a second apart: 0.26 to 0.34 on the pond, 4 to 16 on
  the open sea), a straight waterline in the pond. ⚠️ Windows caught no frame cut by the waterline in its twelve (each
  was all air or all water): the cut along the mesh is seen on Linux and macOS only. ⚠️ On the M6 the water volume
  joins the chain about 10 s after the scene is up (the sea answers its water once it is ready for instantiation).
  NOT run: AMD (no session on the AMD laptop that day).
- NOT measured: the map's cost in the ocean's own stages (one more fetch a read); the look of the ripples from the
  owner's seat.

### ⚠️ Limits

- The map has the scene's square and `shelterMapSide` cells (1 to 4096 a side, an ocean with another count is
  refused when it loads): 2.5 m a cell for `water-world`. A pond of a few cells is
  not resolved; the passage's threshold is rounded to cells.
- `getLevelAt()` answers the level at rest, as before: the CPU knows neither the waves nor the map.
- The values tuned on a running scene (§ 2b) are not saved.
- The cost of the map in the ocean's own stages is not measured: one more fetch a read of the cascades (a vertex, a
  pixel). The underwater view's moving variant, which sums the cascades through the shared function, costs 0.064 ms
  a frame at 720p on an RTX 3070 Ti (graphics doc 13-12 § 44, the measurements).

**Code references:** `Graphics/SeaShelterMap.hpp`, `Graphics/SeaSurfaceWaves.hpp`, `Graphics/Geometry/OceanSurface.hpp`,
`Graphics/Geometry/OceanSurfaceResource.hpp/cpp`, `Graphics/Renderable/OceanResource.hpp/cpp`,
`Graphics/Renderable/AbstractSea.hpp`, `Scenes/SeaLevelInterface.hpp`, `Scenes/Scene.hpp`,
`Saphir/Generator/OceanSurfaceHelper.hpp/cpp`, `Graphics/Effects/Atmosphere/WaterVolume.cpp`; base
`VertexFactory/ShelterMap.hpp/cpp`. Engine item `ocean-sheltered-water`.
