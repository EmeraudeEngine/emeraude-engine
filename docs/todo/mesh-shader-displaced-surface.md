---
id: mesh-shader-displaced-surface
title: Real displaced micro-geometry near the camera through task + mesh shaders
status: open
priority: medium
scope: Graphics/Renderable, Saphir/Generator, Vulkan (optional mesh stage)
opened: 2026-09-22
tags: [mesh-shaders, relief, geometry]
---

# Real displaced micro-geometry near the camera through task + mesh shaders

## Why

Owner (2026-09-22): "On teste le POM contre les mesh shaders pour le micro-relief ?" — projet-alpha's
`relief` demo has options 0 (normal mapping) and 1 (parallax occlusion mapping, fixed the same day);
option 2 is this. The optional mesh stage exists (`fae9bdcb`, `docs/subsystems/saphir/20-task-and-mesh-stages.md`);
before this work no generator built a LIT, material-driven program from it.

## Owner decisions (2026-09-22, AskUserQuestion)

1. **A dedicated renderable** (a displaced surface): it describes flat patches, the task shader selects
   and culls them, the mesh shader emits the height-displaced grid. The material stays
   `StandardResource`; its fragment code is reused through `FragmentShader::connectFromPreviousShader(const MeshShader &)`.
   Rejected: generic meshlets for every geometry (far bigger), a ground-only path (a local exception).
2. **A distance cascade**: real displaced geometry close to the camera, then POM, then normal mapping.
3. **The displaced geometry feeds the shadow pass too**: flagstones shadowing each other under a grazing
   sun is exactly what POM cannot do — the differential to show.

## Owner decisions, round 2 (2026-09-22, after the generator map)

4. **An abstract per-vertex stage interface**: the material, light and shadow generators take a
   per-vertex stage instead of `VertexShader &`, and `VertexShader` and `MeshShader` both implement
   it. Chosen over "hosting" a vertex shader inside the mesh stage and over a hand-written mesh stage
   with a fixed output set. Realised with the synthesis machinery (the ~20 synthetic variables) in the
   shared base, so it is not duplicated: the two stages differ in how they DECLARE and EMIT their
   outputs (plain variables vs per-vertex arrays), not in what they synthesise.
5. **Depth sharing for the handover**: in a band [R1, R2] the geometric depth is heightScale·(1−t) and
   the POM depth heightScale·t, so the sum stays the full relief; the POM gains a fade-IN distance
   (`parallaxParameters.w`).
6. **Automatic fallback without `VK_EXT_mesh_shader`**: the same surface draws its flat grid with POM
   through the ordinary vertex path, and says so once in the log.

## Done (2026-09-22 / 23) — the design lives in the docs

The design and its contracts are in `docs/subsystems/saphir/05-the-per-vertex-stage-contract-abstractvertexstage.md`,
`docs/subsystems/saphir/20-task-and-mesh-stages.md` and `docs/subsystems/saphir/21-the-mesh-shading-surface.md`
(one task workgroup per 1 m tile, meshlets of at most 8×8 quads, the material's height map as the displacement,
skirts — only toward a coarser neighbour since 2026-09-28 —, the handover band, the fallback, frustum culling).

1. The per-vertex stage base `AbstractVertexStage` (`1a6c1e1c`; the stages diverge through OVERLOADS, not virtuals).
2. `MeshShader` implements it (`6f7d7c34`).
3. A program / pipeline without a vertex stage (`2fd7cd3a`).
4. `DisplacedGridResource` + task/mesh code + draw branch (`cf16fd26`; owner: a dedicated GEOMETRY rather than a
   renderable, SKIRTS rather than a geomorph), the displaced shadow (`1b726a39`), task-stage frustum culling and the
   shadow-map timers (`c0f31941`).
5. The handover band, `relief` option 0 = 2, the fallback, the measurements below.

## What remains

Steps 1-5 are done and cross-checked (2026-09-23): the handover band, `relief` option 0 = 2, and the fallback —
MoltenVK (Apple M2) and the Windows AMD iGPU both log the fallback exactly once and render mode 2
pixel-indistinguishable from mode 1 (inside the run-to-run noise). Only the GPU cost is left — the history below,
then the split still to measure (last bullet):

