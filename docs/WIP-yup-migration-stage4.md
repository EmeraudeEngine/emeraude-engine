> [!CAUTION]
> **TEMPORARY FILE.** It exists only while the Y-up migration is mid-flight, so the work can be
> picked up on another machine. DELETE IT in the commit that completes stage 4, and fold whatever
> is still true into `docs/coordinate-system.md`.
>
> ⚠️⚠️ **THIS MAP GOES STALE FASTER THAN THE CODE.** On 2026-08-25 four of its entries described
> defects that had already been fixed, and one overstated the remaining work. Acting on them would
> have sent a session chasing ghosts. **Before acting on ANY item here, verify it against the code —
> and correct the item in the same pass.** An item is a lead, never a finding.

# Y-up migration — stage 4 COMMITTED as work-in-progress

The flip is **incomplete but committed** on all three repositories, so the tree is clean and work
proceeds **step by step, one measured defect at a time**. It builds `-Werror` clean and
`EmeraudeBaseUnitTests` is 1987/1987 green.

⚠️⚠️ **Green does NOT mean correct here.** Three of the four defect classes this migration deals with
(vertex coordinates, UV pairing, declared normals) are invisible to the compiler AND to the unit
suite. A passing build is the floor, never the evidence.

## The core result, measured and archived

The renderer is **no longer a mirror**. Two compass poses prove it together — neither alone would:

| pose | before the flip | after |
|---|---|---|
| C — look `(+X, 0, +Z)`, control | X+ x=544 LEFT, Z+ x=2335 RIGHT, y=810 | **bit-identical** |
| diagonal — pitch toward `+Y` | X+/Z+ rise to y=172 | **fall to y=1447** |

So `screen-right` is unchanged and `screen-up` went from `-Y` to `+Y`, giving

    screen-right x screen-up = (-1,0,1)/sqrt(2) x (0,1,0) = (-1,0,-1)/sqrt(2) = -look

which is what a non-mirrored render requires. It was `+look` before. Captures:
`compass_frameC_afterFlip.png`, `compass_diag_afterFlip.png`.

The control being UNCHANGED is the half that matters most: a vertical flip cannot move a horizontal
reading, so if pose C had moved, something other than the projection Y sign had been touched.

## Done

- `Matrix::perspectiveProjection` `[Col1Row1] = -a`; `orthographicProjection` scale AND bias negated
- `CartesianFrame`: `m_downward` -> `m_upward` (53 sites), `downwardVector()`/`upwardVector()` bodies
  and return types swapped, `computeYAxis` renamed and re-documented
- `EnvironmentPhysicalProperties::DownDirection` = `{0,-1,0}` — the only place the physics loop
  learns which way is down
- `Scene.physics.cpp`: three duplicated ground blocks now ask for the `minY*` AABB corners; the
  surface test is `normal[Y] < -threshold`; `velocity[Y] <= 0.1F`
- `ConstraintSolver`: the two ground tests SWAPPED (not each inverted — they are a pair)
- `SceneDataConsumer`: the 180 degree X import rotation deleted in BOTH branches; import is identity
- Winding swaps deleted in GLTF/FBX/USD + their `computeTriangleNormal(true)` -> `(false)`
- WADLoader: Z negated in the bake (positions AND normals) making it det +1, winding swap KEPT
- Cubemap Y negations deleted: `StandardResource`, `LightGenerator`, `SSR`, `RTGI`, and `RTR` whose
  `cubeDir()` helper was deleted outright rather than left as an identity
