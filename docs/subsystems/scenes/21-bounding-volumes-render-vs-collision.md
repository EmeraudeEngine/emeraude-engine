## Bounding volumes — render vs collision (2026-08-09)

`Component::Abstract` exposes **two** extents. They differ in CARDINALITY, not merely in value —
that is what makes them two contracts rather than two names:

| | Cardinality | Why |
|---|---|---|
| **Render** | **one** general box / sphere | Culling asks a single question: *is any part of this inside the frustum?* One wide envelope answers it exactly, in one test. An envelope that is too large costs a few useless draw calls. |
| **Collision** | **a group** of primitives | Collision asks *where* and *against what*. One wide envelope answers WRONG — the tree whose canopy you hit at ankle height. An envelope that is too large produces incorrect behaviour, not wasted work. |

> [!IMPORTANT]
> **Target contract**: `renderBoundingBox()` stays singular; the collision side becomes a LIST of
> primitives. The single-box collision accessors below are the INTERIM state — correct for
> simple objects, insufficient for anything with a real silhouette.

Current state:

| Accessor | Meaning | `Visual` | `MultipleVisuals` |
|----------|---------|----------|-------------------|
| `localBoundingBox()` / `localBoundingSphere()` | **PHYSICAL** — what the collision model is built from | renderable's box | box of a **single** instance |
| `renderBoundingBox()` / `renderBoundingSphere()` | **VISUAL** — what frustum culling and the rendering octree must use | renderable's box | **union of every instance** |

> [!CAUTION]
> **The rendering octree inserted every entity as a POINT at its origin.** Only the physics
> octree (`enable_volume == true`) ever used a volume; the rendering branch called
> `insertWithPrimitive(element, element->getWorldCoordinates().position())` and nothing else. A
> 250-unit terrain tile, or a cell holding a thousand instanced trees, was therefore culled by
> whether its ORIGIN landed in a visible sector — it vanished while filling half the screen.
>
> **Fixed (2026-08-09):** both `insert()` and `update()` use `getWorldRenderBoundingBox()` when
> the element exposes one, detected with `if constexpr ( requires { ... } )` so the octree stays
> generic and element types that know nothing about rendering keep the point path.
>
> ⚠️ The `update()` point path has a "still inside its last subsector, nothing to do" shortcut.
> That shortcut is **unsound for volumes**: an element whose origin has not moved can still have
> grown — an animated pose, a rebuilt instance set — and now span sectors it is not registered
> in. It is skipped whenever a render box is available.
>
> Widening the COLLISION extent to fix any of this would have been the wrong cure: a forest cell
> would become one solid block. Hence the split.

> [!NOTE]
> `MultipleVisuals` caches its visual extent and **recomputes it when the renderable finishes
> loading** — the renderable's own box is empty before that, so a union built at construction
> is empty too, and the component would be culled as a point anyway. The eight corners are
> transformed individually: under rotation, transforming only the min/max pair yields a box that
> does not contain the shape.

### Open: compound collision shapes

A collision extent is currently **one** box or sphere per component, and `AbstractEntity` builds
a single `BoxCollisionModel` from it. Real objects need several: a tree is a narrow trunk at
ground level and a wide canopy above it — one AABB around both makes you collide with foliage at
ankle height. Moving the collision extent to a **list** of primitives is a separate project: it
touches the collision model, the broad phase and the narrow phase, all of which assume one shape
per entity. Owner-identified, not scheduled.
