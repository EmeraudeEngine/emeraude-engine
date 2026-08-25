# Coordinate System Convention

**CRITICAL:** The entire engine uses ONE world convention, consistently, in ALL systems — physics,
rendering, scene graph, audio, input.

## The convention (right-handed, Y-UP)

```
From the player/camera perspective in world space:

+Y (UP)       -Z (FORWARD)
   ↑         ↗
   |       /
   |     /
   |   /
   | /
   └────────→ +X (RIGHT)

• X-axis: +X = RIGHT,  -X = LEFT
• Y-axis: +Y = UP,     -Y = DOWN
• Z-axis: +Z = BACK,   -Z = FORWARD
```

This is the convention of **glTF 2.0, USD and FBX**, which is why scene imports are the identity: no
rotation, no mirror, no per-asset flag.

## Why Y-UP, and why the previous convention was broken

The engine used to be **Y-DOWN** (`+Y = DOWN`, `-Y = UP`) with `-Z` forward, on the rationale that
it matched Vulkan's clip space and therefore cost nothing. It did cost something: **it rendered a
mirror image**, measured in Aug 2026 with the debug compass.

The criterion is that a frame is non-mirrored if and only if
`(screen-right) × (screen-up) = (toward the viewer)`. Under Y-down: screen-right `= +X`,
screen-up `= -Y`, toward-the-viewer `= +Z`, and `X × (-Y) = -Z ≠ +Z`. The triad
`(right, down, back)` is physically LEFT-handed, while every cross product in the engine
(`CartesianFrame::rightVector()`, the rotation matrices, the tangent frames) treats the basis as
right-handed. Mixing a left-handed frame with right-handed algebra reverses every chiral result.

`Matrix::perspectiveProjection()` confirmed it arithmetically: `[Col1Row1] = +a` (no Y flip,
Vulkan-style NDC) together with `w = -z_eye` (OpenGL-style -Z eye space) gave an eye→NDC map of
`diag(+, +, −)` — negative determinant.

**The fix, applied:** flip Y in the projection (`[Col1Row1] = -a`), with `orthographicProjection()`
moving in lockstep (it feeds the CSM cascades, every shadow map and every 2D/cubemap target), and
delete every local compensation the mirror had accumulated. The other coherent option — keeping
Y-down and making `+Z` the forward axis — was rejected: Y-up also aligns the engine with every asset
format it consumes.

### What the mirror had left in the tree, and what happened to it

Each of these was worked around locally instead of being recognised as one root cause. All are now
**deleted**, not kept:

| Compensation | Fate |
|---|---|
| Triangle-winding swap (indices 1↔2) in glTF/FBX/USD loaders | deleted, with their `computeTriangleNormal(true)` → `(false)` partners |
| 180° X import rotation in `SceneDataConsumer` | deleted in both branches — the import is the IDENTITY |
| `Scenes::Loaders::AxisFlip` + `LoaderOptions::swapX/swapY/swapZ` per-asset flip | deleted; no asset needs a flag |
| Editor gizmo arrows authored on NEGATIVE axes | authored POSITIVE (`src/Scenes/Editor/AGENTS.md`) |
| Cubemap sampling negating Y (`StandardResource`, `LightGenerator`, `SSR`, `RTGI`, `RTR`) | deleted (`docs/reflection-pipeline.md`) |
| `ShapeGenerator` mirror-wound faces (16 sites + 8 loop-driven generators) | rewound CCW around each declared outward normal |
| **Point-light shadow cubemap lookup** `vec3(-d.x, d.y, d.z)` (`LightGenerator::generate3DShadowMapCode()` + its PCF twin) | **MISSED at migration time, fixed Aug 2026** — now the plain `-d` |
| **RTR environment fallback** `vec3(reflDir.x, -reflDir.y, reflDir.z)` (`Effects/Framebuffer/RTR.cpp`) | **MISSED at migration time, fixed Aug 2026** — now the raw world direction |

