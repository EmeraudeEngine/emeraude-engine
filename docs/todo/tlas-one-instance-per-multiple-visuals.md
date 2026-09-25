---
id: tlas-one-instance-per-multiple-visuals
title: Verify (then fix) the TLAS holding ONE instance per MultipleVisuals component — a grove traced as one tree
status: open
priority: unranked
scope: Scenes (Scene.rendering.cpp RT list, SceneMetaData TLAS build), Graphics/RenderableInstance/Multiple
opened: 2026-09-25
tags: [ray-tracing, tlas, vegetation, instancing]
---

# Verify (then fix) the TLAS holding ONE instance per MultipleVisuals component — a grove traced as one tree

## Why

Found by READING during the leaf-translucency design (2026-09-25), NOT measured. The RT list makes ONE batch per
renderable component (`Scene.rendering.cpp` ~1685, `traceFrom` → `RenderBatch::create(... m_RTOpaque*List ...)`),
and `SceneMetaData` emits one TLAS instance per batch (`SceneMetaData.cpp` ~374-464, `instances.emplace_back` at
~464). The per-instance frames of `RenderableInstance::Multiple` (`Multiple.hpp` ~67-134) are read nowhere on that
path, and nothing in `Multiple.cpp` / `MultipleVisuals.cpp` mentions ray tracing. If confirmed, every
`MultipleVisuals` cell — the `forest` groves (`Forest.cpp` ~301) and the `terrain` cells (`Terrain.cpp` ~610) — is
traced as ONE tree at the entity origin, and no traced-lane measurement on those demos means anything (RTGI, RTR,
RTAO, RTContactShadows, the probes). Owner decision (2026-09-25): verify by measurement BEFORE the translucency work.

## What remains

1. Measure: in `forest`, look through a grove with the traced lane and compare an RTR reflection or an RTAO pattern
   against the raster; count the TLAS instances (a debug trace in `SceneMetaData`) against the cell's instance count.
2. If confirmed: emit one TLAS instance per frame of a `Multiple` instance (same BLAS, per-instance transform), with
   a budget (the TLAS distance already bounds it) — measure the TLAS build cost on `terrain` (~690 000 trees).

## ⚠️ Traps

- Imposters are excluded from the TLAS by design (`disableRayTracing()`): only the mesh range (≤ 250 m on `terrain`)
  would be multiplied.
- The BLAS holds REST-POSE triangles (owner decision 2026-09-21): the wind never reaches the traced lane.
