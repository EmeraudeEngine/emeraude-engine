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

## Two retention mechanisms found in the code (quantified in § Phase 0)

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

Phase 0 is done (the census and the measurement below; the reader inventory further down). The owner's order:

1. **Metadata extraction**: every per-frame reader of a CPU copy (bounds, dimensions, durations, the binary-alpha
   flag, the sky's illuminance) reads a value kept on the resource, not the shape / pixmap.
2. **Release by declared usage**: "GPU only" (default for meshes and textures) frees the CPU copy after the one-shot
   consumers; "CPU too" for grounds / terrains. A release SWAPS with empty containers (`clear()` keeps the capacity,
   base `vertexfactory/07`). A resource gains a "not resident" state (no `Loaded → Unloaded` exists today).
3. **Reload, asynchronous**: from the store source, else a GPU readback; a request answers "not resident" and the
   reader skips that frame.
4. **Owner decisions after phase 0 (2026-10-03):**
   - **The construction-time hash indexes are dropped AT UPLOAD**, independently of the release: the engine calls a
     base `Shape` "finish construction" step before the upload (the three indexes swapped empty); a shape edited later
     rebuilds them lazily in `addEdge()` / `addVertex()`. Citadel: −360 MiB with no reload needed. To do first.
   - **Decoded music and movie frames are separate items**: `decoded-music-stays-resident`,
     `movie-frames-stay-resident` (streaming is another mechanism).

## Phase 0 — the measurement (2026-10-03, Linux, RTX 3070 Ti, Release)

`Core.ResourcesManagerService.memoryCensus()` (committed) + a LOCAL probe (never committed): after ~25 s of a settled
scene, release every loaded `IndexedVertexResource` / `VertexResource` shape (bounds kept) and every `ImageResource`
pixmap, then `malloc_trim(0)`, then `/proc/<pid>/smaps_rollup`. Each figure is one run.

| Demo | RSS at rest | RSS trimmed | Census (CPU copies) | of which geometry / images | RSS trimmed after release | Gain |
|---|---|---|---|---|---|---|
| citadel | 6046 MiB | 5141 MiB | 3051 MiB | 1359 / 775 | 3080 MiB | −2061 MiB |
| forest | 3821 MiB | 3096 MiB | 1957 MiB | 1328 / 154 | 1679 MiB | −1417 MiB |
| terrain | 9508 MiB | 8527 MiB | 4915 MiB | 3286 / 653 | 4651 MiB | −3876 MiB |

- The scene rendered the same after the release (citadel: 0.19 % of the pixels differ by more than 8/255, the animated
  dragon), no new error, no crash: after the one-shot consumers, nothing in those three scenes read the released
  geometry or pixels (bounds were kept).
- **Where citadel's 1359 MiB of geometry go** (339 shapes, 6.17 M vertices, 3.49 M triangles; a local per-shape probe):
  vertices 542 MiB, the construction-time unpaired-edge index 360, triangles 200, vertex colours 147, edges 109; the
  vertex / colour merge indexes < 1. The trees' LOD0 shapes are 116 MiB each (Broadleaf: 511 k vertices).
- `unusedBytes` = 508 MiB of citadel's 775 MiB of images are held by the store alone (no texture refers to them).
- The rest of the census: decoded movie frames (citadel 492 MiB, 7 movies), decoded music (367 MiB, 32 tracks).
- **glibc keeps freed memory**: `malloc_trim(0)` alone gave back 0.7–1 GiB in every demo (loading garbage). A release
  measured without a trim under-reports; a release that must lower the RSS may need a trim after it.

## Owner decisions (2026-10-03, after the reader inventory below)

The owner asked for a system that RELEASES a CPU copy once it is useless and RELOADS it automatically when needed
again ("continuity"). State of the art: Unity (a mesh without Read/Write is uploaded then dropped from CPU memory;
`Mesh.UploadMeshData(markNoLongerReadable)`), Unreal (`UStaticMesh::bAllowCPUAccess`), Bevy (`RenderAssetUsages`
MAIN_WORLD / RENDER_WORLD: a render-only asset keeps only its metadata, reloading brings the data back), Godot 4
(mesh data lives on the GPU; `surface_get_arrays` reads it back, stalling the rendering).

- **Release: DECLARED USAGE.** Each resource declares "GPU only" (default for meshes and textures) or "CPU too"
  (ground / terrain grids, which physics reads every step). The metadata the per-frame readers need (bounds,
  dimensions, durations / frame times, the binary-alpha flag, the sky's illuminance) is extracted first. The
  release happens after the one-shot post-upload consumers (automatic LOD, alpha promotion, illuminance, RT strip
  indices).
- **Reload: the SOURCE, else the GPU.** The store entry's file / JSON when there is one (exact), otherwise a
  readback of the GPU copy (vertex / index buffers, texture pixels; partial: no full topology, a compressed texture
  comes back compressed).
- **Rehydration: ASYNCHRONOUS.** A request answers "not resident" and starts the reload in the background; a
  per-frame reader skips that frame. Never a stall on the render or logic thread (the lesson of
  `logic-stalls-on-runtime-pipeline-compile`).
- **Order: MEASURE FIRST.** Phase 0 = the memory census (bytes per resource type) + the local measurement of the gain.
  Then the metadata extraction, the release, the reload.

## The reader inventory (2026-10-03)

Who reads a CPU copy after its upload (file:line in the session's report; the main ones):
- **Every frame, metadata only**: bounding volumes (LOD, culling: `Scene.rendering.cpp` `worldRadius()`, shadow / view
  LOD, light culling; entity bounds), `DisplacedGridResource::meshShadingSurface()` dimensions, animated textures'
  frame count and durations (`BindlessTextureManager::syncTextureSet`, `frameIndexAt`), sounds' durations.
- **Once after upload**: automatic LOD decimation (`MeshResource::generateLODLevel`, the whole shape), alpha-mask
  promotion (`Texture2D::isBinaryAlphaMask`, pixels), the sky's hemisphere illuminance (cubemap pixels),
  `VertexGridResource::generateTriangleListIndicesForRT()` (also on a late RT builder / stale BLAS).
- **Permanently**: ground queries (`BasicGroundResource` / `TerrainResource` `getLevelAt`, `getNormalAt`,
  `visitTriangles`: physics every step, demos, console), CDLOD streaming (`m_source` heights every frame, the RT
  proxy's sub-grids).
- **Writes after upload (a defect)**: `BasicGroundResource::load(Json)` displaces its grid AFTER the upload
  (`BasicGroundResource.cpp:225` then `:264`): CPU and GPU disagree.
- **No reader**: physics triangle meshes (built from the caller's own shape), imposter baking, IBL baking, X-ray, GPU
  skinning, the editor (collision models). Citadel's tree stock: only its bounds are read after upload.
- **A GPU rebuild** (`destroyFromHardware(false)` then `createOnHardware()`, a device loss or a swap-chain / format
  change) re-uploads FROM the CPU copy: a released resource must reload before it can be rebuilt.
- **Reloading today**: none. No Loaded → Unloaded state; a store resource can find its source by name
  (`Container::localFilepath`); a procedural resource keeps no recipe (the lambda runs once and often captures the
  whole shape); LOD levels and some grids live outside any container.

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
