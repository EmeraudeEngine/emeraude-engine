## 15b. PostProcessStack Race Condition (Fixed Mar 2026)

> [!WARNING]
> **The render thread must NOT create the scene render target before the logic thread has
> set the PostProcessStack.** If created too early, the scene target uses wrong formats
> (no HDR, no depth/normals attachments) because `PostProcessor::requiresHDR()` etc. return
> false when the stack is null.
>
> **Fix:** Defer `SceneRenderTarget` creation until the PostProcessStack is non-null.
> The Renderer checks for a non-null stack before creating the scene target.
>
> **Code references:**
> - `Graphics/Renderer.cpp` — Deferred scene target creation
> - `Graphics/PostProcessor.cpp` — `requiresHDR()` aggregates chain needs

> [!WARNING]
> **Still true with the on-demand chain (Jul 2026), and it constrains WHEN an application
> may hand over a stack.** The render thread can now create an empty stack itself
> (`Scene::requirePostProcessStack()`), and `Scene::setPostProcessStack()` **destroys** the
> stack it replaces — so calling it on a scene that is already being rendered would tear
> down the camera effects under the render thread. Build the scene fully, THEN activate it:
> `Manager::newScene()` deliberately does not activate, which is what keeps the two writers
> apart. Never call `setPostProcessStack()` on the active scene.
>
> Ordering inside the frame is what makes the requirements correct: the scene target is
> created AFTER `syncCameraEffects()`, so a stack the camera just populated already reports
> `requiresHDR()` on the very frame it appears — no one-frame LDR flash.

### Adaptive geometries — the CDLOD terrain over a height clipmap (Sept 2026)

`Geometry::CDLODTerrainResource` (every `TerrainResource` floor, since 2026-09-22 — it REPLACED
`AdaptiveVertexGridResource`, deleted with its window slide, far mesh, skirt and stitching fans; owner
decisions in `docs/todo/terrain-cdlod-heightmap-clipmap.md`) is Strugar's CDLOD (JGT 2009,
https://github.com/fstrugar/CDLOD) on a Losasso-Hoppe clipmap (SIGGRAPH 2004). No terrain vertex is
stored: ONE shared patch of (G+1)² flat points (G = 64, positions only, 12 B each) is drawn once per
selected quadtree node, placed by push constants and displaced in the vertex stage. The contract on
`Geometry::Interface` is a SELECTION made once per pass on the render thread:

| Call | Role |
|---|---|
| `prepareAdaptiveRendering(lodViewPosition, cullingFrustum, worldCoordinates)` | Strugar's recursive node selection: a node of level k is taken when its box (per-node Y range) is within `range_k` of `lodViewPosition`; beyond `range_{k-1}` it is drawn whole, otherwise its children decide and it draws, at level k, the QUARTERS none of them took (the patch IBO is sorted by quarter). The pass's frustum culls in the traversal (`nullptr` for multi-view targets) |
| `getAdaptiveDrawCallCount()` / `getAdaptiveDrawCallRange(i)` | the selected nodes: whole patch or one quarter |
| `getAdaptiveDrawCallConstants(i)` | the node's `{origin x, origin z, size, level}`, pushed right before its draw with the pass's LOD camera (`drawAdaptiveGeometry()`, at `Program::heightfieldPushConstantOffset()`) |
| `surfaceDescriptorSet()` | the PerModel set of a heightfield program: height clipmap, normal clipmap, per-frame uniforms (`Graphics/Geometry/HeightfieldSurface.hpp`, layout `Saphir::Generator::getHeightfieldSurfaceDescriptorSetLayout()`) |
| `updateSurfaceVideoMemory(lodViewPosition, frameIndex)` | render thread, once per frame, BEFORE the shadow maps (`Renderer::updateSurfaceGeometries()` for every geometry registered with `registerSurfaceGeometry()`) |
| `dedicatedRTVertexBufferObject()` / `dedicatedRTIndexBufferObject()` / `dedicatedRTVertexFlags()` | the traced proxy, read by the BLAS build AND the hit-shading metadata through `rtVertexBufferObject()` / `rtIndexBufferObject()` / `rtVertexFlags()` |

