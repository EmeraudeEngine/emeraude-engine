# CPU copies: release and reload — history, decisions, measurements (2026-10-03 → 10-04)

The RULES live in [`03-resources-specific-rules.md`](03-resources-specific-rules.md) § CPU Copies (leases, the three
ways to read, what is released, the release pass). This file keeps WHY and WHAT WAS MEASURED, from the closed item
`cpu-copies-retained-after-upload` (opened by the Windows session, 2026-10-03).

## Why

On the Windows laptop (16 GB, ~6.5 GB free with the owner's IDEs open), `citadel` and `terrain` drove the system to
the edge (background jobs killed for "critically low memory"). Measured 2026-10-03, RTX 3060 Laptop, validation OFF:
private bytes — engine alone 1.46 GB, `collision-debug` 2.06, `forest` 4.84, `citadel` 7.40, `terrain` 9.66 GB. The
memory climbed with the scene construction (the tree stock) and NEVER came back down: every geometry kept its whole
`Shape`, every image its decoded pixmap, beside their GPU copies.

State of the art the design follows: Unity (`Mesh.UploadMeshData(markNoLongerReadable)`), Unreal
(`UStaticMesh::bAllowCPUAccess`), Bevy (`RenderAssetUsages` MAIN_WORLD / RENDER_WORLD), Godot 4
(`surface_get_arrays` reads the GPU copy back).

## Owner decisions, in order

- 2026-10-03: **declared usage** ("GPU only" by type, "CPU too" by code); **reload from the source, else the GPU**;
  **asynchronous "not resident"** for per-frame readers; **measure first**.
- Phase 0: drop the shape's construction-time hash indexes AT UPLOAD (base `Shape::releaseConstructionIndexes()`);
  decoded music and movie frames are separate items.
- Phase 1: bounds cached on the geometry resource; the pixel facts of images / cubemaps **extracted at release**.
  Movies show their store images instead of copying them (image or owned pixmap).
- Phase 2: **deferred + leases**; type default + `retainLocalData()`; scope = indexed / plain geometries, images,
  cubemaps; behind `Core/Resources/ReleaseLocalData` (OFF until the reload existed); grace delay 5 s; the pass and
  the Linux trim on a pool worker, the trim once per release burst (asked after the owner questioned whether a trim
  was counter-productive: 1-60 ms each, it held glibc's arena locks on the logic thread).
- Phase 3: **`acquireLocalData()` blocks** (all late readers are load-time ones; refused on the render thread),
  **`requestLocalData()` never waits**; reload from the recorded store entry, else the **geometry GPU readback**;
  **images and cubemaps without a store source are never released** (their texture is BC7 on every BC-capable GPU).
  macOS `malloc_zone_pressure_relief()`: measured, no effect, **not adopted**.
- 2026-10-04: `Core/Resources/ReleaseLocalData` **ON by default** after the three-OS validation. ⚠️ An existing
  `settings.json` keeps the `false` its first run wrote (`getOrSetDefault()`): set it to `true` by hand.

## Measurements (45 s after the first draw, release OFF → ON)

| | citadel | terrain | forest | liminal | sponza |
|---|---|---|---|---|---|
| Census, Linux (MiB) | 2198 → 499 | 3549 → 529 | → 467 | → 728 | → 2246 |
| Census, macOS M2 | 2167 → 467 | 3517 → 497 | 1570 → 435 | 1032 → 696 | 3845 → 2214 |
| Census, Windows | 2167 → 467 | → 497 | | → 696 | |

- Linux RSS with the trim: citadel 5435 → 2508 MiB, terrain 8813 → 4085 MiB. Windows citadel private bytes 6414 →
  4752 MiB (7470 when the item opened). macOS phys_footprint citadel 7512 → 5588 MB, terrain 11 GB → 8.9-9.3 GB.
- What stays: decoded music (367 MiB everywhere, item `decoded-music-stays-resident`), sponza's
  `CompressedImageResource` (1792 MiB, not in this scope), the source-less images (citadel 81, liminal 228 MiB: the
  CarConcept / DragonPig embedded textures), movie frames the movies own (generated ones).
- Where citadel's 1359 MiB of geometry went before (phase 0): vertices 542, the construction-time unpaired-edge index
  360, triangles 200, vertex colours 147, edges 109 MiB; a tree LOD0 = 116 MiB.
- Reload costs (Linux, citadel): an image from its file 26 ms (4 MiB), a cubemap 103 ms (24 MiB), a store geometry
  11 ms, a tree from its GPU copy 18-116 ms (7-72 MiB; re-encoded byte-identical to the downloaded buffers).
- A scene after the release renders the same (pixel diffs = animated actors only); a pinned-exposure terrain ABBA
  (Linux) shows ±0.1 % mean luminance between OFF and ON — macOS's −1.4 % was auto-exposure variation.

## Allocators (who gives the memory back)

- **glibc keeps freed blocks** in its arenas: without `malloc_trim()` citadel's RSS barely moved (5143 MiB). The
  trim runs once per burst on the pass's pool worker.
- **Windows' heap returns them by itself** (private bytes −1662 to −1746 MiB, no trim).
- **macOS returns the LARGE blocks by itself** (MALLOC_LARGE −1.9 to −2.9 GB); its MALLOC_SMALL varies ±150-200 MB
  run to run, sometimes higher with the release ON; the pressure relief did not change it.

## Traps met

- The reader inventory before phase 1 found the per-frame readers (bounds, dimensions, durations) and the one-shot
  ones (automatic LOD, alpha promotion, the sky's illuminance, displacement, the cursor, movies copying frames).
- A packed / equirectangular cubemap finds its image by the RESOURCE NAME: the reload's temporary cubemap must carry
  the same name.
- `downloadImage()` leaked one reserved transfer operation per call (caution-points, fixed 2026-10-04).
- The base readback broke the Windows build once: MSVC C4244 on `? -1 : 1` passed to a float (GCC / Clang silent).
- Windows `Get-Process` private bytes keep heap memory not yet decommitted; PowerShell prints decimals with a comma
  on a fr-FR machine; never run citadel / terrain beside a build on the 15 GB laptop.

## Follow-ups (items)

`memory-outside-the-census` (terrain: 8.1 GB private for a 497 MiB census), `decoded-music-stays-resident`,
`movie-frames-stay-resident` (the residency of decoded frames), `automatic-lod-buffers-destroyed-in-use`,
`normal-map-flip-mutates-shared-image`. Not decided: give an embedded image its model file as a source (liminal's
228 MiB).