- **The SIXTH and last one, `Material/Helpers.cpp` `checkPrimaryTextureCoordinates` — the skybox
  DISPLAY path (Aug 2026, measured).** The generated vertex shader is now
  `sv3DTexCoord0 = normalize(va_Position)` instead of `normalize(vec3(P.x, -P.y, P.z))`.
  It was absent from the list above, and its absence was VISIBLE: deleting five of six left the
  displayed sky Y-flipped **with respect to the lighting derived from it** (the IBL irradiance is
  sampled with the raw world normal), so the bright zenith of a sky lit the scene from the zenith
  while the screen showed the nadir face there. Measured in `coordinates-debug` with `AxisDebug` +
  the compass: BEFORE, the magenta `Y-` face sat at the zenith against a green compass sphere and
  the four side faces were rotated 180°; AFTER, all six face colours equal their compass sphere
  (`X+` red, `X-` cyan, `Y+` green, `Y-` magenta, `Z+` blue, `Z-` yellow) and labels stand upright.
  ⚠️ The `-Y` face needs the camera BELOW the ground (`Act.setPosition(0,-30,0)`) — the ground
  plane masks it otherwise. Captures: `scratchpad/base_*.png` vs `fix_*.png`.
  ⚠️ **Residual, NOT a defect and NOT to be "fixed" at a sampling site**: each face stores the
  view taken from OUTSIDE the cube, because the standard Vulkan cube-face mapping is left-handed
  while this world is right-handed (`screen-right = look × up`). The equirectangular loader
  already bakes it in, and the packed assets are authored for it — **measured, not assumed**:
  the ring seam test (`docs/caution-points.md`) gives 0.17–1.87 MAD for the outside pairing
  against 6.22–56.85 for the inside one on 9 of the 11 packed cubemaps (the 2 others are
  degenerate assets, not failures).
  `AxisDebug.Packed.png` was the lone exception — authored from the inside, labels drawn legible
  in the editor. **Owner re-authored it (Aug 2026) and it is now CLOSED**: the six axes are
  measured green, colour AND legibility. Its glyphs now read backwards in an image editor, which
  is the correct state for a cube face asset.
  ⚠️ Pole captures: apparent rotation is set by the camera YAW; declare the yaw or the reading
  is worthless. See the discriminator (rotation vs mirror) in `docs/caution-points.md`.
- `CelestialBody` zenith `{0,+1,0}`
- `IndexedVertexResource`/`VertexResource`: geometric `flipYAxis` removed, `flipV` KEPT
- `ShapeGenerator`: 16 winding reversals, each verified by computing the face cross product against
  the generator's own declared outward normal
- `ShapeGenerator` **UV pairing, both `generateCuboid` overloads**: `V` negated on all six faces,
  measured on screen against `Test/512x512square-c` in the `coordinates-debug` scene. See the new
  defect class below.
- `Grid::normal`: all four accumulation argument orders swapped, plus the degenerate fallback
- The four `MathMatrixYConventionPin` tests flipped deliberately; 8 `MathCartesianFrame`
  expectations updated

## NOT done — the remaining work

1. **`ShapeGenerator` loop-driven generators — ✅ WINDING MEASURED CORRECT AND LOCKED (2026-08-25).**
   This item claimed `generateSphere`, `generateCylinder`, `generateDisk`, `generateTorus`,
   `generateCapsule`, `generateHemisphere`, `generateArrow` and `generateTube` were *"still
   mirrored"*. **They are not.** Measured mechanically — `cross(B-A, C-A)` against each triangle's
   own authored vertex normals — **all nine come out 100% CCW around their outward normal.**
   No geometry was changed.

   The work was therefore to **LOCK** them, since nothing covered them:
   `loopDrivenGeneratorsWindCCWAroundTheirNormals` gates all nine, plus `cuboid`/`plane`/`triangle`
   as **CONTROLS** — a probe that fails a control is a broken probe, not a discovery.
   Full rules: `emeraude-base/src/VertexFactory/AGENTS.md` § *Winding convention*.

   ⚠️⚠️ **Skipping the normal-coherence filter reported 13 FALSE POSITIVES on `generateArrow` alone**,
   reading exactly like a real mirror defect. Triangles straddling a normal discontinuity carry no
   evidence. With the filter the arrow is 15/15 clean.

   ⚠️ **This item was about WINDING only. It never implied the UV class — see 3b, measured separately.**
