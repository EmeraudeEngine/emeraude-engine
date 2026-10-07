---
id: mesh-shader-multiview-view-count-limit
title: A mesh-shader pipeline with more views than maxMeshMultiviewViewCount breaks cubemap shadow casting
status: open
priority: high
scope: Vulkan / Saphir / shadow mapping
opened: 2026-09-30
tags: [vulkan, mesh-shader, multiview, shadows, windows]
---

# A mesh-shader pipeline with more views than maxMeshMultiviewViewCount breaks cubemap shadow casting

## Why

Found by the Windows peer on `citadel` (NVIDIA GeForce RTX 3060 Laptop GPU, driver 616.92, SDK 1.4.357.0,
2026-09-30), not reproduced on Linux (RTX 3070 Ti):

- `VUID-VkGraphicsPipelineCreateInfo-renderPass-12325`: "`pSubpasses[0].viewMask` is 0x3f, its highest bit (5) is
  not less than `VkPhysicalDeviceMeshShaderPropertiesEXT::maxMeshMultiviewViewCount` (4)";
- then `[Error][VulkanGraphicsPipeline] Unable to create a graphics pipeline : VK_ERROR_VALIDATION_FAILED_EXT` and
  `[Error][ShaderGenerator] Unable to finalize the graphics pipeline of the program 'ShadowCasting'`;
- then EVERY frame `[Error][RenderableInstance] Unable to get ready for shadow casting !` and "Unable to get ready
  the renderable instance (Renderable:CitadelLandscapeDetailWindow') for rendering with render-target
  'SceneRenderTarget_…'" — ~184 lines/s on stderr.

A 6-view multiview (0x3f: a cubemap shadow map, one view per face — the fires' point lights) compiled with a MESH
shader stage (the landscape detail window, a mesh-shading surface). The engine never reads
`maxMeshMultiviewViewCount` (grep: 0 hits in `src/`): that limit is separate from `maxMultiviewViewCount`, and a
device may report 4.

## What remains

- Read `VkPhysicalDeviceMeshShaderPropertiesEXT::maxMeshMultiviewViewCount` with the mesh-shader features.
- When a mesh-shader program would render into a multiview target with more views than that: fall back — the
  vertex-pipeline program for that pass, or per-face passes. An architecture choice for the owner.
- The failing preparation must not be retried every frame (the stderr flood): fail once, log once.
- Check why the SCENE render target preparation also fails for that renderable (the second error line).

## References

- Vulkan spec, `VkPhysicalDeviceMeshShaderPropertiesEXT::maxMeshMultiviewViewCount`, VUID 12325.
- Engine `docs/subsystems/saphir/21-the-mesh-shading-surface.md`, graphics shadow-mapping docs.
