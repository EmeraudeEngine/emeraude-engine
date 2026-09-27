## Entity Physics Flags

AbstractEntity uses minimal flags for physics control:

| Flag | Purpose | Check Method |
|------|---------|--------------|
| `IsCollisionDisabled` | Skip collision detection | `isCollidable()` (inverted) |
| `IsSimulationPaused` | Skip gravity/drag | `isSimulationPaused()` |

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
