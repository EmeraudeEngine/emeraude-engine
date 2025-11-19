# ADR-001: Y-Down Coordinate System

## Status
**Accepted** - Implemented throughout the entire engine

## Context

When developing a 3D graphics engine, there are two common coordinate system conventions:
- **Y-Up**: Traditional mathematical convention where Y increases upward (+Y = up, -Y = down)
- **Y-Down**: Computer graphics convention where Y increases downward (+Y = down, -Y = up)

Different subsystems in a game engine may naturally tend toward different conventions:
- **Physics**: Traditionally uses Y-up (gravity = -9.81)
- **Rendering**: Mixed (OpenGL is Y-up, Vulkan's viewport is Y-down by default)
- **UI/2D**: Naturally Y-down (screen coordinates flow downward)
- **Audio**: Often follows the graphics system

Mixing conventions within an engine leads to:
- Constant coordinate conversions between subsystems
- Performance overhead from Y-axis flips in critical paths
- Subtle bugs when forgetting to convert coordinates
- Cognitive load for developers switching between subsystems

## Decision

**Emeraude Engine adopts Y-down coordinate system consistently across ALL subsystems.**

**Coordinate Convention:**
```
+Y (DOWN)     -Z (FORWARD)
   ↓         ↗
   |       /
   |     /
   |   /
   | /
   └────────→ +X (RIGHT)

• X-axis: +X = RIGHT,  -X = LEFT
• Y-axis: +Y = DOWN,   -Y = UP  
• Z-axis: +Z = BACK,   -Z = FORWARD
```

**Implementation Guidelines:**
- Physics: Gravity = +9.81 along Y-axis, jump impulse = negative Y
- Graphics: All matrices, transforms, and projections configured for Y-down
- Audio: 3D positioning uses Y-down coordinates
- Scene Graph: All transformations and hierarchies use Y-down
- UI/Overlay: Natural fit with screen coordinates

## Consequences

### Positive
- **Zero-cost abstraction**: No coordinate conversions between engine and Vulkan API
- **Performance**: Eliminates Y-axis flips in critical rendering paths
- **Consistency**: Single coordinate system throughout entire engine
- **Bug prevention**: No mixing of coordinate conventions
- **Natural 2D/UI**: Screen coordinates flow naturally downward
- **Vulkan alignment**: Matches Vulkan's native viewport orientation

### Negative
- **Counter-intuitive**: Developers must think "down = positive Y" (opposite of math)
- **Physics adaptation**: Gravity and forces must use positive Y values
- **External library integration**: May require coordinate conversion when interfacing with Y-up libraries
- **Migration burden**: Porting from other engines requires coordinate system changes
- **Documentation overhead**: Must clearly document and enforce convention

### Neutral
- **Industry variance**: Both conventions are valid and used in production engines
- **Performance trade-off**: Gains from no conversions vs potential confusion from non-standard physics

## Implementation Notes

**Critical Code Patterns:**
```cpp
// CORRECT: Jump impulse (upward movement)
entity.applyImpulse(glm::vec3(0.0f, -jumpForce, 0.0f));

// CORRECT: Ground normal for collision  
glm::vec3 groundNormal(0.0f, -1.0f, 0.0f);

// CORRECT: Gravity acceleration
const float GRAVITY = +9.81f;  // Positive Y (downward)
```

**Enforcement:**
- Code review checks for Y-coordinate flips
- Documentation emphasizes Y-down throughout
- Test cases verify coordinate consistency
- Static analysis could detect common Y-flip patterns

## Related ADRs
- ADR-002: Vulkan-Only Graphics API (alignment with Vulkan's Y-down viewport)
- ADR-007: Physics Four-Entity Architecture (Y-down gravity implementation)

## References
- `docs/coordinate-system.md` - Complete coordinate system specification
- `src/Physics/AGENTS.md` - Physics system Y-down implementation
- `src/Graphics/AGENTS.md` - Graphics Y-down matrix configurations