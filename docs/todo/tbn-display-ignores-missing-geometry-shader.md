---
id: tbn-display-ignores-missing-geometry-shader
title: TBN-space display is built on a device without geometryShader (MoltenVK) — 1848 VUIDs and a shutdown race
status: open
priority: high
scope: Graphics/RenderableInstance
opened: 2026-09-27
tags: [vulkan, validation, macos, moltenvk]
---

# TBN-space display is built on a device without geometryShader (MoltenVK) — 1848 VUIDs and a shutdown race

## Why

Reported by the macOS peer (Apple M2, MoltenVK, validation ON) on 2026-09-27 while testing the typed
console commands, on `--load-demo coordinates-debug`, which enables the TBN-space display
(`projet-alpha src/Builtin/CoordinatesDebug.cpp:84` `enableTBNSpaceRendering(true)` and `:92`
`enableDisplayTBNSpace(true)` per instance):

- The engine itself says at startup `The physical device 'Apple M2' does not advertise
  'geometryShader': not requested, TBN space display unavailable.` (`src/Vulkan/Instance.cpp`).
- Yet `RenderableInstance::Abstract` builds the TBN program anyway, on every attempt
  (`src/Graphics/RenderableInstance/Abstract.cpp:984` and `:1023`, generator
  `src/Saphir/Generator/TBNSpaceRendering.cpp` — a GEOMETRY stage): 308 × "Unable to create the
  shader module from the shader 'TBNSpaceRenderingGeometryShader'", with
  `VUID-VkShaderModuleCreateInfo-pCode-08740` ×616 (Geometry capability without the feature) and
  `VUID-RuntimeSpirv-Location-06272` ×1232 (`maxGeometryInputComponents (0)`).
- At shutdown: `UNASSIGNED-Threading-MultipleThreads-Write` — `vkDestroyDevice` on the main thread
  while a worker thread is still inside `vkCreateShaderModule` (one of those retries). The failing
  retries make a device-destruction race visible; the race itself is not TBN-specific.

Re-seen on the Mac mini M6 (MoltenVK 1.4.1, 2026-10-10, macOS-PA): 100 VUIDs in the sweep window, 5 kinds × 20 —
`VUID-VkShaderModuleCreateInfo-pCode-08740`, `VUID-RuntimeSpirv-Location-06272`,
`VUID-VkPipelineShaderStageCreateInfo-stage-00704`, `-stage-00714`, `-stage-00715` — "Unable to generate the TBN space
program !", the demo still renders, exit 0. The M6 has no geometry stage either.

## What remains

1. Gate the TBN-space program on the device feature (one place, before any generation attempt): no
   geometry shader ⇒ the display is unavailable, said ONCE, and nothing is retried per instance.
2. Find why a shader-module creation can still be running on a worker thread when the device is
   destroyed, and make the teardown wait for (or cancel) those tasks.
3. Re-run `coordinates-debug` on macOS: 0 VUID, 0 UNASSIGNED expected.

## ⚠️ Traps

- Linux/NVIDIA and Windows advertise `geometryShader`: the defect is invisible there. Only a device
  without it (MoltenVK, some mobile-class GPUs) reproduces it.
- The retry happens per instance and per attempt: fixing only the log noise (trace once) without the
  gate leaves the 308 failing compilations and the race.

## References

- macOS peer report, 2026-09-27 (engine `82063d97`, alpha `c191de9`).