> [!NOTE]
> **Both misses were found by looking for the wrong thing, then the right thing.** The migration
> hunted "cubemap sampling negating **Y**" and cleaned SSR, RTGI, the skybox and the material
> reflections. The shadow lookup escaped because its stale compensation negated **X**, not Y; the
> RTR fallback escaped because it lives in the **miss path only** — rays that escape the scene —
> so it is invisible unless a smooth reflective surface faces open sky with readable vertical
> structure.
>
> The sweep that found RTR did not search for Y negations: it searched for **any hardcoded
> per-component sign flip on a direction** across the whole shader codegen and effect sources, then
> checked each hit against the contract. Use that shape of search, not the symptom-shaped one.
>
> Measured on `reflexion-debug --demo-options 0,5,0` with a clear-horizon sky, on the mirror
> sphere's top (where rays leave toward the sky): luminance 86.3 → 105.1 and blue-minus-green
> −2.8 → +32.5, while the real sky, the real ground and the sphere's BOTTOM (which uses the hit
> path and must not move) stayed put to within 0.8 and 0.1 respectively.

> [!CAUTION]
> **The shadow-cubemap lookup was missed, and the way it was missed is the lesson.** The migration
> checked the cubemap machinery on the reflection probes (`reflexion-debug`) and separately measured
> that the change moved almost no shadow pixels — 3329 out of 4665600 on `light-and-shadow-debug` —
> and recorded "shadows are unaffected". That sentence was TRUE and it bought weeks of false
> confidence: it said the change did not MOVE the shadows, never that the shadows were RIGHT. Only
> the cubemap RENDER is shared with the probes; the shadow LOOKUP is shared with nothing, and it
> still carried an X-only negation dating from 0.8.5.
>
> The symptom, once someone looked at the right scene: on `global-illumination` the walking
> paladin's shadow was cast on the **CEILING**, with none under his feet. `light-and-shadow-debug`
> is a poor detector for it — its 100 000 lux directional Sun drowns the single 800 lm omni — while
> `global-illumination` has exactly ONE shadow-casting omni light and shows it immediately.
>
> Two rules come out of this: **a measurement of the form "X did not change Y" is not a verification
> that Y is correct**, and **pick the scene from the failure mode** — validating a point-light
> shadow needs a scene where a point light is the only caster.

⚠️ The winding swap was justified by a **false statement** in those loaders: *"the 180° X rotation
inverts the winding"*. A rotation has determinant +1 and NEVER inverts winding. The swap was
compensating the mirror. Do not reintroduce that reasoning.

`WADLoader` is the one exception that KEEPS its winding swap: Doom geometry is baked with **Z
negated** (positions AND normals), which makes that bake determinant +1, so its swap is a genuine
property of the bake and not a mirror compensation.

### The measurement that proves it, reproducible

Two compass poses are needed — **neither alone is sufficient**, and using only one is what produced a
bogus conclusion once already:

| pose | before the flip | after |
|---|---|---|
| control — look `(+X, 0, +Z)` | X+ at x=544 LEFT, Z+ at x=2335 RIGHT, y=810 | **bit-identical** |
| diagonal — pitch toward `+Y` | X+/Z+ rise to y=172 | **fall to y=1447** |

So screen-right is unchanged and screen-up went from `-Y` to `+Y`, giving
`screen-right × screen-up = (-1,0,-1)/√2 = -look`, which is what a non-mirrored render requires. It
was `+look` before. The control pose being UNCHANGED is the half that matters most: a vertical flip
cannot move a horizontal reading, so if it had moved, something other than the projection Y sign had
been touched.

Protocol: `Core.SceneManagerService.toggleCompass()` (⚠️ **not** `keyPress` — console key injection
never reaches Core-level bindings), then `Act.setPosition` / `Act.lookAt`. The console `lookAt` never
rolls, which is what closes the computation.

## Movement Examples (Player/Object perspective)

- **Moving upward**: `velocity.y > 0` (toward +Y)
- **Falling downward**: `velocity.y < 0` (toward -Y)
- **Moving right**: `velocity.x > 0` (toward +X)
- **Moving left**: `velocity.x < 0` (toward -X)
- **Moving forward**: `velocity.z < 0` (toward -Z)
- **Moving backward**: `velocity.z > 0` (toward +Z)

