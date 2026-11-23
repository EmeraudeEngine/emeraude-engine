---
id: terrain-rt-proxy-coarser-than-the-drawn-surface
title: A terrain's ray-tracing proxy coarser than the drawn surface blackens the hollows near the camera
status: in-progress
priority: high
scope: Graphics/Geometry (CDLODTerrainResource), Graphics/Effects/Lighting (RTContactShadows, RTAO, RTGI)
opened: 2026-11-23
tags: [terrain, cdlod, ray-tracing, proxy, contact-shadows, rtao]
---

# A terrain's ray-tracing proxy coarser than the drawn surface blackens the hollows near the camera

## Why

Owner report, 2026-11-23, projet-alpha `water-world` (the archipelago, the default ground): close to the ground,
darker patches appear in the hollows of the sand — grey lobes with comb-shaped edges around a brown core — and they
vanish when the camera rises.

**Mechanism.** `CDLODTerrainResource::generateRayTracingProxy()` builds ONE surface for the whole proxy window, at the
finest power-of-two step whose triangle count fits `Core/Graphics/RayTracing/TerrainBLASMaxTriangles` (2 000 000).
The archipelago is 2048 cells of 1.25 m: step 1 is 8 388 608 triangles, step 2 is 2 097 152 (4.9 % over the budget),
so the proxy takes **step 4 — cells of 5 m against a drawn surface of 1.25 m**
(`ray-tracing proxy of 2560 m around (0, 0), step 4 (524288 triangles, budget 2000000)`). In a hollow the 5 m chord
passes ABOVE the drawn ground: on a surveyed 80 m block of dry sand the drawn nodes sit under the proxy by up to
**0.58 m** (43 % of them by more than 5 mm, 22 % by more than 10 cm; at step 2: 0.18 m at most).

The traced effects start their rays from the DRAWN surface (the depth buffer), lifted along the normal by a bias that
grows with the camera distance: `normalBias × max(1, distance) × min(1 / N·V, 10)` — 1 cm per metre for
`RTContactShadows`, 5 mm per metre for `RTAO`. Close to the camera that lift is smaller than the gap: the ray starts
INSIDE the proxy, hits the underside of its triangle at once (every TLAS instance carries
`VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR`, no effect culls a face) and the pixel loses its sun
(contact shadows: the grey lobes) and, deeper, its sky (RTAO, RTGI: the brown core). The comb-shaped edges are the
sand's normal map moving the lift from ripple to ripple.

**Why it vanishes from above**: the lift grows with the distance until it clears the gap, and both effects fade with
the camera distance (`RTContactShadows` reaches no shadow at `maxDistance × 10` = 20 m, `RTAO` at `maxDistance × 20` =
40 m). It is NOT the CDLOD joining the proxy: its finest level reaches 384 m (`detailDistance`).