2. **`generatePlane` — ✅ DONE (verified 2026-08-25).** It was a DIFFERENT failure mode from item 1:
   its winding was already correct for Y-up and only the declared normal was stale. It now reads
   `enableGlobalNormal(positiveY())` and says so in place. Pinned by
   `planeTopFaceGrowsVWithPositiveZ`, cross-checked against the cuboid by
   `cuboidTopFaceAgreesWithPlane`.
   ⚠️ The only `negativeY()` calls left in `ShapeGenerator` are LEGITIMATE: the bottom face of both
   `generateCuboid` overloads, and `generateHemisphere`'s cap normal. Do not sweep them.
3. **Y-down content authoring** in `ShapeGenerator`, pervasive: `generateTriangle`'s apex at `-Y`,
   `generateTetrahedron`'s "Y- is up", `generateOctahedron`'s top pyramid. Not winding, actual
   vertex coordinates.
   ✅ **RESOLVED — MEASURED 2026-08-25, the item was indeed stale.** Not on the strength of the
   comments (which this file rightly distrusts) but by reading the produced VERTEX COORDINATES:

   | generator | Y extent | vertices at max Y | verdict |
   |---|---|---|---|
   | `generateTriangle` | −0.433 … **+0.433** | **1** (against 2 at the bottom) | apex at **+Y** |
   | `generateTetrahedron` | −0.333 … **+1** | 3 (against 9) | apex at **+Y** |
   | `generateOctahedron` | −1 … **+1** | 4 / 4 | symmetric, no bias |
   | `generateCone` | 0 … **+2** | 17 / 17 | apex at **+Y** |

   (The counts exceed the topological vertex count because flat shading duplicates a shared apex
   once per face — 3 faces meet at the tetrahedron's apex, 16 + 1 at the cone's.)

   ⚠️⚠️ **The method is the part worth keeping.** `generateCuboid` carried `/* Top face (Y+) */`
   labels through the entire Y-down era while its UV data was Y-down: the labels described intent,
   the data described the old convention. **Check the coordinates, never the comment.**
3b. **UV pairing — a FOURTH defect class, missing from this plan until now.** Distinct from winding,
   from vertex coordinates and from declared normals: the `setTextureCoordinates` call still carries
   the `V` it had when `-Y` was up, so the texture renders upside down on geometry that is otherwise
   perfectly correct. ⚠️ **It compiles clean, it passes 1987/1987 tests, and no geometric assertion
   can see it** — the shape, the normals and the winding are all right. The only detector is a
   legible test texture on screen, or a probe over the generated vertices. `generateCuboid` (both
   overloads) is done and measured.

   ✅ **MEASURED 2026-08-25 — this class is LARGELY CLOSED, contrary to how this item used to read.**
   A probe over the generated shapes (mean `V` of vertices above vs below mid-height, vertical faces
   only) shows `V` DECREASING with `Y` — the Y-up pairing — on **`generateCylinder`, `generateTube`,
   `generateCone` and `generateCapsule`** (V ≈ 0.125 top against 0.75 bottom); a pole probe puts
   **`generateHemisphere`** at V = 0 on its `+Y` pole against V ≈ 0.89 at its rim. **Nothing to do on
   those five.**

   ⚠️⚠️ **`generateTorus` and `generateDisk` are OUT OF SCOPE BY CONSTRUCTION, not "unverified".** A
   torus's `V` runs periodically around the tube cross-section, and a disk is horizontal so its `Y`
   is constant. A `V`-versus-`Y` test on either is **VACUOUS** and will report whatever the sampling
   order happens to give — the first version of this probe called both "INVERTED" on exactly that
   artefact. Do not add such a test for them, and do not read an old one as evidence.

   🔴 **`generateSphere` — a SEPARATE, PRE-EXISTING defect, NOT a Y-up residue.** Its texture
   coordinates are TRANSPOSED: `texCoordU` advances per STACK (latitude) while `texCoordV` advances
   per SLICE (longitude), so each lands in the other's slot, and
   `textureCoordinates[1] = {texCoordU - deltaV, ...}` even mixes `deltaV` into the `U` term. That is
   why a `V`-versus-`Y` probe reads a flat 0.5/0.5 on a sphere: it is measuring longitude. Fixing it
   is its own task with its own visual check — **do not fold it into the Y-up sweep.**

   Rules in `dependencies/emeraude-base/src/VertexFactory/AGENTS.md` § "Texture coordinate convention".
   `generateHollowedCube` is out of scope by construction (UVs parameterised per beam, not by
   world `Y`).
