## Render list statistics and the shadow casting distance (2026-09-23)

**`Scene::viewRenderStatistics()` / `shadowRenderStatistics()`** (console
`Core.SceneManagerService.getRenderStatistics()`): per geometry LOD, the batches, instances and triangles the
last frame's lists submit, measured on the lists as built (after frustum, distance and LOD selection).
Triangles = the drawn index count / 3 × the instances. The view record covers every colour list of the
primary view; the shadow record sums the shadow targets cast before it (`castShadows()` accumulates,
`prepareRender()` of the View publishes both). First reading, on `terrain` with 209 939 trees: the view drew
135 090 instances, ALL at LOD 3, 1.09 billion triangles — and the sun's map 419 878 instances, **3.24 billion
triangles**, three times the view.

**`RenderableInstance::Abstract::setShadowCastingDistance(metres)`** (0 = no limit): beyond that distance
from the VIEWER (the main render target, published state), the instance enters no shadow map — the test sits
in `populateShadowCastingRenderList()`, after the light's own distance and frustum tests, for static entities
and nodes alike. Distance to the instance's ENTITY position: ⚠️⚠️ a cell of instances must stand on the
ground at its centre, not at ground zero — `terrain`'s cells first sat at y = 0 under a ground 310 m up, and
the limit dropped every tree. With 250 m: shadows 419 878 → 8 430 instances, frame ~600-900 ms → 46 ms.

⚠️ **The LOD is chosen per ENTITY** (`selectLODLevel(distance, radius)`, one level for every instance of a
`Multiple`): a 62.5 m cell of trees switches as one, and with the default coverage threshold (0.75) a
~8 m tree is at LOD 3 beyond ~21 m, so a cell centre 31-44 m away draws even its nearest tree at LOD 3.

**Render-target lists are SNAPSHOT before their callbacks (2026-09-23).** `forEachRenderToShadowMap/Texture/View()`
copy the live targets under the list lock and call back WITHOUT it (`snapshotRenderTargets()`): held across a
`prepareRender()`, the texture-target lock crossed the node lock with `SkyFollowsSun` on the logic thread and froze
`terrain` (`docs/caution-points.md`). The `with*()` batch variants still hold the lock — keep their callbacks trivial.

**Draw range and bake-only instances (2026-09-23).** `RenderableInstance::setDrawDistanceRange(near, far)` is tested
after the frustum in both render-list branches (static entities, nodes) and bounds the RT lists by its far limit;
`isBakeOnly()` instances are rejected by `checkRenderableInstanceForRendering()` unless they are the target's bake
subject, and never reach a shadow list or the TLAS; a BAKE target (`bakeSubject() != nullptr`) never rebuilds the
TLAS from its one-subject lists. `Toolkit::bakeTreeImposter()` uses both (Graphics `AGENTS.md` § 15e).
