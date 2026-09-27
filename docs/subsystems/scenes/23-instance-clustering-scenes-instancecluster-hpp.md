## Instance clustering — `Scenes/InstanceCluster.hpp` (2026-08-09)

`buildInstanceClusters()` splits an instance set into a fixed metric grid, one entity per
non-empty cell, transforms stored RELATIVE to the cell centroid. The rendering octree then culls
whole cells with no new culling path. Cells are anchored on the centroid rather than the grid
intersection, so relative coordinates stay small — precision no longer depends on how far the
scene sits from the origin.

> [!NOTE]
> **RESOLVED (2026-08-09) — it was never the instance objects.**
>
> | Cell size | Cells | RSS before | RSS after |
> |-----------|-------|------------|-----------|
> | 32 units | 4006 | **104 GB** | **3279 MB** |
> | 500 units | 16 | 3.5 GB | — |
>
> Same 20 000 instances, same geometry, no change to `RenderableInstance::Multiple`. The cost was
> the octree's **all-levels storage**: an element was copied into every sector it touched, at
> every depth. A cell spanning a boundary near the root multiplied itself down whole subtrees.
> Small cells span more boundaries, hence the ratio that looked like a per-instance cost.
>
> Cell size can now be chosen for culling quality rather than dictated by the memory budget.
> See "Octree storage and traversal" below — the fix moved elements to a single sector, which
> **changes what a traversal must read**.
>
> Eliminated along the way, and still true: VMA dedicated allocations (`Buffer::createWithVMA`
> uses `VMA_MEMORY_USAGE_AUTO` with no dedicated bit, and suballocates — measured 277 MiB in 4056
> allocations over 10 blocks); `SceneInstanceTransforms` (a `Scene` member, one per scene, not per
> entity); the VBO size itself.

> [!WARNING]
> Any test that creates a number of objects proportional to a parameter must run capped:
> `systemd-run --user --scope -p MemoryMax=8G -p MemorySwapMax=0 --quiet -- <cmd>`.
> The uncapped first run took the machine to 1 GB available and made it unusable.