## Normal Vector Examples

- **Ground normal** (pointing up): `(0, +1, 0)`
- **Ceiling normal** (pointing down): `(0, -1, 0)`
- **Right wall normal** (pointing left): `(-1, 0, 0)`
- **Left wall normal** (pointing right): `(+1, 0, 0)`
- **Front wall normal** (pointing back): `(0, 0, +1)`
- **Back wall normal** (pointing forward): `(0, 0, -1)`

## Physics Examples

- **Gravity direction**: `EnvironmentPhysicalProperties::DownDirection = {0, -1, 0}` — the single
  place the physics loop learns which way is down. Gravity is a **vector**, not a signed scalar.
- **Jump impulse**: POSITIVE Y value (pushes up)
- **Forward thrust**: negative Z value (pushes forward)

## Camera pitch

A **POSITIVE pitch looks UP**, a negative pitch looks DOWN (standard convention). A `lookAt` target
placed above the eye has a positive `deltaY` and yields a positive angle directly — no negation
anywhere. `AbstractLiving::setHeadPitch()` (projet-alpha) is the single site where any head pitch is
applied, and therefore the single site where the ±90° clamp is enforced.

## Implementation Guidelines

### DO
- Use the Y-up convention in all calculations, in every subsystem
- Use POSITIVE Y for "upward", negative Z for "forward"
- Ask `CartesianFrame::upwardVector()` / `downwardVector()` rather than hard-coding a Y sign
- Read the gravity direction from `EnvironmentPhysicalProperties`, never assume it

### DON'T
- Never flip Y coordinates anywhere in the engine
- Never reintroduce a per-asset axis flip: the import is the identity, and if an asset looks
  mirrored the cause is elsewhere
- Don't mix conventions across subsystems

### Example Code

```cpp
// Correct: Jump impulse (upward movement)
entity.addForce({0.0F, jumpForce, 0.0F});

// Correct: Forward movement
entity.addForce({0.0F, 0.0F, -forwardForce});

// Correct: Ground normal for collision
Vector< 3, float > groundNormal{0.0F, 1.0F, 0.0F};

// Correct: Camera looking forward and slightly down
Vector< 3, float > cameraDirection{0.0F, -0.2F, -1.0F};
```

## Winding Conventions for Parametric Geometry

**Front faces wind COUNTER-CLOCKWISE around their outward normal** — `VK_FRONT_FACE_COUNTER_CLOCKWISE`,
which is the pipeline default (`GraphicsPipeline.cpp`). The `emitTriangle(A, B, C)` helpers in
`ShapeGenerator` emit vertices in **natural A/B/C order**: the historical B/C swap was a mirror
compensation and is gone. Callers list each face CCW around its own outward normal.

⚠️ **Verify a winding by computing it, never by eye**: take the face's cross product and check it
against the generator's own declared outward normal. That is how the 16 + 8 reversals of the Y-up
migration were each validated — and it is now an automatic gate,
`loopDrivenGeneratorsWindCCWAroundTheirNormals` in emeraude-base, covering every loop-driven
generator with `cuboid`/`plane`/`triangle` as controls.

⚠️⚠️ Two traps make a naive version of that check lie, both documented in
[`emeraude-base/src/VertexFactory/AGENTS.md`](../dependencies/emeraude-base/src/VertexFactory/AGENTS.md)
§ *Winding convention*: triangles straddling a **normal discontinuity** carry no evidence (ignoring
them produced 13 false positives on the arrow alone), and the check is only valid while the normals
stay **independent of the winding** — pinned by `theWindingCheckRejectsAMirroredShape`.

### Vertical authoring for primitives

Primitives are authored **Y-up**: a cone/arrow/dome points `+Y`, a disk and a plane face `+Y`, a
tetrahedron's apex is at `+Y`, `setCenterAtBottom()` rests the shape on `Y=0` extending toward `+Y`.