4. **Terrain — ✅ DONE (2026-08-25, data store `2c53edd`).** `"Inverse": true` removed from
   `projet-alpha.data` `demo.json` and `terrain_demo.json`; the code semantics in
   `TerrainResource.cpp` and `BasicGroundResource.cpp` are UNTOUCHED, as decided.
   **Why one side only:** `Inverse` merely negates the displacement scale
   (`inverse ? -scale : scale`) — it means "flip the height map", not "flip Y", so it carries no
   convention and needed no migration. What moved is the WORLD: under Y-down a NEGATIVE displacement
   was what pushed terrain upward, so both scenes set the flag to get hills; under Y-up a positive
   scale already raises the ground (`Grid::m_pointHeights`, positive = higher), so the flag now digs
   the terrain out. ⚠️ Editing BOTH sides would have cancelled out and left the terrain inverted —
   that is the whole point of the "change ONE side only" note.
   Verified: both files still parse as JSON, they were the only two occurrences in the data store,
   and no C++ site sets the flag programmatically. Visual acceptance pending (hills, not a mould).
5d. **Rendering INTO a cubemap — done (Aug 2026). THREE coupled pieces, a site nothing in this
   plan named.** `ViewMatrices3DUBO::CubemapOrientation` (each face on its own axis, ups NEGATED),
   `updatePerspectiveViewProperties()` (the Y-up projection flip UNDONE for cube faces), and
   `GraphicsPipeline::configureRasterizationState(..., mirroredViewport = isCubemap())` (front face
   inverted). They are one mechanism; changing one alone breaks the cubemap.
   **Root cause**: the cube-face convention is LEFT-handed (`right x up = +look`) while a camera is
   right-handed. Measured: with honest look/up vectors all six faces come out with the OPPOSITE
   right vector, and no choice of `up` can fix it — `up -> -up` ROTATES a face, it never MIRRORS it.
   Reproduce with `reflexion-debug --demo-options 0,3,0`. ⚠️ **The only usable mode**: 1/2/5 reflect
   a SKY, which has no left/right landmark, so a mirror is invisible there — that blind spot cost a
   whole wrong bisection this session.
   ⚠️ Diagnostic that identified the missing mirror: the palm TRUNK stayed left while its CROWN
   crossed right. A GLOBAL mirror moves the whole tree; a PER-FACE one cuts it in two.
   ⚠️ Blast radius: point-light SHADOW cubemaps share the mechanism. Measured on
   `light-and-shadow-debug`: 3329/4665600 differing pixels, i.e. unaffected.
   ⚠️ Exonerated, do not re-chase: material reflection, IBL split-sum, SSR, RTR. All three sampling
   paths take the raw world-space `reflect(I, N)` — the correct contract.

5a. **The JUMP — done (Aug 2026). It was NOT the jump force.** The force was already `{0,+power,0}`
   and correct; `isGrounded()` was true; free-fly was off. The killer was
   `MovableTrait::updateSimulation()`'s micro-bounce clamp, which zeroes the vertical velocity of a
   grounded body: it tested `m_linearVelocity[Y] > 0.0F` (Y-down: positive = sinking), so under Y-up
   it clamped every UPWARD velocity — and a body is still grounded on the frame it pushes off.
   Now `< 0.0F`.
   ⚠️ **The diagnostic signature is the reusable part**: velocity[Y] read exactly ONE frame's worth
   of impulse (`cyclesLeft × 0.03`) while the position never moved. That says "velocity wiped
   between frames", not "force missing", and it points at the consumer rather than the producer.
   ⚠️ Console `keyPress()` reaches **neither** Core bindings **nor** the Player's `onKeyPress` —
   the trap is wider than documented. Triggering `jump()` from code was the only way to exercise it.
   Also confirmed on the way: the player's foot-anchored AABB is correct (released at Y=2, settles
   at exactly Y=0), which closes the "camera at 0.94x height ABOVE the feet" half of gate point 3.

