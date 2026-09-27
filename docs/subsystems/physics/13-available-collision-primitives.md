## Available Collision Primitives

The physics system uses collision primitives from `Base/Math/Space3D`:

| Primitive | Description | Use Case |
|-----------|-------------|----------|
| `Sphere` | Center + radius | Simple entities, particles |
| `Capsule` | Axis segment + radius | Characters, elongated objects |
| `AACuboid` | Axis-aligned box | Static objects, triggers |
| `Triangle` | 3 vertices | Terrain mesh collision |

**Capsule** is ideal for character collision:
- Better fit for humanoid shapes than spheres
- Handles slopes and stairs naturally
- See: `@src/Libs/AGENTS.md` → "Math/Space3D: Capsule Primitive"
