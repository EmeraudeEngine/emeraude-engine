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

1. **Metadata extraction** — DONE 2026-10-04 for geometries, images and cubemaps (owner: bounds cached on the
   resource; pixel facts "extracted at release"): `docs/subsystems/resources/03-resources-specific-rules.md`
   § CPU Copies. Movies, sounds and music keep theirs in their own items. ACCEPTED macOS M2 2026-10-04 (engine
   939b3fc2): 0 VUID on citadel and sponza; sponza's cutouts identical to a44b3135 (binary-alpha promotion now via
   `ImageResource::isBinaryAlphaMask()`); citadel census unchanged (2658.8 MiB).
   ACCEPTED Windows (NVIDIA) 2026-10-04: census unchanged (2659 MiB), sponza's cutouts intact; the only VUIDs are
   the known pre-existing `renderPass-12325` (mesh-shader shadow pipeline viewMask on that laptop GPU).
2. **Release by declared usage** — DONE 2026-10-04, behind `Core/Resources/ReleaseLocalData` (OFF until phase 3,
   owner; grace delay `Core/Resources/LocalDataReleaseDelay` = 5 s, owner): `docs/subsystems/resources/03` § CPU
   Copies. Linux, release ON, 0 VUID / 0 new error everywhere: census citadel 2198 → 418 MiB (geometries 998 → 0,
   images 775 → 16: the images no texture read), forest 431, terrain 493, liminal 500, sponza 2210 (of which
   `CompressedImageResource` 1792, out of this scope). glibc kept the freed memory (citadel RSS 5143 MiB untrimmed):
   owner, 2026-10-04 — trim once per release burst, on the pass's pool worker (the trim costs 1-60 ms and holds the
   arena locks), and the pass itself on a pool worker (up to ~165 ms measured on the logic thread). RSS release ON vs
   default (45 s): **citadel 5435 → 2508 MiB, terrain 8813 → 4085 MiB**; logic overruns citadel 1 = 1, terrain ~10
   extra of 17-21 ms (under the 33 ms cycle, the warning fires at 16.66). ACCEPTED macOS M2 2026-10-04 (engine
   96e7f2c7, release ON vs OFF): 0 VUID on citadel / terrain / liminal / sponza, scenes intact, flames animate; census
   citadel 2167 → 386 MiB, terrain 3517 → 461; **the macOS allocator returns the freed LARGE blocks by itself** (no
   trim: citadel MALLOC_LARGE 2602 → 715 MB, phys_footprint 7512 → 5588 MB). Open observation: terrain's MALLOC_SMALL
   GREW by 556 MB with the release ON (swap differed between the runs: 5.1 vs 2.5 GB) — to re-measure. Sponza keeps
   2.2 GiB: its `CompressedImageResource` (1792 MiB), outside this scope. A late reader found as designed:
   a `SimpleMeshResource` created after the release from a released geometry renders (GPU buffers) but gets no
   automatic LOD — phase 3's case. Owner decisions: **deferred + leases**; **type default + code override**;
   scope **indexed / plain geometries, images, cubemaps**. Design:
   - `ResourceTrait` gains a local-data state (resident / released), a per-resource mutex, a lease counter and a
     "retained" flag (GPU only by default for the three types; `retainLocalData()` = "CPU too", one way).
   - A reader of the data takes an RAII **lease** (`leaseLocalData()`): it fails ("not resident") once released;
     the release only happens with no lease held. Leases go in: the automatic LOD jobs (`MeshResource`,
     `MultiLayerMeshResource`), every texture upload (Texture1D / 2D incl. the BC7 cache, TextureCubemap, the
     animated textures through their movies' frames), the ground / terrain displacement.
   - A resource becomes **releasable** after its own upload (geometries) or after a texture upload that read it
     (images, cubemaps: an image no texture ever read stays resident). `CursorAtlas` marks its images retained.
   - The `Resources::Manager` releases the releasable, lease-free, not-retained copies about once a second from the
     logic cadence (`Core::logicsTask()`), after a grace delay since they became releasable: `extractMetadata()`
     first, then a SWAP with empty containers (`clear()` keeps the capacity, base `vertexfactory/07`).
   - Movies read their frame images' metadata (`ImageResource::width()` …) instead of the frame pixels;
     cubemap movies the cubemaps' `cubeSize()`.
3. **Reload** — owner decisions 2026-10-04: every late reader today is a LOAD-TIME reader, so **`acquireLocalData()`
   reloads in the caller's own context** (loader / pool thread, or the thread of a synchronous load, which did its
   I/O there already) and is refused on the render thread; **`requestLocalData()`** keeps the "async + not resident"
   contract for any future per-frame reader. Sources: the store entry's file / JSON re-read by the type's own reading
   code (factored out of `load()`, no status change); **without a source, a GPU readback** (geometries: vertex /
   index buffers back into a shape; images: an RGBA8 texture read back; an image whose only texture went BC7 stays
   resident). Order 3a (reader API + source reload) → 3b (geometry readback) → 3c (image readback);
   **3a DONE 2026-10-04** (`docs/subsystems/resources/03` § CPU Copies), Linux, release ON, citadel: a texture's image
   reloaded from its file in 26 ms (4 MiB), a movie-frame image 10 ms, the store geometry 'Furnitures/MetalBarrel'
   11 ms, the cubemap 'StormyDays' 103 ms (24 MiB); each released again after the grace delay (census back to 418
   MiB); `requestLocalData()` answers "not resident" then leases 150 ms later; with automatic LOD on, the late
   `SimpleMeshResource` reloads its geometry and gets its LOD (0 skipped). The procedural tree 'Aspen0LOD2' cannot
   come back yet (3b). 0 VUID.
   `Core/Resources/ReleaseLocalData` flips to ON in a last commit once all three pass on the three OS.