5c. **MDx loaders (MDL/MD2/MD3/MD5) — done (Aug 2026). They were in NEITHER winding list above,
   and had been missed entirely.** `emeraude-base` `FileFormatMDx.hpp`. The id Tech → engine
   transform was `(y,-z,x)`, a reflection (det -1); it is now `(y,z,x)`, a rotation (det +1).
   ⚠️ Three things hung off that sign and only two of them were compensations:
   the vertex-normal negation after `computeVertexTBNSpace()` (**deleted** — a rotation does not
   invert the cross product), and the **winding reversal, which is NOT one** — id Tech simply
   stores the opposite winding to the engine's front-face convention, so it is **KEPT**. Removing
   it renders every ID model inside out; that was measured on `boss1.md2`, not assumed.
   ⚠️⚠️ The MD5 conversion existed in **four** places, two of them unnamed: the two helpers, the
   normal negation, and a copy **inlined in the skinning loop** — the one building the visible
   mesh. Fixing the helpers alone left the skinned CyberDemon upside down while MDL/MD2/MD3 were
   already upright. Also fixed in passing: the MDL vertex NORMAL used `(n.z,-n.y,n.x)` while its
   position used `(n.y,-n.z,n.x)` — a pre-existing mismatch unrelated to Y-up.
   Verified per format: `geometry-loader --demo-options 7` (MDL) / `8` (MD2) / `6` (MD5).
   1983/1983 base unit tests pass — **none of them sees any of this**, the check is visual.

5b. **Actor FACING — done (Aug 2026), and it is NOT what the plan expected.** Deleting the
   per-instance `diag(s,-s,-s)` was only half right. That matrix has a POSITIVE determinant: it was
   never a mirror, it was a 180° rotation about **X**, flipping Y *and* Z together. Only the Y half
   was obsolete. The Z half was encoding a real, standing fact: **a character model faces +Z**
   (normative in glTF; the FBX path reaches the same frame via `ufbx_axes_right_handed_y_up`),
   while the engine's forward is **-Z** (the camera convention).
   Symptom once it was gone: the Fox and the Paladins **ran at the player backwards**.
   Fix, in `projet-alpha` `Fox.cpp` / `Paladin.cpp`: aim the node's BACKWARD vector at the target,
   un-negated — which REMOVES two negations per actor rather than adding a compensation.
   ⚠️ Read and write are a PAIR (`backwardVector()` / `setBackwardVector()`); negating one alone
   makes the actor spin away instead of turning.
   ⚠️ **Rejected on purpose: doing this at load time.** A 180° Y rotation in the loaders would make
   every asset fight a standard it already respects, and would re-orient Sponza, Doom and every
   calibrated demo camera. Owner decision.
   ✅ **`Drone` done too (verified 2026-08-25)**: it reads `backwardVector()` and writes
   `setBackwardVector()` un-negated, and aims at `player.headNode()`.
   ⚠️ It had carried a SECOND compensation on top: a `diag(1, 1, -1)` instance matrix, commented
   "the mesh is exported facing +Z instead of -Z" — right observation, wrong conclusion. Its
   determinant is **-1**, a REFLECTION, so it reversed the WINDING as well as the axis: the drone
   rendered inside-out AND backwards from a single sign. Removed. Contrast with Fox/Paladin's
   `diag(s,-s,-s)`, determinant **+1**, a 180° X rotation — a different story entirely.
   **Read the determinant before you read the intent.**
   ⚠️ Aim at `headNode()`, never `baseNode()`: since the AABB is foot-anchored, an actor's origin is
   at ground level. The resulting defect is DISTANCE-DEPENDENT, which makes it deceptive — far away
   the horizontal term dominates and the aim looks perfect; up close the height gap takes over and
   it dips, reading as "it targets the ground".

