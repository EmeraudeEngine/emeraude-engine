---
id: mesh-shader-displaced-surface
title: Real displaced micro-geometry near the camera through task + mesh shaders
status: in-progress
priority: unranked
scope: Graphics/Renderable, Saphir/Generator, Vulkan (optional mesh stage)
opened: 2026-09-22
tags: [mesh-shaders, relief, geometry]
---

# Real displaced micro-geometry near the camera through task + mesh shaders

## Why

Owner (2026-09-22): "On teste le POM contre les mesh shaders pour le micro-relief ?" — projet-alpha's
`relief` demo has options 0 (normal mapping) and 1 (parallax occlusion mapping, fixed the same day);
option 2 is this. The optional mesh stage exists (`fae9bdcb`: `Vulkan/AGENTS.md`, `Saphir/AGENTS.md`),
but no generator builds a LIT, material-driven program from it.

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

## Plan

1. ~~Extract the per-vertex stage base~~ — DONE 2026-09-22 (`AbstractVertexStage`, 410 generated
   sources byte-identical; `src/Saphir/AGENTS.md` § The per-vertex stage contract). Owner, same day:
   the stages diverge through OVERLOADS, not virtuals.
2. ~~`MeshShader` implements it~~ — DONE 2026-09-22: per-vertex loop + provided attributes + array outputs at
   the same locations + the light block through a local structure; per-primitive outputs dropped (unused).
   `src/Saphir/AGENTS.md` § Task and mesh stages. Vertex path re-proven byte-identical; the mesh path is
   exercised for the first time at step 4.
3. ~~Program/pipeline without a vertex stage~~ — DONE 2026-09-22 (inert on the vertex path):
   `Program::perVertexStage()` / `perVertexStageFlags()`, the `was*Enabled()` queries through it, no vertex
   buffer format and no vertex input / input assembly for a mesh program, push-constant ranges from the
   per-vertex stage (MESH | TASK for a mesh program, VERTEX unchanged otherwise), and
   `Device::meshShadingStages()` ORed into the view, instance-transforms, light and material layouts (the
   material's height sampler included).
4. `DisplacedGridResource` + task/mesh code + draw branch — COLOUR PASSES DONE 2026-09-23: relief option 0 = 2
   renders on the RTX 3070 Ti, 0 VUID, real geometry near the camera and POM beyond (docs:
   `src/Saphir/AGENTS.md` § The mesh-shading surface). Owner decisions of the day: a dedicated GEOMETRY rather
   than a renderable, and SKIRTS rather than a geomorph against the cracks. SHADOW PROGRAM DONE 2026-09-23
   (`ShadowCasting::generateMeshShadingStages()`, subdivided for the MAIN camera like a heightfield's levels):
   under an 8° sun, switching `Core/Graphics/ShadowMapping/Enabled` off brightens 20 % of the near-ground pixels by
   more than 10/255 (mean 109.5 → 115.8). The level flagstones shadow the JOINTS, not each other's tops: their tops
   all sit at the ground plane, only the joints are displaced down. FRUSTUM CULLING DONE the same day; the GPU cost
   remains (below).
5. The handover band, the fallback, `relief` option 0 = 2, measurements.

## The concrete design (2026-09-22, implementing steps 3 + 4 together — step 3 alone cannot run)

- **One renderable covers the WHOLE ground**, near and far, in one `drawMeshTasks`: the ground is cut into
  1 m tiles; one TASK workgroup per tile picks its subdivision from the camera distance and emits its
  meshlets; one MESH workgroup = one meshlet of at most 8×8 quads (81 vertices, 128 triangles). Far tiles
  emit a single quad (the ground stays flat there and the material's POM / normal map does the relief),
  near tiles emit up to 16×16 meshlets (128×128 quads per metre, ~8 mm). No hole, no z-fight with a
  second ground.
- ~~Crack-free by geomorph~~ — SUPERSEDED by the owner's decision of 2026-09-23: SKIRTS (a vertical strip down to
  the deepest relief along each tile edge, both windings), which is what is implemented.
- **The displacement is the MATERIAL's height map** (same texture, same `heightScale`, same UV transform as
  the POM — one relief, two techniques): `depth = (1 − h) · heightScale · repeatSize`, scaled by the
  handover factor. The frame stays the flat ground's (the normal map carries the lighting relief, as in
  modes 0 and 1).
