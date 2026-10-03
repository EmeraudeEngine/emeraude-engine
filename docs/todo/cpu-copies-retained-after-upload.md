---
id: cpu-copies-retained-after-upload
title: Host RAM — geometry shapes and image pixmaps stay resident after their GPU upload (citadel 7.4 GB private)
status: open
priority: unranked
scope: Graphics/Geometry (IndexedVertexResource and siblings), Graphics/ImageResource, projet-alpha TreeStock
opened: 2026-10-03
tags: [memory, diagnostics, windows]
---

# Host RAM — geometry shapes and image pixmaps stay resident after their GPU upload

## Why

On the Windows laptop (16 GB installed, 15.2 GB visible, ~6.5 GB free with the owner's IDEs open), every run of
`citadel` or `terrain` drives the system to the edge: Claude Code killed three background jobs for "critically low
memory" on 2026-10-01..03 (a citadel ramp-jump batch, two builds). Measured 2026-10-03, NVIDIA RTX 3060 Laptop,
validation OFF, `--disable-cef`, projet-alpha `main` / engine `develop` (669353ae):

| Launch | Private bytes | Working set | System free |
|---|---|---|---|
| No demo (engine only) | 1.46 GB | 1.36 GB | 4.8 GB |
| `collision-debug` | 2.06 GB | 1.64 GB | 4.6 GB |
| `forest` | 4.84 GB | 3.14 GB | 3.2 GB |
| `citadel` | 7.40 GB | 5.10 GB | 1.3 GB |
| `terrain` | 9.66 GB | 2.9 GB (paged out) | 0.6 GB at worst |

- The memory of `citadel` climbs from 1 to 7 GB during the ~25 s of scene construction, in step with the tree growth
  ("stock grown in 18832.7 ms"), and NEVER comes back down: it is retained, not a transient peak.
- `forest` (the same `TreeStock`, no castle) costs ~3.3 GB above the engine alone: the procedural trees are the
  largest single contributor.
- The GPU's shared (system) memory used by the process is only 0.29 GB (`forest`) to 0.78 GB (`citadel`,
  `terrain`): the bulk is CPU-side data.
- For scale: one `cl.exe` on a heavy engine TU peaks at 0.50-0.59 GB (Scene.cpp, Citadel.cpp, Renderer.cpp), so
  `-j 4` is the safe build parallelism on this machine.

What citadel generates: the tree stock of 12 procedural trees (`TreeStock::grow`, 3 seeds per species, 6 levels of
detail): about 1.74 M triangles at LOD 0 (Broadleaf 287 622 each, Conifer ~150 924, Aspen 119 709, Colonized
15-28 k), plus 5 coarser LODs and the imposter atlases, instanced 3255 times; a 1024 x 1024 terrain and its
524 288-triangle ray-tracing proxy; ~10 shadowed point lights; the material textures.

## Two retention mechanisms found in the code (not yet quantified separately)

1. **A geometry keeps its whole `Shape` in RAM after the upload.** `IndexedVertexResource` (and the sibling
   resources: `VertexResource`, `VertexGridResource`, ...) holds `m_localData`, cleared only in
   `destroyFromHardware(clearLocalData)` (`src/Graphics/Geometry/IndexedVertexResource.cpp:168`). In emeraude-base,
   a `ShapeVertex< float >` is 92 bytes (position, tangent, normal, two UV sets, influences, weights, handedness),
   a `ShapeTriangle` about 48 bytes, and the `Shape` also keeps its construction hash maps (`m_vertexIndex`,
   `m_vertexColorIndex`, `m_unpairedEdges`, `Shape.hpp:2451-2456`). Leaf cards have unique vertices, so a tree
   costs roughly 2 x 92 + 48 bytes per triangle plus the maps: ~1 GB for the citadel stock is the order of
   magnitude (an ESTIMATE).
2. **An image keeps its decoded pixmap.** `ImageResource::m_pixmap` (`src/Graphics/ImageResource.hpp:344`) is only
   cleared when the validation fails (`src/Graphics/ImageResource.cpp:112`): every material texture stays decoded as
   8-bit RGBA on the CPU beside its GPU copy (64 MB for a 4K image).

Not yet explained: the rest of the ~3.3 GB of the trees and of citadel's total. Suspects, unproven: the tree
generator's intermediates (skeleton, `TreeMesh`) kept by `TreeStock`, and the Windows heap keeping the high-water
mark of the construction (freed blocks not decommitted).

## What remains

1. **Quantify, locally, never committed:** clear `m_localData` and `m_pixmap` right after a successful upload, rebuild,
   remeasure `forest` / `citadel` / `terrain` private bytes (same table). That gives the real gain of each mechanism.
2. **Find who still reads the CPU copies after the upload** before any real change: the ray-tracing BLAS build, the
   physics (static triangle meshes, P5), bounding volumes, LOD / shadow LOD selection, the editor, a device-loss
   rebuild (`destroyFromHardware(false)` then `createOnHardware()` needs the shape). Each reader must either copy what it
   needs or the release must wait for it.
3. **Owner decision (architecture): the retention rule.** Options: (a) release the CPU copy after upload by default,
   with an opt-in "keep local data" flag per resource; (b) keep it but shrink it (drop the construction hash maps once
   the shape is final, a compact vertex format for GPU-only meshes); (c) keep the current rule and accept the cost.
4. **Observability:** a memory census console command (per resource type: shape bytes, pixmap bytes, count), so the
   next measurement does not need a local patch. Pairs with `vram-budget-observability` (the GPU side).

## ⚠️ Traps

- Windows `Get-Process` private bytes include memory the heap no longer uses but has not decommitted: a fix can free
  blocks without the number dropping. Measure the peak AND the value after the scene settles, and compare with a
  heap-level count (the census) when available.
- PowerShell prints decimals with a comma on this machine (fr-FR): convert before any numeric test.
- Do not run citadel / terrain while a build runs on the 15 GB laptop; one instance at a time.

## References

- Measurement scripts (scratch, Windows session 2026-10-03): per-second process private bytes / working set / GPU
  shared usage while the demo loads (`Get-Process`, `\GPU Process Memory(*)\Shared Usage`).
- `src/Graphics/Geometry/IndexedVertexResource.hpp:324`, `IndexedVertexResource.cpp:168`;
  `src/Graphics/ImageResource.hpp:344`, `ImageResource.cpp:112`; emeraude-base
  `src/VertexFactory/ShapeVertex.hpp:384-399`, `Shape.hpp:2444-2457`; projet-alpha `src/Builtin/Citadel.cpp:1749-1768`
  (the woods' tree stock).
