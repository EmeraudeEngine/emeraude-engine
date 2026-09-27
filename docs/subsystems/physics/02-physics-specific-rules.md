## Physics-Specific Rules

### CRITICAL Coordinate Convention
- **Y-UP mandatory** in all physics calculations
- Gravity: a **VECTOR**, not a signed scalar — `EnvironmentPhysicalProperties::DownDirection{0,-1,0}` scaled by the surface gravity magnitude (`Physics::Gravity::Earth = 9.807`). Read the direction from the environment properties, never hard-code a Y sign
- Jump impulse: POSITIVE Y value (pushes upward)
- Forward thrust: negative Z value

### Entity Types (4 distinct types)
1. **Boundaries**: Game constraints (invisible walls)
2. **Ground**: Hybrid physical surfaces (stability + realism)
3. **StaticEntity**: Static objects with defined mass
4. **Nodes**: Full dynamic entities (via MovableTrait)

### Node-Centric Correction Philosophy

**CRITICAL**: Only Nodes receive corrections. All other entity types are passive influences.

| Collision Type | Who is corrected? | Influence |
|----------------|-------------------|-----------|
| Node ↔ Boundary | Node only | Infinite mass (100% absorption) |
| Node ↔ Ground | Node only | Infinite mass (100% absorption) |
| Node ↔ StaticEntity | Node only | Infinite mass (100% absorption) |
| Node ↔ Node | Both Nodes | Mass-proportional distribution |

**Mental model**: Instead of "resolve collision between A and B", think:
> "What correction to apply to THIS Node, given what it touches?"

The Node is the active subject; other entities are parameters influencing the correction.

### Correction Priority Order

**CRITICAL**: Process corrections from MOST constraining to LEAST constraining.

```
1. Boundaries    → ABSOLUTE constraints (world limits, inviolable)
2. Ground        → BASE constraints (where entities can exist)
3. StaticEntity  → FIXED obstacles (walls, rocks, structures)
4. Node ↔ Node   → DYNAMIC interactions (negotiable, flexible)
```

**Why this order matters**:
- If Node↔Node corrected BEFORE Ground → Node could be pushed INTO the ground
- If Ground corrected BEFORE Boundaries → Node could be pushed OUT of the world
- Each pass respects constraints established by previous passes
- Later passes adapt to the "remaining space" after hard constraints

### Physics Execution Pipeline (`Scene.physics.cpp:simulatePhysics()`)

Both phases walk the physics octree with `OctreeSector::forEachSector()` — every sector that owns
elements, inner nodes included — and follow its pairing contract (`src/Scenes/AGENTS.md` § Octree
storage and traversal): at each sector, `owned × owned` and `owned × inherited`, nothing else. ⚠️ No
pair hash set, no "corrected" set: the traversal produces every geometrically possible pair exactly
once. The leaf-only walk with a dedup set cost 23 ms per tick on 121 nodes (2026-09-03).

**Phase 1: Static Collisions** (per-movable accumulation, at the sector that OWNS the movable)
1. Accumulate boundary corrections (if the owning sector touches the world border)
2. Accumulate ground corrections (track separately for grounded state)
3. Accumulate StaticEntity corrections: inherited statics + `forTouchedSector(aabb)` over the
   sector's subtree (a body straddling two child sectors meets the statics of BOTH)
4. Apply combined position correction
5. Apply velocity bounce + set grounded state with source

**Phase 2: Dynamic Collisions** (Node ↔ Node)
1. Iterate every octree sector owning elements; pair owned × owned-after and owned × inherited
2. Skip non-movable entities; skip pairs where **both** are simulation-paused
3. Test movable pairs via `detectCollisionMovableToMovable()`
4. Collect ContactManifolds
5. Resolve via `ConstraintSolver::solve()` (Sequential Impulse)
6. Re-clip involved entities to boundaries

**Sleep/Wake behavior:** Nodes at rest are paused by `checkSimulationInertia()`. A paused
Node is still a solid body — active entities (with velocity) collide against it normally.
Only pairs where both are paused are skipped (optimization for scenes with many resting
objects, e.g., settled balls). When a collision impulse is applied, `addForce()` resumes
simulation automatically via `pauseSimulation(false)`.

See: `Scene.physics.cpp:simulatePhysics()`