✅ **The twelve gem cuts are authored Y-up too (Aug 2026), and `convertYDownAuthoring()` is DELETED.**
They used to author their facet math with the table (or the rose cut's flat base) toward `-Y` and
mirror the finished shape at the end. All eleven generators now write the Y-up frame directly.

⚠️⚠️ **Removing that mirror had FOUR consequences, not one** — the recipe is worth knowing before
re-authoring anything else: the `Y` literals negate; every face swaps its first two vertices AND
their UVs; each cross product feeding a per-face normal swaps its operands, since
`cross(M·e₁, M·e₂) = −M·cross(e₁, e₂)`; and **the bitangent** of the per-face tangent frame flips —
miss that last one and `v` silently becomes `1 − v`. Full account and the two measuring traps it
exposed: [`emeraude-base/src/VertexFactory/AGENTS.md`](../dependencies/emeraude-base/src/VertexFactory/AGENTS.md)
§ *The gem cuts are authored Y-UP*. Witness: the `parametric-geometries` demo.

### Patterns for Ring-Based Geometry

Ring vertices are generated CCW in the XZ plane (using `cos(θ), sin(θ)`). Two standard patterns
exist for connecting rings — note that the *pattern* is about which ring is which, and the Y-up
authoring decides which one is on top:

**Crown/Table pattern** (small ring toward the display side, large ring away from it):
```cpp
normal = cross(inner[next] - inner[i], outer[i] - inner[i]);
emitTriangle(inner[i], outer[i], outer[next]);
emitTriangle(inner[i], outer[next], inner[next]);
```

**Pavilion pattern** (large ring toward the display side, small ring away from it):
```cpp
normal = cross(inner[i] - outer[i], outer[next] - outer[i]);
emitTriangle(outer[i], inner[i], inner[next]);
emitTriangle(outer[i], inner[next], outer[next]);
```

**Table fan** (flat polygon facing the display side):
```cpp
normal = cross(ring[1] - ring[0], ring[n-1] - ring[0]);
emitTriangle(ring[0], ring[i], ring[i+1]);  // for i=1..n-2
```

**Culet/base fan** (flat polygon facing the opposite side):
```cpp
normal = cross(ring[n-1] - ring[0], ring[1] - ring[0]);  // reversed
emitTriangle(ring[0], ring[i+1], ring[i]);  // reversed winding
```

### Critical Notes
- ⚠️⚠️ **"The crown pattern normal may point inward for small faces (acceptable for refractive gems)"
  — this line used to stand here, and it legitimised a real defect.** Measured Aug 2026: it was not
  "small faces" but **11 of the 12 cuts**, the princess cut having *every* facet normal pointing into
  the solid, so those facets were lit as if facing away. Fixed in `emeraude-base`: a flat facet's
  normal is now DERIVED from the triangle emitted, which makes the drift structurally impossible.
  Gated by `gemCutsWindAndFaceOutward`.
  ⚠️ Their **winding was correct throughout** — judging a gem against its own normals therefore
  reports the exact opposite conclusion, and did. See
  [`emeraude-base/src/VertexFactory/AGENTS.md`](../dependencies/emeraude-base/src/VertexFactory/AGENTS.md)
  § *The gem cuts are the exception*.
- The pavilion pattern normal uses the diamond's `cross(culet - girdle, girdleTangent)` convention
- See `ShapeGenerator.hpp:generateDiamondCutGem()` for the reference implementation — its vertical
  math is now plain Y-up, with no conversion step to keep in mind
- A strip generator's emission order is NOT interchangeable between generators: the sphere and torus
  need `[0]/[2]/[1]/[3]` while the cylinder needs `[0]/[1]/[2]/[3]`, because their ring
  parameterizations differ. Recompute, never harmonize.

## Cross-System Consistency

This coordinate system is used consistently across:

1. **Physics System**: all forces, velocities and collision normals
2. **Rendering System**: all vertex positions, transformations and camera matrices
3. **Scene Graph**: all node positions and transformations
4. **Audio System**: all 3D positional audio calculations
5. **Input System**: all spatial input mappings (mouse, gamepad)

⚠️ **Screen/pointer space is NOT world space**: pointer Y grows DOWNWARD. A mouse-look controller
must negate it to get a world pitch (`Player::onPointerMove`, projet-alpha).

## Migration Notes

When porting content or code from a Y-DOWN source (including this engine's own pre-Aug-2026 code,
and any Doom/WAD-era tooling):

1. **Negate Y** on world positions, velocities, forces and vertical offsets
2. **Swap AABB corner Y values** so `maximum.Y >= minimum.Y` — ⚠️ `AACuboid::set()` silently
   reorders out-of-order corners, so a half-done edit neither fails to compile nor asserts: it just
   puts the origin at the wrong end
3. **Update normal vectors** to use POSITIVE Y for "up"
4. **Leave UVs, colors, 2D/overlay coordinates, radii, sizes and angles alone** — only world
   verticals move
5. **Test on a SLOPE, not on flat ground**: flat ground grounds correctly even with a wrong
   `Grid::normal`, so it proves nothing

> [!CAUTION]
> **THE FREEZE LIST — verify these, never "convert" them.** Six sites were already written Y-UP
> before the flip, which repaired them by accident. A conscientious sweep for Y literals WILL "fix"
> them and break them a second time:
> `BasicSeaResource::isSubmerged` / `getDepthAt` / `getNormalAt`, the fallback in
> `BasicGroundResource::getNormalAt`, `NodeController`'s D-Pad Up/Down, and the Player/Drone/Sun Y
> values in the two scene JSONs of the data store.
> **Never "fix" a Y literal you meet in isolation.** Establish what frame the surrounding code is
> actually in first — several defects of this migration were comments asserting Y-down over code
> that was already correct.

> [!CAUTION]
> **The `Test/512x512square-c` measurement texture is authored Y-DOWN** and prints `Top (-Y)` /
> `Bottom (+Y)`. With the UVs correct it now shows `Top (-Y)` at the top, which is FALSE.
> Use it for **orientation** — are the glyphs legible, upright, unmirrored? — and **never for axis
> naming**. Re-authoring the asset to `Top (+Y)` / `Bottom (-Y)` is an owner decision, still open.

## What the flip actually cost, and how it was proved

Recorded because the migration's own tracking file has been deleted and these are the numbers, not
the narrative. Everything below is measured, never inferred.

| Check | Result |
|---|---|
| Compass, control pose (look `(+X,0,+Z)`) | **bit-identical** before and after — a vertical flip cannot move a horizontal reading |
| Compass, diagonal pose (pitched toward `+Y`) | X+/Z+ **fall** to y=1447 where they used to **rise** to y=172 |
| FOV round-trip (`coordinates-debug`, pose C) | view distance 10000 → 2500 moves **2.27 %** of pixels; restoring it returns a **bit-identical** image. A vertical flip would have moved **99.97 %** |
| Physics: rest on flat ground | a 1 m sphere settles at **exactly Y = 1.000000**, velocity 0, grounded on `"Ground"` |
| Physics: slope, generated terrain | balls rest at all different elevations and every tracked one descends; `\|v\|` stays 0.2–1.8 instead of accelerating toward 9.81, so the motion is surface-constrained, not a fall through |
| Player eye height | body Y = 241.618469, eye Y = **243.263474** → offset **+1.645005**, against `DefaultPlayerHeight` 1.75 × 0.94 = **1.645**, eye ABOVE body |
| Sponza inscription | reads *FALLERE NOSTRA VETANT ET FALLI PONDERA MEQVE …* correctly, letters unmirrored, with **NO per-asset flag** |
| Paladin | sword in the RIGHT hand, shield on the LEFT: the canonical right-handed knight, so not mirrored |

⚠️ **The control pose is the half that matters most**, and the same shape of argument recurs
throughout: a check that cannot FAIL for the right reason proves nothing. If pose C had moved,
something other than the projection's Y sign had been touched; if the FOV round-trip had shown no
difference at all, the change had not reached the renderer and the test would have been vacuous.

⚠️ Reproducing the Sponza witness: `setPosition(-6.072286, 4.461862, 0.853897)` then
`lookAt(-15.878870, 3.595881, -0.901378)`.
