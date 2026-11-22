## A ray through the physics world — `Scene::castRay()` (2026-11-22)

`Scene::castRay(origin, direction, maxDistance, ignored)` returns what a ray meets FIRST in the scene's physics
world, as a `Scene::RayHit` — the entity (null for the ground), the point, the surface's normal toward the origin, the
distance — or nothing. Asked for a direct-hit weapon of projet-alpha (a laser); it is a line of sight as well.

Owner decision (2026-11-22): the ray is the ENGINE's, on the physics world — not a test a game writes against its
own actors (a shot would then go through the walls and the ground).

### What it meets

The same shapes the bodies collide with, and nothing else:

| | How |
|---|---|
| the ground (`GroundLevelInterface::visitTriangles()`) | its triangles, met from ABOVE only, as for the bodies |
| a triangle mesh (a static solid, a kinematic mesh) | its FRONT faces only — both for a two-sided mesh |
| a convex shape: a capsule (a kinematic character), a box, a sphere | a point swept by the solver's narrow phase (`NarrowPhase::sweepCore()`) |
| what has no collision model (a visual, a decorative tree), what is not collidable | not met |
| `ignored` (the entity that casts the ray) | not met |
| a shape the ray STARTS inside | not met: the ray leaves it |

The ray is followed in stretches of 32 m, each with its own small box of triangles to visit (a diagonal ray of 200 m
spans a box of 2 hectares): the first stretch that meets something ends it. The entities the whole ray may meet are
gathered once (every entity of the physics octree whose box overlaps the ray's). It is cut at `MaxRayDistance`
(10 km). A value that is not finite, a zero direction, a distance that is not positive: nothing.

### ⚠️ A triangle is met EXACTLY, never by the bodies' swept point

The first version swept a point against every triangle with the narrow phase, as the continuous step does for a fast
body. That sweep advances CONSERVATIVELY (GJK, the base's `castRay()` in `Math/Space3D/Casts/ShapeCast.hpp`) and
gives up after its iterations. Measured in `city` (2 m quads): a ray of 32 m, 18° below the horizon, went THROUGH the
road — 38 triangles visited, the road's never "swept", and the only two answers were the BACK of the plinth's base
12 m lower, refused as back faces. A steep ray at 6 m and an oblique one at 14 m were met; the same ray to the lawn
and to a sidewalk went through.

A triangle is now met by the exact meeting of a segment and a triangle (the base's `Space3D::isIntersecting(Segment,
Triangle, Point)`: T. Möller, B. Trumbore, "Fast, Minimum Storage Ray-Triangle Intersection", Journal of Graphics
Tools 2(1), 1997), front face by the sign of direction · normal. The seven rays of that test are all met: the road at
15.97 m (twice, one of them exactly along an edge of the mesh), 14.35 m, 5.91 m and 5.96 m; the lawn at 10.29 m; a
sidewalk at 7.17 m. The convex shapes keep the swept point: their distance is exact, the advance converges at once
(a capsule met at 132.0 m).

⚠️ The bodies' continuous step still sweeps its triangles the conservative way: right for a body that moves a
fraction of a metre in a step, not for a ray — never route a long cast through it.

### Thread

The thread that runs the scene's logic, OUTSIDE the physics step: it takes the physics octree's lock. Called from
inside the step (a handler run under it) it is refused with an `[Error]` — the step holds that lock on the same
thread, the call would wait for itself. The step's own events (`NodeCollision`, the characters' `Landed` / `Struck`)
are emitted after it: a ray may be cast from them.

### Not done

- No filter but `ignored` (no layer, no "bodies only").
- A plain segment–triangle test is not watertight in floating point along a shared edge; a ray exactly on a mesh
  edge was met in the test above, a leak is possible in principle.
- One ray a call: nothing is batched, the candidates are gathered at each call (a weapon's shot, not a per-pixel
  query).