⚠️⚠️ **The LOD camera and the culling frustum are two different things.** `castShadows()` passes the
SHADOW target's frustum but the **MAIN camera's position**: the caster must be the very mesh the
receiver is drawn with, or the shadow swims at every level boundary. The camera pushed per node is
the SAME `lodViewPosition` the selection used, so the vertex stage morphs exactly where the selection
put the boundaries — a reflection probe selects and morphs from its own centre, consistently.

**The vertex stage** (`AbstractVertexStage::generateHeightfieldSurfaceCode()`): `flat = origin + g · step`;
`morphK` grows 0 → 1 over the last third of the node's ring (Strugar's `morphStartRatio` 0.66, fully
morphed 1 % before the boundary), from the camera to the UNMORPHED vertex; the odd coordinates slide by
one cell toward the even ones (at 1 the patch IS the next level's patch — every quad is split along the
same diagonal, which is what makes the collapse exact); the height blends from clip level k to k+1 with
the SAME factor, so a fully morphed vertex reads exactly what the coarser neighbour reads. No seam is
stitched. Levels above the last clip level read the last one. The frame is a function of the normal
(Khronos convention of the grid it replaced: T = dP/du along +X, B = N × T), the UV is `xz · scale + offset`.

**The clipmap** (`HeightfieldSurface`): L levels of 2048² texels, level l's texel = `cell · 2^l`, one
`R16_UNORM` 2D-array layer each (heights over the source's range) + one `R16G16_SFLOAT` layer of normal
X/Z. A world lattice point i lives in texel `i mod 2048` and the sampler is REPEAT: no per-level origin
exists. Levels 1..L-1 are a TENT-FILTERED pyramid (`Grid::halvedTent()`, 1-2-1 separable), level 0 the
source itself: a coarse level holds the mean of its footprint, not an aliased point sample. The last
level holds the whole grid and never moves (L = 5 for 16 384 cells). Each frame a level re-centres by
32-texel strips: the entered strip is copied (staging per frame in flight) and its normals re-baked by
a compute pass (central differences on that level's heights — the gradient of mean heights IS the mean
gradient), in a command buffer submitted to the GRAPHICS queue ahead of the frame: queue order puts it
before every pass, the frame's fence covers it, and the barrier's first scope is every earlier
command, so frames still in flight never race the rewrite. The first frame uploads everything (40 MiB,
113 ms on `terrain`); until then `surfaceDescriptorSet()` is null and the terrain skips its draws.

**The fragment stage** rebuilds the frame PER PIXEL (`FragmentShader::enableHeightfieldPixelFrame()`):
the interpolated frame variables are received under other names (Vulkan links stages by location, so
the rename is free) and `main()` opens by redefining the canonical ones from `hfPixelNormalAt()` — the
level whose texel matches the pixel's footprint, never finer than the finest level holding the pixel,
sliding to the next level over the outer tenth of a level's extent. The material code is untouched.
This is what removed the dark streaks of the adaptive grid (one 1 m normal per vertex read by every LOD).

**The ray-tracing proxy** (`updateRayTracingProxy()`, logic thread via `TerrainResource::updateVisibility()`):
4096 m around the camera at the finest power-of-two step fitting `Core/Graphics/RayTracing/TerrainBLASMaxTriangles`
(step 8, 524 288 triangles, 51 ms on `terrain`), 14-float vertices built on the pool from the CPU grid,
published by `updateVideoMemory()` (old buffers retired, BLAS marked stale LAST); rebuilt when less than
1500 m is left to its edge. Physics keeps reading the CPU `Grid< float >`, shared (never copied) between
`TerrainResource` and the geometry.

⚠️ **The object space of the terrain IS the world** (a ground answers levels in world coordinates):
`worldCoordinates` is ignored by the selection, the nodes and the camera are pushed in world space.
⚠️ Parameters that cannot keep a node inside its clip level (`detailDistance` + node diagonal + strip
lag beyond the level's half extent) are clamped at load with a warning. Measured against the adaptive
grid at the same pose (`terrain --demo-options 100000,25`, 2880×1620, 3070 Ti): ScenePass 3.27 ms
(screen-space lane) / 2.32 ms (ray-traced lane) against 2-3.4 ms, VRAM of the PROCESS 1821 MiB against
2791 MiB, 0 VUID. Traps met on the way: [`docs/caution-points.md`](../../caution-points.md) § CDLOD terrain.
