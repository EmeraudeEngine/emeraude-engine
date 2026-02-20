# Coordinate System Convention

**CRITICAL:** The entire engine uses Vulkan's coordinate convention throughout all systems.

## Overview

This convention is used **consistently in ALL engine systems** (physics, rendering, scene graph, audio, etc.).

### Coordinate System (Right-Handed, Y-Down)

```
From the player/camera perspective in world space:

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

## Rationale

This design decision was made to:
- **Eliminate coordinate conversions** between engine and Vulkan API (zero-cost abstraction)
- **Avoid performance overhead** from constant Y-axis flips in critical paths
- **Prevent subtle bugs** caused by mixing conventions across subsystems
- **Maintain consistency and clarity** throughout the codebase

## Important Note

While counter-intuitive (humans naturally think "up = positive"), this convention must be respected in ALL code. **Never introduce Y-axis conversions or assumptions based on traditional math coordinates (Y-up).**

## Movement Examples (Player/Object perspective)

- **Moving upward**: `velocity.y < 0` (toward -Y)
- **Falling downward**: `velocity.y > 0` (toward +Y)
- **Moving right**: `velocity.x > 0` (toward +X)
- **Moving left**: `velocity.x < 0` (toward -X)
- **Moving forward**: `velocity.z < 0` (toward -Z)
- **Moving backward**: `velocity.z > 0` (toward +Z)

## Normal Vector Examples

- **Ground normal** (pointing up): `(0, -1, 0)`
- **Ceiling normal** (pointing down): `(0, +1, 0)`
- **Right wall normal** (pointing left): `(-1, 0, 0)`
- **Left wall normal** (pointing right): `(+1, 0, 0)`
- **Front wall normal** (pointing back): `(0, 0, +1)`
- **Back wall normal** (pointing forward): `(0, 0, -1)`

## Physics Examples

- **Gravity acceleration**: `+9.81` on Y-axis (pulls down, positive Y)
- **Jump impulse**: negative Y value (pushes up, negative Y)
- **Forward thrust**: negative Z value (pushes forward, negative Z)

## Implementation Guidelines

### DO
- Always use the Y-down convention in all calculations
- Respect the coordinate system in physics, rendering, and audio
- Use negative Y values for "upward" movement
- Use negative Z values for "forward" movement

### DON'T
- Never flip Y coordinates anywhere in the engine
- Don't assume traditional math coordinates (Y-up)
- Don't introduce coordinate conversions between subsystems
- Don't mix coordinate conventions in the same codebase

### Example Code

```cpp
// Correct: Jump impulse (upward movement)
entity.applyImpulse(glm::vec3(0.0f, -jumpForce, 0.0f));

// Correct: Forward movement
entity.applyForce(glm::vec3(0.0f, 0.0f, -forwardForce));

// Correct: Ground normal for collision
glm::vec3 groundNormal(0.0f, -1.0f, 0.0f);

// Correct: Camera looking forward and down
glm::vec3 cameraDirection(0.0f, 0.2f, -1.0f);
```

## Winding Conventions for Parametric Geometry

The `emitTriangle` lambda used in all `ShapeGenerator` gem/shape generators **swaps B and C**: calling `emitTriangle(A, B, C)` actually emits vertices in order `(A, C, B)`. This is the engine's winding convention for front-facing triangles.

### Front Face Determination

With the B/C swap, triangles that appear **CW (clockwise) in screen space** after projection are front-facing. This matches `VK_FRONT_FACE_CLOCKWISE` behavior.

### Patterns for Ring-Based Geometry

Ring vertices are generated CCW in the XZ plane (using `cos(θ), sin(θ)`). Two standard patterns exist for connecting rings:

**Crown/Table pattern** (small ring on top Y-, large ring on bottom):
```cpp
// Normal: cross(innerRingTangent, innerToOuter)
normal = cross(inner[next] - inner[i], outer[i] - inner[i]);
// Winding:
emitTriangle(inner[i], outer[i], outer[next]);
emitTriangle(inner[i], outer[next], inner[next]);
```

**Pavilion pattern** (large ring on top, small ring on bottom Y+):
```cpp
// Normal: cross(innerMinusOuter, outerRingTangent) — follows diamond pavilion
normal = cross(inner[i] - outer[i], outer[next] - outer[i]);
// Winding:
emitTriangle(outer[i], inner[i], inner[next]);
emitTriangle(outer[i], inner[next], outer[next]);
```

**Table fan** (flat polygon facing sky Y-):
```cpp
normal = cross(ring[1] - ring[0], ring[n-1] - ring[0]);
emitTriangle(ring[0], ring[i], ring[i+1]);  // for i=1..n-2
```

**Culet/base fan** (flat polygon facing ground Y+):
```cpp
normal = cross(ring[n-1] - ring[0], ring[1] - ring[0]);  // reversed
emitTriangle(ring[0], ring[i+1], ring[i]);  // reversed winding
```

### Critical Notes
- The crown pattern normal may point inward for small faces (acceptable for refractive gems)
- The pavilion pattern normal uses the diamond's `cross(culet - girdle, girdleTangent)` convention
- For dome geometry (Rose cut), use pavilion pattern when dome points downward (+Y)
- See: `ShapeGenerator.hpp:generateDiamondCutGem()` for reference implementation

## Cross-System Consistency

This coordinate system is used consistently across:

1. **Physics System**: All forces, velocities, and collision normals
2. **Rendering System**: All vertex positions, transformations, and camera matrices
3. **Scene Graph**: All node positions and transformations
4. **Audio System**: All 3D positional audio calculations
5. **Input System**: All spatial input mappings (mouse, gamepad)

## Migration Notes

When porting code from other engines or libraries that use Y-up conventions:

1. **Flip Y-axis signs** in movement calculations
2. **Invert gravity direction** (make it positive Y)
3. **Update normal vectors** to use negative Y for "up"
4. **Adjust camera matrices** for Y-down viewport
5. **Test thoroughly** - coordinate bugs can be subtle