This is the residue announced in `docs/caution-points.md` § CDLOD terrain ("at step > 1 the proxy cannot match the
drawn 1 m surface at all") — the same family as the diagonal defect fixed on 2026-09-27, by the same contract: the
traced surface must be the rasterized one. By reading (not bisected at runtime), the archipelago has had this step
since it became the default on 2026-10-26: the step logic dates from 2026-09-22.

## Measurements (2026-11-23)

Linux, RTX 3070 Ti, Release, window-less test instance (1280 × 720), validation layer loaded, 0 `VUID-`, exposure
pinned (f/11, 1/250 s, ISO 100), the owner's settings (`LightingLane = Auto`). Metric: pixels of the ray-traced
capture darker than the screen-space capture of the SAME launch and pose by more than 12/255 (luma), on the rows
300–719 (537 600 pixels) — the grove is not seeded, so a launch is only compared with itself.

Target (−547.5, 3.78, −218.0). Eyes: p1 (−538.5, 13.01, −209.0), 12 m away; p2 (−543.5, 7.28, −214.0), close, the
owner's kind of view; p3 (−541.5, 38.78, −212.0), 35 m up.

| Proxy step (budget) | Cell | Triangles | p1 | p2 | p3 | Process VRAM |
|---|---|---|---|---|---|---|
| 4 (2 000 000, the default) | 5 m | 524 288 | 11 083 | **249 929** (46.5 %) | 2 372 | 1 095 MiB |
| 2 (2 100 000) | 2.5 m | 2 097 152 | 236 | 79 183 (14.7 %) | 3 262 | 1 257 MiB |
| 1 (8 400 000) | 1.25 m | 8 388 608 | 463 | **0** | 3 129 | 1 941 MiB |

- The residue at p1 (steps 2 and 1) and at p3 (every step) is the sea's moving foam, not sand: near-white pixels in
  the reference (mean RGB about 170–205 on the three channels), 7 to 70 sand-coloured ones among them.
- The mesh-shading detail window was enabled in every launch (`mesh-shading detail window enabled`): it is not the
  cause — step 1 is clean with it.
- Which effect, at p1, step 4, one launch (10 930 patch pixels): `AmbientOcclusion` off → 9 165; `ContactShadows` off →
  **781**; both lanes back on → 10 909 (the control). Mean luma inside the patch mask: 79.9 traced, 104.4 screen-space,
  108.5 traced without the contact shadows.
- The step-2 launch ran with `--disable-cef` (two launches with CEF died in the browser library's known
  memory-dump trap before the captures; nothing to do with the terrain).

### What a re-centre costs (2026-11-23)

A proxy with a level of detail follows the camera, and a replaced proxy's BLAS is rebuilt SYNCHRONOUSLY on the frame
path (`Scenes::SceneMetaData` sees the stale flag, `Interface::buildAccelerationStructure()` →
`AccelerationStructureBuilder::buildBLAS()` submits and waits for its fence). Today the archipelago's proxy never
re-centres (it spans the whole map). Measured with a temporary probe around `buildBLAS()` (removed), at the scene's
load, ONE sample per size, same machine:

| Proxy triangles | Synchronous BLAS build | CPU generation (worker, the log's `built in`) |
|---|---|---|
| 524 288 | 4.5 ms | 37 ms |
| 2 097 152 | 8.2 ms | 125 ms |
| 8 388 608 | 54.0 ms | 465 ms |

Not measured: the same build during a frame in motion (the GPU was loading the scene), and the upload.

## Done on 2026-11-23 (emeraude-base and the engine, `develop`)

Owner decisions of the day: a level of detail in the proxy, centred on the camera; the triangulation in
emeraude-base, proven by unit tests; the BLAS rebuild stays synchronous, measured in motion; the sizes are two
terrain parameters (128 m of half-side, a rebuild every 64 m); the rest of the proxy keeps its step.

- emeraude-base: `VertexFactory::buildNestedLattice()` / `countNestedLattice()` (`src/VertexFactory/NestedLattice.*`),
  5 tests (`test_VertexFactoryNestedLattice.cpp`) — the watertightness check FAILS on a triangulation without the fan
  (8 nested cases) and passes with it; Release and ASan / UBSan.
- Engine: `CDLODTerrainParameters::rayTracingProxyDetailRadius` / `rayTracingProxyDetailSlack`,
  `CDLODTerrainResource::planRayTracingProxy()`, `rayTracingProxyDetailCenter()`, the generation and the move
  decision; `RTContactShadows::FadeRangeFactor`, `RTAO::FadeRangeFactor`.
- Measured, same poses and metric as above, default budget: **p1 717, p2 0, p3 2 640** (654 912 triangles, 1 103 MiB,
  0 `VUID-`); the residue is the sea's foam (14 and 53 sand-coloured pixels). A 600 000 budget on the same binary
  leaves the window out with its warning: p1 11 900, p2 249 923, p3 1 755.
- In motion (a temporary probe, removed): 500 m at 50 m/s, 7 rebuilds, 3.9–4.7 ms of synchronous BLAS build each;
  0 rebuild standing still, 0 drifting outside the map, 3 along the border inside it; 13 builds, median 4.1 ms,
  7.5 ms at most.
- `terrain`: loads with three nested windows (820 736 triangles, 94 ms), 0 `VUID-`; no before / after there (the
  block surveyed near its origin sinks under a step-8 proxy by 8 cm at most).
- `gate.py commit`: every step OK (GCC Release and clang builds 0 error 0 warning, base suite 2858/2858, clang-tidy
  28 → 0 on the changed lines, cppcheck 0, include-what-you-use clean); base suite under ASan / UBSan 2858/2858,
  0 report; the GCC Debug build of the cascade 0 error 0 warning. The ledgers hold what was fixed.
- **Confirmed by the owner on the screen the report came from** (2880 × 1620, a window), the same day, on a build
  of these changes: "the dark patches are gone".
- The lasting knowledge is in `docs/caution-points.md` § CDLOD terrain and graphics doc 17-15b.

## What remains

1. **macOS and Windows** (the peers): the builds and the base tests; the proxy's log line; on a machine with the
   ray-traced lane, the poses above in both lanes.
2. Then delete this file.

## ⚠️ Traps

- **Bisect by lane first**, then by slot (`PostProcess.setLightingMode(ScreenSpace)`, `PostProcess.disable(slot)` /
  `select(slot, effect)` on the same instance): a patch only the ray-traced lane draws is a traced-versus-drawn
  mismatch.
- The budget is read when the terrain loads: a budget A/B needs a settings copy and a relaunch.
- Where the gap is can be computed without a picture: `Core.SceneManagerService.getGroundLevel(x, z)` on every drawn
  node, against the same nodes taken every `step` cells and interpolated along the proxy's diagonal
  ((x, z)–(x + 1, z + 1)).
- The palms are planted at random: compare the two lanes inside ONE launch, never two launches.

## References

- `docs/caution-points.md` § CDLOD terrain (the diagonal defect of 2026-09-27, the budget).
- `src/Graphics/Geometry/CDLODTerrainResource.cpp` (`generateRayTracingProxy()`), `src/SettingKeys.hpp`
  (`TerrainBLASMaxTriangles`), `src/Scenes/SceneMetaData.cpp` (the TLAS instance flags),
  `src/Graphics/Effects/Lighting/RTContactShadows.cpp`, `RTAO.cpp` (the bias and the fade).
- projet-alpha `docs/subsystems/builtin/16-14-waterworld-scene.md`.
