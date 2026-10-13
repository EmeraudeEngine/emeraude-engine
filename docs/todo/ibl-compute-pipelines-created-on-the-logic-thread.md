---
id: ibl-compute-pipelines-created-on-the-logic-thread
title: The IBL baker's two compute pipelines are created on the logic thread (P3)
status: open
priority: medium
scope: Graphics/Compute/IBLBaker, Scenes/Scene (updateEnvironmentIBL), Graphics/Renderer (initialization)
opened: 2026-10-13
tags: [macos, moltenvk, stall, threads, pipeline]
---

# The IBL baker's two compute pipelines are created on the logic thread (P3)

## Why

Two COMPUTE pipelines are created on the LOGIC thread itself, right after "Scene will use environment cubemap …":
124 and 144 ms on the macOS M2 (2026-10-03), 88–97 ms on the Mac mini M6 on a cold miss (2026-10-12), reported as
"an unlabelled pipeline" by `reportPipelineCreation()` (thread ID = the logic thread). A logic tick that long is a
visible hitch; on NVIDIA (Linux) the same creations take under 1 ms.

The path, by reading (2026-10-13): `Scene::processLogics()` (`src/Scenes/Scene.cpp:401`) → `updateEnvironmentIBL()`
→ `IBLBaker::bakeEnvironment()` (`src/Scenes/Scene.lighting.cpp:337`) → `IBLBaker::ensureEnvironmentPipelines()`
(`src/Graphics/Compute/IBLBaker.cpp:608`, called at `:729`), which creates `m_prefilterPipeline` and
`m_irradiancePipeline` lazily (`:671`, `:674`) the first time an environment is baked. The baker itself is built at
renderer initialization (`src/Graphics/Renderer.cpp:431`, where the BRDF LUT is baked).

Split out of `logic-stalls-on-runtime-pipeline-compile` (2026-10-13; it was P3 of the M6 diagnosis).

## What remains

- Take the creation off the logic thread. Two candidates, for the owner: (a) pre-create the two environment pipelines
  at renderer initialization, beside the BRDF LUT bake (the shaders are fixed sources, `PrefilterShaderSource` and
  `IrradianceShaderSource`: nothing scene-dependent); (b) create them on a worker and let `updateEnvironmentIBL()` retry until
  ready. (a) is the simpler and moves the cost to start-up, where the pipeline cache and the loading screen cover it.
- The bake's own GPU work stays where it is; only the pipeline creation moves.
- Proof: macOS with a cold Metal cache, no `Slow pipeline creation` on the logic thread, the logic max under the
  current figures; IBL output unchanged (same capture on Linux).

## ⚠️ Traps

- Linux cannot show the cost (NVIDIA under 1 ms): measure on macOS with the Metal shader cache moved aside, or with
  a temporary delay in `ComputePipeline::createOnHardware()` (the 2026-10-03 method).
- The two pipelines print "an unlabelled pipeline" until `label-effect-and-compute-pipelines` is done: identify them
  by the thread ID and the preceding "Scene will use environment cubemap" line.

## References

- `logic-stalls-on-runtime-pipeline-compile` (P1, the render-thread side; history and measurements),
  `label-effect-and-compute-pipelines` (P4), `renderer-cache-pruning` (P2 step 2).
