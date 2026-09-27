## Visual Debug System

Entities support visual debugging through `enableVisualDebug()` with different visualization types.

### Debug Types

| Type | Purpose | Mesh Used |
|------|---------|-----------|
| `Axis` | Show entity orientation | RGB axis lines |
| `Velocity` | Show movement direction | Arrow |
| `BoundingShape` | Show collision model | Shape-specific mesh |
| `Camera` | Show camera frustum | Camera model |

### BoundingShape Visualization

The debug system visualizes all collision model types with appropriate transformations:

- **Point**: Identity transform (axis gizmo used)
- **Sphere**: Uniform scaling by diameter
- **AABB**: World-space axis-aligned box (always aligned to scene axes, not entity rotation)
- **Capsule**: Translation to center + scaling (diameter, height, diameter)

**AABB debug shows the world AABB**, not the local one. For rotated entities, the world AABB
is larger than the geometry. The instance transform uses `inverseEntityMatrix * translation(worldAABBCentroid) * scaling(worldAABBDims)` to counter-rotate the debug mesh so it remains axis-aligned in world space.

See: `AbstractEntity.debug.cpp:enableVisualDebug()`, `AbstractEntity.debug.cpp:updateVisualDebug()`

### The TWO extents, and why a debug helper must not touch either (Aug 2026)

> [!CRITICAL]
> An entity carries **two different extents**, and confusing them has already cost a session:
>
> | | Component accessor | Entity member | Consumer |
> |---|---|---|---|
> | **RENDER** | `renderBoundingBox()` / `renderBoundingSphere()` | `m_renderBoundingBox` | frustum culling, rendering octree |
> | **PHYSICS** | `localBoundingBox()` / `localBoundingSphere()` | `m_collisionModel` | collision, physics octree |
>
> ⚠️ **The physics one INHERITS the render one by default** — `Component::Abstract::localBoundingBox()`
> literally returns `renderBoundingBox()`. That default is right for real content and wrong for an
> overlay, and it is the whole reason the two get confused.
>
> **`Component::Abstract::setContributesToEntityExtents(false)` excludes a component from BOTH.**
> Every visual debug helper sets it (in `enableVisualDebug()`). A gizmo is DEBUG LOGIC: it must never
> move an engine-level measurement of the content it only describes. A helper excluded from the
> render extent may be culled a touch early — deliberate, and cheaper than a falsified extent.
>
> ⚠️⚠️ **Set the flag from the builder's `setup()` callback, NEVER after `build()`.** `build()` links
> the component and linking runs `updateEntityProperties()` immediately, so a flag set afterwards
> arrives one accumulation too late.
>
> **The defect this fixed, and its signature:** the axis gizmo is generated at extent 1.0 AND is drawn
> scaled by the collider radius. Left contributing, it merged into the collider of whatever it
> annotated — a 1 m cube got its collider nearly doubled by the mere act of looking at it — and the
> bigger collider then drew a bigger gizmo, which is how it was spotted (arrows overshooting the
> object). In `geometry-debug` it read as "the AABB never shrinks when I switch object", because the
> helper's constant raw box floored it. **No assertion can see this**; the detector is
> `coordinates-debug` with both box helpers on.
>
> **Two helpers, on purpose**: `VisualDebugType::CollisionShape` (vertex-coloured) draws the PHYSICS
> extent, `VisualDebugType::RenderBoundingBox` (flat cyan) the RENDER one. Enable both to see them
> apart — on content with no scaled instance they must coincide.
> ⚠️ `CollisionShape` needs a collision model, so it silently traces an error and draws NOTHING if
> enabled before the entity has any component: enable helpers AFTER building the visual.
> ⚠️ Only the AXIS gizmo overshoots (`AxisDebugRadiusOvershoot`, 1.5x) — a shape helper reports its
> extent truthfully or it is not an instrument.

### ⚠️ OPEN: the extents ignore the renderable instance transform

> [!WARNING]
> `Component::Visual::renderBoundingBox()` returns `renderable()->boundingBox()` — the RAW mesh box,
> **not** transformed by the instance matrix set through `RenderableInstance::setTransformationMatrix()`.
> Both extents therefore ignore any per-instance scale. Content sites that DO set one:
> `DefinitionResource.cpp:457,545` (scene JSON scale), `Manager.console.cpp:547` (`addMesh` scale),
> `projet-alpha` `Actor/Fox.cpp:107` (**0.01**) and `Builtin/GameLogic.cpp:200,274` (0.01 / 0.5).
> At scale 0.01 the collider and the culling box would be 100x too large.
>
> **Status: code-derived, NOT reproduced on screen** (the actor gizmo is gated behind an
> `EnableVisualDebug` flag that no current demo sets, so the Fox shows no box to measure). Do not
> quote it as measured until someone enables that flag and captures it.

### Collision Model Auto-Creation

**CRITICAL BUG PATTERN**: Visual components with meshes trigger automatic collision model creation.

When creating debug/gizmo entities (e.g., sun position markers):
1. `generateSphereInstance()` creates a visual mesh
2. `updateEntityProperties()` auto-generates AABB from mesh bounds
3. This collision model interferes with physics!

**Solution**: Disable physics on gizmo entities:
```cpp
// Option 2: Set null collision model after creation
entity->setCollisionModel(nullptr);
```

See: `AbstractEntity.cpp:updateEntityProperties()` for auto-AABB creation logic.
