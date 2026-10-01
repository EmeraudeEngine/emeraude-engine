## Entity Physics Flags

AbstractEntity uses minimal flags for physics control:

| Flag | Purpose | Check Method |
|------|---------|--------------|
| `IsCollisionDisabled` | Skip collision detection | `isCollidable()` (inverted) |
| `IsSimulationPaused` | Skip gravity/drag | `isSimulationPaused()` |
| `IsCollisionAuthored` | `setCollidable()` decided the collision state | (internal) |

### Who decides `isCollidable()` (2026-09-28)

- **The author, when they say so.** `setCollidable(bool)` is FINAL: it raises `IsCollisionAuthored` and
  `onComponentsUpdated()` never derives the state again. A change notifies the scene
  (`onContentModified()`), which inserts the entity in the physics octree or ERASES it from it
  (`Scene::onEntityContentModified()`).
- **Otherwise, the mass.** Without an authored decision, `onComponentsUpdated()` derives it: collidable
  when at least one component declares a non-null mass.
- ⚠️ **A static solid needs NO mass — it needs `setCollidable(true)`.** The constraint solver reads an
  inverse mass of 0 for every static and non-movable body (`SoftStepSolver`, physics overhaul P2), so a wall is infinitely heavy
  whatever it declares. Before 2026-09-28 the only way in was a FICTITIOUS mass (`GameLogic`'s
  buildings, `Liminal`'s `solidStone()`), and `onComponentsUpdated()` overwrote `setCollidable()` both
  ways on every component update: `citadel` was entirely walk-through, and a cloud made non-solid
  would have turned solid the moment a component with a mass joined it.
- ⚠️ The erasure happens on the content notification ONLY, never in
  `checkEntityLocationInOctrees()`: that one runs on every frame for every moving node, and `erase()`
  walks the whole tree (an expanded root holds no element to test first).
- ⚠️ `setCollidable()` notifies through `shared_from_this()`: call it on an entity already owned by a
  shared pointer (after the Toolkit or the scene created it).
- **The contact material of a massless solid** (owner decision, same day): declare it with
  `BodyPhysicalProperties::contactMaterial(bounciness, stickiness)` on the component. When NO
  component has a mass, `onComponentsUpdated()` averages the bounciness and stickiness of the
  components that shape the collider (`contributesToEntityExtents()`); it used to leave the defaults
  (0.5 / 0.5) whatever was declared — the other half of what the fictitious mass carried
  (`Liminal`'s stone: bounciness 0.05). A mass of 0 now has an inverse of 0 in the constructor and in
  `setMass()` (it was +inf).

**Removed flags** (now derived from actual state):
- ~~`HasBodyPhysicalProperties`~~ → Use `bodyPhysicalProperties().mass() > 0`

**Physics participation conditions:**
```cpp
hasMovableAbility()                    // Node (not StaticEntity)
&& !isSimulationPaused()               // Simulation active
&& getMovableTrait()->isMovable()      // Movement enabled
&& hasCollisionModel()                 // Has collision primitive
&& isCollidable()                      // Collision not disabled
```

See: `AbstractEntity.hpp:IsCollisionDisabled`, `AbstractEntity.hpp:IsSimulationPaused`