- **The GPU cost is too high: +9.6 ms of ScenePass** on an RTX 3060 Laptop (Windows, validation ON, relief spawn
  pose): ScenePass 3.91 ms in mode 1 against 13.52 ms in mode 2, the whole frame 8.73 → 17.19 ms. The draw
  launches one task workgroup per 1 m tile, 256 × 256 = 65 536 of them for the relief ground, in the colour pass
  AND the shadow pass, and every one emits at least its flat quad — none is culled. ⚠️ ScenePass never contained
  the shadow map (its own submission, untimed before 2026-09-23): these deltas are the colour pass alone.
- Re-measured 2026-09-23 with the displaced SHADOW (same RTX 3060, the new spawn and 8° sun): ScenePass mode 2
  vs mode 1 = 13.49 vs 5.37 ms with validation, **15.24 vs 4.69 ms without** — +8 to +10.5 ms; validation does not
  inflate it, and the laptop varies by ±1-2 ms run to run. The displaced shadow added nothing visible (13.52 →
  13.49) — expected, since the shadow map is not inside ScenePass. The profiler now times it on its own line.
- **Frustum culling in the task stage: DONE 2026-09-23** (`docs/subsystems/saphir/21-the-mesh-shading-surface.md`), and the
  GPU profiler now times the shadow maps (`ShadowMap/<id>`, host query reset). RTX 3070 Ti, validation OFF, relief
  spawn, 8° sun, the same settings copy for the three runs:

  | | ScenePass | ShadowMap |
  |---|---|---|
  | mode 1 (POM) | 3.41 ms | 0.04 ms |
  | mode 2, no culling | 8.70 ms | 0.56 ms |
  | mode 2, culled | 6.64 ms | 0.52 ms |

  The culling saves 2.1 ms of the colour pass; it barely moves the shadow (0.56 → 0.52), whose cost is not
  the tile count. What is left, +3.2 ms colour and +0.5 ms shadow, is the displaced geometry itself.
- Split by resolution (RTX 3070 Ti, same bench, 1281×720 against 2880×1620): mode 1 ScenePass 1.01 / 3.41 ms,
  mode 2 2.90 / 6.63 ms, the ShadowMap 0.52 ms at both. So mode 2 adds ~1.6 ms that do not depend on the
  resolution (the geometry), plus ~0.33 ms per 720p worth of pixels. ⚠️ The Windows RTX 3060 Laptop is simply
  3.7-5× slower than this card at the same resolution (mode 1 3.71 ms at 720p), and the culling did not show
  there, inside its ±2 ms run-to-run spread. A "720p makes the triangles sub-pixel" theory was checked and is
  FALSE: the cost falls with the resolution.
- ⚠️ `f93c5a0d` (single `EmitMeshTasksEXT` with a zero count instead of an early exit) was pushed as a candidate
  fix for a Windows white window, which turned out to be the shutdown hang (`shutdown-hangs-after-act-removal`,
  closed 2026-10-07, `9261fe82`: fixed by the act-unload order, projet-alpha `docs/caution-points.md` § "Unloading
  an act").
  The change stands on its own merits (one terminator, same cost, same image), not as that fix.
- **Next, not yet measured** (split the remaining cost before choosing):
  - the density — 0.004 m of quad per metre is ~5 px per quad at 1620p, and the rasteriser is inefficient
    below ~8 px per triangle;
  - the number of lit passes that regenerate the geometry — the program set holds an ambient AND a
    directional-light task/mesh pair, so each lighting pass re-runs the task and mesh stages;
  - the 32 task invocations doing the same scalar work.

## ⚠️ Traps

- The height map must be a HEIGHT (`docs/subsystems/graphics/05-5-material-ubo-system.md` § Parallax Occlusion
  Mapping (POM)): the same data drives both techniques, so a photo-luminance map breaks both.
- MoltenVK has no `VK_EXT_mesh_shader` (verified 2026-09-22).
- A POM surface cannot cast its relief's shadow at all (the shadow map sees the flat plane), so the shadow A/B
  of the three techniques is mode 2 against itself with `Core/Graphics/ShadowMapping/Enabled = false`, never
  mode 2 against mode 1.
- `relief` option 3 = 1 draws the ground in WIREFRAME (`PolygonMode::Line`): the instrument that shows the
  subdivision the task stage chose — dense near the camera, one flat quad per 1 m tile beyond the handover.
