---
id: label-effect-and-compute-pipelines
title: Label the effects' and the compute pipelines in the slow-creation report (P4)
status: open
priority: medium
scope: Vulkan/ComputePipeline, Vulkan/GraphicsPipeline, Graphics/Effects (every pipeline built outside Saphir)
opened: 2026-10-13
tags: [diagnostics, pipeline, macos]
---

# Label the effects' and the compute pipelines in the slow-creation report (P4)

## Why

`reportPipelineCreation()` names a slow pipeline (≥ 50 ms) by a label kept in Release, so a stall can be traced to its
pass. Only the Saphir generator sets one (`src/Saphir/Generator/Abstract.cpp:192`, `graphicsPipeline->setLabel(this->name())`,
e.g. `RenderableInstanceSpotLightPassFull`). Everything else prints "an unlabelled pipeline" (`src/Vulkan/Utility.cpp:295`):
- `src/Vulkan/ComputePipeline.cpp:89` passes an EMPTY label (`std::string{}`): every compute pipeline is unlabelled
  (`ComputePipeline` has no label at all);
- `src/Vulkan/GraphicsPipeline.cpp:1086` passes `m_label`, which stays empty for an effect's own pipeline (built
  directly, not through a Saphir generator).

The macOS cold-cache runs printed 46 "an unlabelled pipeline" lines: those stalls could not be attributed
(the two on the logic thread are item `ibl-compute-pipelines-created-on-the-logic-thread`).

Split out of `logic-stalls-on-runtime-pipeline-compile` (2026-10-13; it was P4 of the M6 diagnosis).

## What remains

- Give `ComputePipeline` a label (the same `setLabel()` / `m_label` contract as `GraphicsPipeline`) and pass it at
  `ComputePipeline.cpp:89`.
- Label every pipeline built outside Saphir at its creation site — the compute ones today: `IBLBaker`, `CDLODTerrainResource`,
  `SkinnedGeometryProcessor`, `IrradianceProbeVolume`, `DeferredLightResolve`, `OceanWaves`, `VideoFrameConverter`,
  `OverflowCensus`, `XRayAnalyzer`, the `SSR` / `RTR` effects — and every effect's own graphics pipeline: a stable name
  of the effect and its pass.
- Proof: a macOS cold-cache run (or a Linux run with the report threshold at 0) prints no "an unlabelled pipeline".

## ⚠️ Traps

- The label must survive Release (it is the report's whole value on a peer machine): not a debug-only object name.

## References

- `logic-stalls-on-runtime-pipeline-compile` (P1, where the report is described),
  `ibl-compute-pipelines-created-on-the-logic-thread` (P3), `renderer-cache-pruning` (P2 step 2).
