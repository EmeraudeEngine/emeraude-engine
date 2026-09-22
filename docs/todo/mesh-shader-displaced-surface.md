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
4. `DisplacedSurfaceResource` + task/mesh code + draw branch + shadow program.
5. The handover band, the fallback, `relief` option 0 = 2, measurements.

## The concrete design (2026-09-22, implementing steps 3 + 4 together — step 3 alone cannot run)

- **One renderable covers the WHOLE ground**, near and far, in one `drawMeshTasks`: the ground is cut into
  1 m tiles; one TASK workgroup per tile picks its subdivision from the camera distance and emits its
  meshlets; one MESH workgroup = one meshlet of at most 8×8 quads (81 vertices, 128 triangles). Far tiles
  emit a single quad (the ground stays flat there and the material's POM / normal map does the relief),
  near tiles emit up to 16×16 meshlets (128×128 quads per metre, ~8 mm). No hole, no z-fight with a
  second ground.
- **Crack-free by geomorph** — the CDLOD rule already used by the terrain (`Geometry/HeightfieldSurface`):
  every vertex morphs toward the next coarser lattice over the last part of its level's range, as a
  function of its own world distance, so two tiles sharing an edge agree on it.
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

- [ ] Design the renderable + the generator path (task/mesh/fragment), shadow program included.
- [ ] Handover with the POM band (no double relief, no gap).
- [ ] `relief` option 0 = 2; measure GPU cost against mode 1 at the A/B pose.
- [ ] Device without `VK_EXT_mesh_shader` (MoltenVK): the surface must fall back to POM, visibly traced.

## ⚠️ Traps

- The height map must be a HEIGHT (`src/Graphics/AGENTS.md` § Parallax Occlusion Mapping): the same
  data drives both techniques, so a photo-luminance map breaks both.
- MoltenVK has no `VK_EXT_mesh_shader` (verified 2026-09-22).