4. **Owner decisions after phase 0 (2026-10-03):**
   - **The construction-time hash indexes are dropped AT UPLOAD** — DONE 2026-10-03: `IndexedVertexResource` /
     `VertexResource::createOnHardware()` call base `Shape::releaseConstructionIndexes()` after a successful upload; a
     later edit rebuilds them (base `vertexfactory/07`). Census, geometry container: citadel 1359 → 998 MiB (−361),
     forest 1328 → 984 (−344), terrain 3286 → 2411 (−875); RSS at rest (untrimmed: glibc keeps part of it) citadel
     6046 → 5855 MiB, terrain 9508 → 9249.
     ACCEPTED macOS M2 2026-10-04: 2324 tests (Release + ASan/UBSan), citadel geometry 1311 → 998 MiB (= Linux: the
     libc++ gap was all in the indexes), phys_footprint 8125 → 7835 MB (MALLOC_SMALL −208, MALLOC_LARGE −83).
     ACCEPTED Windows (NVIDIA) 2026-10-04: 2324 tests, citadel geometry 1328 → 998 MiB, census 2989 → 2659 MiB,
     private bytes 7470 → 6943 MiB (−527), working set 5102 → 4617 MiB.
   - **Decoded music and movie frames are separate items**: `decoded-music-stays-resident`,
     `movie-frames-stay-resident` (streaming is another mechanism).

5. **The memory the census does not see**: citadel after the index release, census 2.66 GiB against Windows private
   bytes 6.9 GiB (Linux: anonymous 4.9 GiB, of which ~0.9 GiB glibc gives back on a trim). Candidates: the driver's
   host allocations, resources outside every container (LOD levels, generated grids, the physics' triangle meshes,
   octrees), loading intermediates kept at the allocator's high-water mark. To be split (heaptrack / VMA statistics)
   before deciding anything there.

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
- **macOS M2 (peer, 2026-10-03)**: 2322 tests green; citadel census 2972 MiB — images, movies, music identical to
  Linux, geometry 1311 MiB (−48: libc++ sizes the hash indexes differently — gone once they are released). Footprint 8.1 GB, of which
  MALLOC_LARGE 2.99 GB (≈ the census) beside ~3.5 GB of graphics allocations in the same unified memory; 2.6 GB were
  swapped out (memory pressure).
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

- **Late readers found during phase 1, for phase 2's declared usage**: `CursorAtlas::setCursor()` reads an image's
  pixels whenever a cursor is set (a cursor image is "CPU too"); `MovieResource` copies frame pixels at its load;
  ground / terrain displacement reads an image at the ground's load; automatic LOD (`MeshResource` /
  `MultiLayerMeshResource::generateLODLevel`) decimates the source shape once; a texture created later from an
  already-released image needs the pixels back (reload).
- `extractMetadata()` and the release must not run while another thread reads the resource: phase 2 decides the
  thread and the moment.

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
