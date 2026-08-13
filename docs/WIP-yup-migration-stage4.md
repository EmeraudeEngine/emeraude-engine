> [!CAUTION]
> **TEMPORARY FILE.** It exists only while the Y-up migration is mid-flight, so the work can be
> picked up on another machine. DELETE IT in the commit that completes stage 4, and fold whatever
> is still true into `docs/coordinate-system.md`.

# Y-up migration — stage 4 in progress, NOT committed

The working tree across the three repositories is **half-flipped**. It builds `-Werror` clean and
`EmeraudeBaseUnitTests` is 1975/1975 green, but the flip is INCOMPLETE and nothing is committed,
because stage 4 is atomic by construction. `stage4-working-tree.txt` lists the 27 modified files.

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

1. **`ShapeGenerator` loop-driven generators, still mirrored**: `generateSphere`, `generateCylinder`,
   `generateDisk`, `generateTorus`, `generateCapsule`, `generateHemisphere`, `generateArrow`,
   `generateTube` (`generateCone` and `generateAssscherCutGem` delegate and inherit the decision).
   ⚠️ The audit found the whole file was authored mirror-wound, not just the sites the plan named —
   twelve `emitTriangle` copies plus two `emitFace`, not three.
2. **`generatePlane` is a DIFFERENT failure mode** — do not sweep it with the others. Its winding is
   already correct for Y-up; the stale part is the hard-coded `enableGlobalNormal(negativeY())`.
3. **Y-down content authoring** in `ShapeGenerator`, pervasive: `generateTriangle`'s apex at `-Y`,
   `generateTetrahedron`'s "Y- is up", `generateOctahedron`'s top pyramid. Not winding, actual
   vertex coordinates.
   ⚠️ **This item is partly stale — RE-VERIFY, do not trust it as written.** As of Aug 2026 the three
   named sites all carry Y-up authoring: `generateTriangle`'s apex is at `+Y`, `generateTetrahedron`
   says "Y+ is up: the apex sits at the +Y pole", `generateOctahedron` says "Top pyramid (Y+ is up)".
   ⚠️⚠️ **But a comment is not evidence in this file.** `generateCuboid` carried `/* Top face (Y+) */`
   labels through the entire Y-down era while its UV data was Y-down — the labels described intent,
   the data described the old convention. Check the vertex coordinates themselves, or put the object
   on screen in `geometry-debug`; never close this item on the strength of a comment.
3b. **UV pairing — a FOURTH defect class, missing from this plan until now.** Distinct from winding,
   from vertex coordinates and from declared normals: the `setTextureCoordinates` call still carries
   the `V` it had when `-Y` was up, so the texture renders upside down on geometry that is otherwise
   perfectly correct. ⚠️ **It compiles clean, it passes 1975/1975 tests, and no geometric assertion
   can see it** — the shape, the normals and the winding are all right. The only detector is a
   legible test texture on screen. `generateCuboid` (both overloads) is done and measured;
   **every other hand-authored generator must be re-read for this**, and it must be checked
   independently of the winding audit, which cannot reveal it. Rules in
   `dependencies/emeraude-base/src/VertexFactory/AGENTS.md` § "Texture coordinate convention".
   `generateHollowedCube` is out of scope by construction (UVs parameterised per beam, not by
   world `Y`).
4. **Terrain**: the `Inverse` flag in `TerrainResource.cpp:282` and `BasicGroundResource.cpp:215`,
   and `"Inverse": true` in `projet-alpha.data` `demo.json:15` / `terrain_demo.json:16`. Decision
   taken: remove it from the two JSONs, leave the code semantics. Change ONE side only.
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
   🔴 `Drone.cpp:216` still carries the old assumption — same defect by construction, NOT verified
   on screen.

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
7. **Docs**: `docs/coordinate-system.md` rewritten for Y-up, the root `AGENTS.md` chirality and
   camera bullets, `.claude/rules/build-and-run.md` and `actors.md`, `src/Scenes/Loaders/AGENTS.md`
   (it still documents the deleted `AxisFlip`).
8. **The physics gate, five points. Point 1 PASSED (Aug 2026), four to go**: a ball dropped on flat
   ground comes to REST on it ✅ — `physics-debug`, a 1 kg sphere and a barrel released at Y = +25
   both fall and settle, and two captures **6 s apart differ by 0 pixels out of 4 665 600**: no
   jitter, no sink, no drift; a ball on a SLOPE rolls DOWNHILL — on generated terrain specifically,
   because flat ground passes even with `Grid::normal` wrong; the player camera sits at 0.94x height
   ABOVE the feet; a box on a StaticEntity reports `isGroundedOnEntity()`; an entity against the +Y
   wall reports `isGroundedOnBoundary()` only at the FLOOR (⚠️ that last one needs a scene with NO
   ground: the ground stops the body long before it can reach the Y = -boundary floor plane).

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

- Do not commit a partial stage 4. Either finish it or revert the 27 files as a unit.
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
