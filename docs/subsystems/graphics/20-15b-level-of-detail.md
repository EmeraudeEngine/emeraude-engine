## 15b. Level of Detail (LOD)

### Settings

LOD behavior is controlled via `Core/Graphics/LOD/` settings in `SettingKeys.hpp`:

| Setting | Default | Purpose |
|---------|---------|---------|
| `EnableAutomaticGeneration` | `false` | Enable/disable automatic LOD mesh generation |
| `MinTriangleCount` | `250` | Minimum triangles a LOD level must produce to be generated |
| `ScreenCoverageThreshold` | `0.75` | Screen-space coverage ratio for LOD 0 → LOD 1 transition |
| `ReductionRatio` | `0.33` | Triangle reduction per LOD level (each level keeps ~33%) |

### LOD Generation Pipeline

When `EnableAutomaticGeneration = true`, LOD meshes are generated automatically in `onDependenciesLoaded()` for both `SimpleMeshResource` and `MeshResource`:

0. **Skip a mesh that carries its own levels** (`m_geometry.size() > 1`, owner 2026-10-07; same in
   `MultiLayerMeshResource`): a generated level is appended only at `m_geometry.size() == LODLevel`, so every level
   of such a mesh used to be decimated, uploaded and dropped. citadel: 130 levels asked → 94 (the 36 of the 12
   tree-stock meshes, ~120 k triangles each, no longer computed), 94 ready either way. Log: "'<name>' carries its own
   N LOD levels: no automatic LOD." Accepted on the three OS (engine c33426d9: 12 skipped, 94 ready on macOS and Windows).
1. Check if source geometry is `IndexedVertexResource` with local data
2. Determine levels to generate based on triangle count and `MinTriangleCount`
3. Submit decimation tasks to engine `ThreadPool` (NOT `std::async`)
4. Each level uses `ShapeDecimator` (QEM) at `ratio^level` reduction
5. LOD 0 renders immediately — generation is non-blocking

### LOD Selection

`Scene::selectLODLevel(distance, objectRadius)` computes LOD from screen-space coverage:
```
screenSize = objectRadius / distance
LODLevel = clamp(MaxLODLevels × (1 - screenSize / threshold), 0, MaxLODLevels-1)
```

The threshold is read from `ScreenCoverageThreshold` at scene init (cached in `m_LODScreenCoverageThreshold`).

**Code references:**
- `Renderable/SimpleMeshResource.cpp:onDependenciesLoaded()` — LOD generation trigger
- `Renderable/MeshResource.cpp:onDependenciesLoaded()` — Same for multi-layer meshes
- `Scenes/Scene.rendering.cpp:selectLODLevel()` — Runtime LOD selection
- `Renderable/Types.hpp` — `Renderable::MaxLODLevels` (4): the ladder the VIEW selects from. A mesh can HOLD up to
  `Geometry::MaxLODLevels` (8, `Geometry/Types.hpp`); the extra levels serve a coarser shadow
  (`RenderableInstance::Abstract::setShadowLevelOfDetailBias()`, `Renderable::Abstract::levelOfDetailCount()`).
  ⚠️ Always QUALIFY the name: unqualified inside `namespace Renderable` it is the 4, which once capped a
  6-level tree and emptied the `terrain` forest (`docs/caution-points.md`).

### A chain supplied by its producer (Sept 2026)

Decimation is not the only way in. `MultiLayerMeshResource::load(geometryLODs, materialList,
rasterizationOptions)` files a chain the producer already reduced, finest first, and decimates
nothing. That is the path vegetation takes: a quadric decimator can shrink a leaf card, never
merge two of them, and the canopy is where the triangles are. `Scenes::Toolkit::generateTreeRenderable()`
is the caller.

**Vegetation materials by NAME** (owner decision 2026-09-23): `generateTreeRenderable(label, mesh)`
takes the materials the species names (`TreeMesh::barkMaterial()` / `leafMaterial()`) unless the caller
passes its own, and `Toolkit::vegetationMaterial(name, Bark|Foliage)` resolves a name: a STORE material
of that name (a JSON in `Materials/`) wins; otherwise one is built from the images of the convention —
`<name>-color_a` (required); foliage: `<name>-alpha` as the cut-out mask (red channel) or the colour
image's own alpha, a HASHED alpha test (the card stays opaque; see § Alpha Test — a fixed 0.5 left the
distant pines bare trunks), roughness 0.5; bark: `<name>-roughness`
or 0.85; both: `<name>-normal` if present. Built once, as `Vegetation/Bark|Foliage/<name>`. ⚠️ No
back-lit translucency yet: the engine's subsurface term needs a thickness
(`LightGenerator.PBR.cpp`), a thin leaf is its own open item.

⚠️ **Every level must expose the SAME number of sub-geometries.** A layer is addressed by its
index whatever the level drawn, so a level that disagrees would silently draw one part with
another part's material — bark shaded as foliage.

⚠️⚠️ **That check CANNOT live in `load()`.** The geometries are *dependencies*: inside `load()`
they are still loading, and asking one for its sub-geometry count there answered **1** for a
two-group shape and rejected a perfectly valid tree — the first run of the tree bench lost its
whole back row to it. It belongs in `onDependenciesLoaded()`, where every dependency is
guaranteed loaded. The rule generalises to anything `load()` might want to know about a
dependency's *content*.

⚠️ `m_geometry` is a `StaticVector< …, Geometry::MaxLODLevels >` (8) and its `emplace_back()` **calls
`std::abort()`** when full — this build has no exceptions. `setGeometry()` refuses past the
ceiling with a trace, in both `MeshResource` and `MultiLayerMeshResource`. Until 2026-09-24 the
multi-layer mesh sized it with the VIEW ladder (4) through the unqualified name; `terrain` files 6.

## The level array is read WITHOUT a lock (2026-10-07)

`MeshResource` and `MultiLayerMeshResource` keep their levels in an APPEND-ONLY `StaticVector` (it never reallocates;
a level is never replaced nor removed). The writers — the load (`setGeometry()`) and the automatic LOD job on a worker
(`generateLODLevel()`) — append under `m_geometryMutex` and then publish the count `m_publishedLevelCount` (release);
the readers — `geometry()`, `levelOfDetailCount()`, `boundingBox()`, `boundingSphere()`, `subGeometryCount()` — load
that count (acquire) and never index past it, so a slot they read is fully constructed. **Rule:** a reader never uses
`m_geometry.size()` (the writers' counter) and nothing ever replaces a published slot.

Why: `geometry()` locked the mutex on every draw, and `boundingBox()` / `subGeometryCount()` read the array with NO
lock while a LOD job appended — a data race. Measured: no frame-time gain (`balls-of-steel`, GPU-bound at 93–99 %:
294.5 vs 294.5 FPS, 2.69 vs 2.74 CPU-ms per frame, medians of 5 and 3 alternated runs) — kept for the race (Ave
Robustus), not for speed. Validated with automatic LODs on: `citadel` 94 levels and `sponza` 371 levels published
during rendering, 0 VUID. ⚠️ `sponza` then crashes at shutdown — a pre-existing use after free of the LOD job, item
`lod-job-outlives-its-mesh`.