- **Handover (owner decision 5)**: geometric depth · (1 − t) + POM depth · t, t = smoothstep(R1, R2, d); the
  POM gains a fade-IN pair in the material UBO. Beyond R2 the ordinary POM fade-out (8 → 18 m) continues.
- **Fallback (decision 6)**: without `VK_EXT_mesh_shader` the same renderable draws its flat grid through
  the vertex path with POM, and says so once.
- **Program/pipeline without a vertex stage (step 3)**: `Program::perVertexStage()` (the vertex OR mesh
  stage) answers the `was*Enabled()` queries; no vertex buffer format for a mesh program; empty vertex
  input; push-constant ranges and descriptor-set layouts (view, material, light, instance transforms) gain
  `TASK | MESH` where the mesh path reads them.

## What remains

Steps 1-4 and the step-5 functions are done and cross-checked (2026-09-23): the handover band, `relief` option
0 = 2, and the fallback — MoltenVK (Apple M2) and the Windows AMD iGPU both log the fallback exactly once and
render mode 2 pixel-indistinguishable from mode 1 (inside the run-to-run noise). Left:

- **The GPU cost is too high: +9.6 ms of ScenePass** on an RTX 3060 Laptop (Windows, validation ON, relief spawn
  pose): ScenePass 3.91 ms in mode 1 against 13.52 ms in mode 2, the whole frame 8.73 → 17.19 ms. The draw
  launches one task workgroup per 1 m tile, 256 × 256 = 65 536 of them for the relief ground, in the colour pass
  AND the shadow pass, and every one emits at least its flat quad — none is culled. ⚠️ ScenePass never contained
  the shadow map (its own submission, untimed before 2026-09-23): these deltas are the colour pass alone.
- Re-measured 2026-09-23 with the displaced SHADOW (same RTX 3060, the new spawn and 8° sun): ScenePass mode 2
  vs mode 1 = 13.49 vs 5.37 ms with validation, **15.24 vs 4.69 ms without** — +8 to +10.5 ms; validation does not
  inflate it, and the laptop varies by ±1-2 ms run to run. The displaced shadow added nothing visible (13.52 →
  13.49) — expected, since the shadow map is not inside ScenePass. The profiler now times it on its own line.
- **Frustum culling in the task stage: DONE 2026-09-23** (`src/Saphir/AGENTS.md` § The mesh-shading surface), and the
  GPU profiler now times the shadow maps (`ShadowMap/<id>`, host query reset). RTX 3070 Ti, validation OFF, relief
  spawn, 8° sun, the same settings copy for the three runs:

  | | ScenePass | ShadowMap |
  |---|---|---|
  | mode 1 (POM) | 3.41 ms | 0.04 ms |
  | mode 2, no culling | 8.70 ms | 0.56 ms |
  | mode 2, culled | 6.64 ms | 0.52 ms |

  The culling saves 2.1 ms of the colour pass; it barely moves the shadow (0.56 → 0.52), whose cost is not
  the tile count. What is left, +3.2 ms colour and +0.5 ms shadow, is the displaced geometry itself.
- **Next, not yet measured** (split the remaining cost before choosing):
  - the density — 0.004 m of quad per metre is ~5 px per quad at 1620p, and the rasteriser is inefficient
    below ~8 px per triangle;
  - the number of lit passes that regenerate the geometry — the program set holds an ambient AND a
    directional-light task/mesh pair, so each lighting pass re-runs the task and mesh stages;
  - the 32 task invocations doing the same scalar work.

## ⚠️ Traps

- The height map must be a HEIGHT (`src/Graphics/AGENTS.md` § Parallax Occlusion Mapping): the same
  data drives both techniques, so a photo-luminance map breaks both.
- MoltenVK has no `VK_EXT_mesh_shader` (verified 2026-09-22).
- A POM surface cannot cast its relief's shadow at all (the shadow map sees the flat plane), so the shadow A/B
  of the three techniques is mode 2 against itself with `Core/Graphics/ShadowMapping/Enabled = false`, never
  mode 2 against mode 1.
- `relief` option 3 = 1 draws the ground in WIREFRAME (`PolygonMode::Line`): the instrument that shows the
  subdivision the task stage chose — dense near the camera, one flat quad per 1 m tile beyond the handover.