5. **projet-alpha actor contracts** — the six head-pitch sites that must move together or not at all
   (`AbstractLiving::setHeadPitch` and its atan2 recovery, `Act.cpp` lookAt and getOrientation,
   Player's LookUp/LookDown keys, Player's mouse-look); the jump force; the foot-anchored AABBs in
   `Player.cpp` and `Paladin.cpp` — ⚠️ `AACuboid::set()` silently auto-swaps out-of-order corners, so
   a half-done edit neither fails to compile nor asserts, it just puts the origin at the head; the
   `diag(s,-s,-s)` instance matrices on Fox/Paladin/Liminal reduced to a plain scale.
6. **The freeze list — VERIFY, DO NOT CONVERT.** Six sites are written Y-UP already and this stage
   repairs them by accident: `BasicSeaResource::isSubmerged/getDepthAt/getNormalAt`,
   `BasicGroundResource::getNormalAt`'s fallback, `NodeController`'s D-Pad Up/Down, and the
   Player/Drone/Sun Y values in the two scene JSONs. A conscientious sweep for Y literals WILL
   "fix" them and break them a second time.
7. **Docs — ✅ DONE (2026-08-24), full cascade sweep.** ~60 stale assertions corrected across the
   three repos. Entry points that were still declaring the retired convention: engine `README.md`
   ("Right-handed, Y-DOWN… Gravity is `+Y`") and engine `AGENTS.md` ("**Y-DOWN** is absolute law"),
   plus `graphics-system.md`, `architecture-philosophy.md`, `scene-graph-architecture.md`,
   `troubleshooting.md`, `cpp-conventions.md` and the `AGENTS.md` of Audio, Physics, Scenes, Saphir,
   Graphics, Vulkan and `src/`. `src/Scenes/Loaders/AGENTS.md` lost its ~50-line `AxisFlip` section
   (replaced by a tombstone) and `src/Scenes/AGENTS.md` its "Coordinate System Conversion" block.
   projet-alpha: the § 3b chirality and camera bullets, `docs/caution-points.md` (the six-step
   mirroring recipe → tombstone), `ai_documentation_map.md`, `development-patterns.md`,
   `src/Actor/AGENTS.md`. emeraude-base: `VertexFactory/Grid.hpp`, `docs/ai_documentation_map.md`.
   ⚠️⚠️ **The two worst offenders were the tooling, not the prose:**
   `projet-alpha/.claude/agents/emeraude-code-reviewer.md` and
   `.claude/commands/check-emeraude-conventions.md` *enforced* Y-down — their grep patterns flagged
   `-9.81` and every Y-up comment as a CRITICAL violation. An external AI review run against them
   reported the migrated engine as broken; that is what triggered this sweep.
   ⚠️ Rule applied throughout, and to keep applying: a comment **asserting** "the engine is Y-down"
   is a defect; a comment saying a compensation **belonged to** the Y-down era and is gone is
   deliberate memory that stops the workaround being reintroduced — those were all KEPT.
   🔴 The sweep surfaced **three behavioural residues** — all **✅ FIXED (2026-08-25)**, so the
   `TODO.md` § Y-UP RESIDUES section is gone and its knowledge moved into the AGENTS.md network:
   - **OpenAL listener UP vector** (`Audio/HardwareOutput.cpp`): passed `downwardVector()`, now
     `upwardVector()`. OpenAL follows the OpenGL convention, which IS the engine convention now, so
     both orientation vectors pass verbatim — see `src/Audio/AGENTS.md`.
   - **Atmospheric fog height falloff**: `heightFalloff` is a POSITIVE decay rate and the shader now
     negates it. Silent because the analytic integral is valid for either sign — see
     `src/Graphics/AGENTS.md`.
   - **A FIFTH MD5 conversion site** (`emeraude-base/src/Animation/MD5AnimParser.hpp`) that the
     four-site inventory missed, in a different module: `.md5anim` clips kept the `(y,-z,x)`
     reflection while the mesh path had moved to `(y,z,x)`. Now pinned by a regression test —
     see `emeraude-base/src/VertexFactory/AGENTS.md`.
   ⚠️ Writing that test exposed a **second-order trap worth keeping**: its two halves need DIFFERENT
   discriminating axes. For POSITIONS only md5 Z separates the transforms; for ROTATIONS md5 Z is
   exactly the axis that CANNOT, since conjugating a rotation by a reflection that negates the
   rotation's own axis leaves it unchanged (`S·R(Y,θ)·Sᵀ == R(Y,θ)`). A rotation pin about md5 Z
   passes against the very defect it is meant to catch.
   ⚠️ It also exposed an unrelated defect in the foundation: `Quaternion::rotatedVector()` called
   the MUTATING `conjugate()` instead of `conjugated()`, so it never compiled on a const quaternion
   and had never been called anywhere — while the test named after it silently exercised
   `operator*` instead. Both fixed; see `emeraude-base/src/AGENTS.md`.
