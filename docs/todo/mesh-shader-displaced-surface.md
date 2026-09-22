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

## What remains

- [ ] Design the renderable + the generator path (task/mesh/fragment), shadow program included.
- [ ] Handover with the POM band (no double relief, no gap).
- [ ] `relief` option 0 = 2; measure GPU cost against mode 1 at the A/B pose.
- [ ] Device without `VK_EXT_mesh_shader` (MoltenVK): the surface must fall back to POM, visibly traced.

## ⚠️ Traps

- The height map must be a HEIGHT (`src/Graphics/AGENTS.md` § Parallax Occlusion Mapping): the same
  data drives both techniques, so a photo-luminance map breaks both.
- MoltenVK has no `VK_EXT_mesh_shader` (verified 2026-09-22).
