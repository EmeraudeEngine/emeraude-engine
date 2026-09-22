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
3. Program/pipeline without a vertex stage (vertex format, empty vertex input, push-constant and
   descriptor-layout stage flags).
4. `DisplacedSurfaceResource` + task/mesh code + draw branch + shadow program.
5. The handover band, the fallback, `relief` option 0 = 2, measurements.

## What remains

- [ ] Design the renderable + the generator path (task/mesh/fragment), shadow program included.
- [ ] Handover with the POM band (no double relief, no gap).
- [ ] `relief` option 0 = 2; measure GPU cost against mode 1 at the A/B pose.
- [ ] Device without `VK_EXT_mesh_shader` (MoltenVK): the surface must fall back to POM, visibly traced.

## ⚠️ Traps

- The height map must be a HEIGHT (`src/Graphics/AGENTS.md` § Parallax Occlusion Mapping): the same
  data drives both techniques, so a photo-luminance map breaks both.
- MoltenVK has no `VK_EXT_mesh_shader` (verified 2026-09-22).