8. **The physics gate — points 1, 2 and 3 PASSED (2026-08-25). Points 4 and 5 moved OUT of this
   migration.** Measured with the new `Core.SceneManagerService.getNodePhysics(<node>)`, which
   returns position, velocity, grounded and WHICH surface a body rests on.

   - **1 ✅ a ball on flat ground comes to REST.** `physics-debug`: a 1 m sphere settles at
     **exactly Y = 1.000000** — its own radius, with the ground at Y = 0 — velocity 0,
     `groundedSource = "Ground"`. That numeric read supersedes the earlier pixel-diff evidence.
   - **2 ✅ a ball on a SLOPE rolls DOWNHILL, on generated terrain.** `balls-of-steel`
     (diamond-square ground): sampled balls rest at **all different elevations** (+15.25, +9.57,
     −6.94, −15.24, −18.64, −20.30), so they followed the relief rather than a plane, and every
     tracked ball moves **downward** — one went Y −19.5 → −33.2 over 50 s.
     ⚠️ It is NOT free fall: `|v|` stayed between 0.2 and 1.8 instead of accelerating toward
     9.81 m/s², and contact kept re-arming, so the motion is genuinely surface-constrained rather
     than a body tunnelling through the ground.
   - **3 ✅ the player camera sits at 0.94× height ABOVE the feet.** `terrain` demo: body
     Y = 241.618469, eye Y = **243.263474**, offset **+1.645005**, against `DefaultPlayerHeight`
     1.75 m × 0.94 = **1.645**. Exact, and the eye is ABOVE the body — under Y-down it was below.
   - **4 and 5 — MOVED to the physics chantier (owner decision).** "A box on a StaticEntity reports
     `isGroundedOnEntity()`" and "an entity against the wall reports `isGroundedOnBoundary()` only
     at the FLOOR" (⚠️ that one needs a scene with NO ground, or the ground stops the body long
     before the boundary floor). Neither depends on an axis sign, so neither belongs to this
     migration. See `TODO.md` § PHYSICS.

   ⚠️ **Staging the gate exposed a defect that had nothing to do with Y-up**: a gravity/grounded
   circular lock froze any body that lost its support (fixed, `ddce43e5`). Two further physics
   defects are logged in `TODO.md` § PHYSICS — NaN velocities, and bodies not settling on generated
   terrain. **Do not fold them back into this migration.**

   **What made point 1 possible — the ground blocks were half-migrated.** The Y-up flip moved the
   AABB corner selection to `minY*` and left the ARITHMETIC Y-down in all three duplicated copies
   (`clipAboveGround`, `detectGroundCollision`, `accumulateGroundCorrection`): `corner[Y] - groundLevel`
   measures the CLEARANCE above the ground, not the penetration, so every airborne body reported a
   collision. Same story for the Point test (`position[Y] > groundLevel` meant "below") and the
   Sphere's lowest point (`position[Y] + radius`). Fixed, plus the `{0,+1,0}` contact normals of
   `detectGroundCollision` (the normal points FROM the body INTO the surface, so a ground contact
   is `{0,-1,0}` now that +Y is up) and `clipAboveGround`'s `moveY(-penetration)`.
   ⚠️ **BOUNDARIES NEEDED NOTHING.** The world box is `[-boundary, +boundary]³` and all three axes
   are handled identically — `clipInsideBoundaries` and `accumulateBoundaryCorrection` carry no
   convention at all. The single Y-dependent decision, "which wall is the floor", lives in
   `resolveCollisions` and was ALREADY Y-up correct. Its two comments said "In Y-down" over correct
   code, which is the inverse trap: they have been rewritten to say so explicitly.

   ⚠️ **Residual measured at point 1, NOT a Y-up defect — do not chase it here.** The sphere settles
   with its centre at **Y = 0.897** (three-point null test against the horizon, eye heights 1.0 /
   0.8745 / 0.75, linear at 160 px/m, the three extrapolations agreeing to 0.0005) while its
   silhouette measures a radius of ~0.82 rather than the requested 1.0. So it rests on a collision
   box BIGGER than its mesh and floats ~8 cm. Two suspects, both outside this migration: the
   collision AABB is derived from the visual's local bounding box (see the AABB defect the owner
   reported independently — the barrel's debug box, and `geometry-debug`'s box that only ever GROWS
   when switching objects), and `generateSphere` is still on the item-1 list above. **Cleared by
   reading, do not re-audit**: `AACuboid` default/`reset()`/`merge()`/`isValid()` are all correct
   (default is the inverted-infinite invalid box, merge handles it), and
   `AbstractEntity::updateEntityProperties()` DOES reset the model, conditioned on
   `areShapeParametersOverridden()`.
9. **Content witnesses**: Sponza's inscriptions must read CORRECTLY with no per-asset flag (compare
   `Sponza_inscription_mirrored_stage3.png`); Doom E1M1 must NOT change orientation, only up/down;
   Paladin's bind pose and walk cycle must agree.
10. **The FOV round-trip re-run**: change the view distance at runtime, then re-shoot pose C — it
    must be unchanged. The only check that catches the round-trip if the stage 1 `std::abs()` guard
    were ever dropped.

## Do not

- ⚠️ **The old "never commit a partial stage 4" rule is RETIRED.** Stage 4 is committed as WIP on all
  three repositories; the tree is clean and work proceeds one measured defect at a time. Commit each
  fix with its evidence rather than accumulating an atomic block nobody can review.
- Do not "fix" a Y-up literal you find in isolation: check the freeze list first.
- ⚠️ Do not migrate `ShapeGenerator::generateScreenQuad()`. It pairs `V` with `+Y`, the opposite of
  every world-space generator, **on purpose**: it is the fullscreen NDC quad of the post-processor
  (`Graphics/PostProcessor.cpp:466`) and the overlay manager (`Overlay/Manager.cpp:142`), whose source
  images are already in screen space. It is not a `generateQuad` that someone forgot. Locked by
  `screenQuadPairsVWithPositiveYOnPurpose` so the sweep fails loudly rather than flipping the whole
  post-process chain and the entire overlay.
- ⚠️ Do not read the `Test/512x512square-c` labels as ground truth: the PNG is authored **Y-down**
  and prints `Top (-Y)` / `Bottom (+Y)`. Once the UVs are correct it shows `Top (-Y)` at the top,
  which is now false. Use it for **orientation** (are the glyphs legible, upright, unmirrored?),
  never for **axis naming**. Re-authoring the asset to `Top (+Y)` / `Bottom (-Y)` is an owner
  decision, still open.
- Do not touch `IBLBaker.cpp:211-216`. An audit flagged its `-st.y` terms as a compensation; they
  are the STANDARD Vulkan cube-face direction table (`+X: (1,-t,-s)`, `+Z: (s,-t,1)`, ...), present
  in every reference implementation. Verified and deliberately left alone.
