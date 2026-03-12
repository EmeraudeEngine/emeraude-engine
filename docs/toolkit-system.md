# Toolkit System

The `Scenes::Toolkit` class (`src/Scenes/Toolkit.hpp`) is the primary scene construction helper. It provides a fluent API for creating entities, attaching components, and building node hierarchies. AI agents should use this as the preferred way to populate scenes.

## Core Concepts

### Cursor

The toolkit maintains a **cursor** (`CartesianFrame`) that defines the position (and optionally orientation) for the next entity created.

```cpp
// Position only (orientation reset to identity)
toolkit.setCursor(x, y, z);
toolkit.setCursor(Vector<3,float>{x, y, z});

// Full frame (position + orientation)
toolkit.setCursor(CartesianFrame<float>{...});

// Read current cursor
const auto & frame = toolkit.cursor();
```

When `GenPolicy::Parent` is active, the cursor is interpreted in the **parent node's local space**.

### Generation Policies

| Policy | Enum | Behavior |
|--------|------|----------|
| Simple | `GenPolicy::Simple` | Creates a standalone entity under the scene root (default) |
| Parent | `GenPolicy::Parent` | Creates the next Node as a **child** of a previously set parent node |
| Reusable | `GenPolicy::Reusable` | Reuses an existing entity for the next component attachment (no new entity created) |

### BuiltEntity Return Type

All `generate*` methods return a `BuiltEntity<entity_t, component_t>` with:

```cpp
builtEntity.entity()     // std::shared_ptr<entity_t> (Node or StaticEntity)
builtEntity.component()  // std::shared_ptr<component_t> (Visual, Camera, Light, etc.)
builtEntity.isValid()    // true if both entity and component were created
```

## Entity Generation

### Empty Entities

```cpp
// Empty node or static entity
auto entity = toolkit.generateEntity<Node>("MyEntity");
auto entity = toolkit.generateEntity<StaticEntity>("MyEntity");

// With lookAt direction
auto entity = toolkit.generateEntity<Node>({0, 0, 0}, "MyEntity");
```

### Mesh Instances

```cpp
// Cuboid (uniform)
auto built = toolkit
    .setCursor(x, y, z)
    .generateCuboidInstance<Node>("Cube", 2.0F, material);

// Cuboid (non-uniform)
auto built = toolkit
    .setCursor(x, y, z)
    .generateCuboidInstance<StaticEntity>("Box", {3.0F, 1.0F, 5.0F}, material);

// Sphere
auto built = toolkit
    .setCursor(x, y, z)
    .generateSphereInstance<Node>("Ball", 1.0F, material);
// Note: generateSphereInstance auto-creates a SphereCollisionModel

// From pre-built renderable
auto built = toolkit
    .setCursor(x, y, z)
    .generateRenderableInstance<Node>("Mesh", renderable);

// From geometry + material
auto built = toolkit
    .setCursor(x, y, z)
    .generateRenderableInstance<Node>("Mesh", geometry, material);

// From VertexFactory shape
auto built = toolkit
    .setCursor(x, y, z)
    .generateRenderableInstance<Node>("Custom", shape, material);
```

All mesh generators automatically:
- Create geometry via `Geometry::ResourceGenerator` (with TBN + UV enabled)
- Create a `SimpleMeshResource` or `MeshResource` (depending on sub-geometry count)
- Attach a `Component::Visual` with lighting enabled by default
- Compute `BodyPhysicalProperties` from geometry dimensions and material density

### Cameras

```cpp
// Perspective camera
auto cam = toolkit
    .setCursor(x, y, z)
    .generatePerspectiveCamera<Node>("MainCam", fov, lookAt, primaryDevice, showModel);

// Orthographic camera
auto cam = toolkit
    .setCursor(x, y, z)
    .generateOrthographicCamera<Node>("OrthoView", size, lookAt, primaryDevice, showModel);

// Cubemap camera (for environment maps)
auto cam = toolkit
    .setCursor(x, y, z)
    .generateCubemapCamera<Node>("EnvCapture", primaryDevice, showModel);
```

### Lights

```cpp
// Directional (sun)
auto light = toolkit
    .setCursor(dirX, dirY, dirZ)
    .generateDirectionalLight<StaticEntity>("Sun", color, intensity);

// Directional with shadow map
auto light = toolkit
    .setCursor(dirX, dirY, dirZ)
    .generateDirectionalLight<StaticEntity>("Sun", color, intensity, shadowRes, coverageSize);

// Directional with CSM (Cascaded Shadow Maps)
auto light = toolkit
    .setCursor(dirX, dirY, dirZ)
    .generateDirectionalLight<StaticEntity>("Sun", color, intensity, shadowRes, cascadeCount, lambda, csmScale);

// Point light
auto light = toolkit
    .setCursor(x, y, z)
    .generatePointLight<StaticEntity>("Bulb", color, radius, intensity, shadowRes);

// Spotlight
auto light = toolkit
    .setCursor(x, y, z)
    .generateSpotLight<StaticEntity>("Spot", lookAt, innerAngle, outerAngle, color, radius, intensity, shadowRes);
```

### Render Target Generators

```cpp
// Camera + 2D offscreen texture
auto [cam, texture] = toolkit
    .setCursor(x, y, z)
    .generateTexture2DRenderer<Node>("SecurityCam", fov, lookAt, precision, distance, showModel);

// Camera + cubemap offscreen rendering (for environment reflections)
auto [cam, cubemap] = toolkit
    .setCursor(x, y, z)
    .generateEnvironmentCubemapRenderer<Node>("EnvMap", precision, distance, showModel);
```

### Modifiers

```cpp
auto modifier = toolkit
    .setCursor(x, y, z)
    .generateSphericalPushModifier<StaticEntity>("WindSource", magnitude);
```

## Node Hierarchy Creation

The `setParentNode()` method is the key to building parent-child Node trees through the toolkit.

### How It Works

1. `setParentNode(parentEntity)` sets the generation policy to `GenPolicy::Parent`
2. The next `setCursor()` positions in the **parent's local space**
3. The next `generateEntity<Node>()` (or any generate*<Node>) creates the node as a child
4. Chain calls to build deeper hierarchies

### Example: Articulated Arm

```cpp
// Base at world origin
const auto base = toolkit
    .setCursor(0.0F, -1.0F, 0.0F)
    .generateCuboidInstance<Node>("ArmBase", {2.0F, 2.0F, 2.0F}, metalMat);
base.entity()->yaw(Radian(30.0F), TransformSpace::Local);

// Segment 1: child of base, offset in local X
const auto seg1 = toolkit
    .setParentNode(base.entity())
    .setCursor(4.0F, 0.0F, 0.0F)
    .generateCuboidInstance<Node>("ArmSeg1", {3.0F, 1.0F, 1.0F}, metalMat);
seg1.entity()->pitch(Radian(45.0F), TransformSpace::Local);

// Segment 2: child of segment 1
const auto seg2 = toolkit
    .setParentNode(seg1.entity())
    .setCursor(4.0F, 0.0F, 0.0F)
    .generateCuboidInstance<Node>("ArmSeg2", {3.0F, 1.0F, 1.0F}, metalMat);
seg2.entity()->pitch(Radian(-30.0F), TransformSpace::Local);

// IMPORTANT: Reset after building hierarchy
toolkit.clearGenerationParameters();
```

### Reusable Entity (Multiple Components)

Use `setReusableNode()` or `setReusableStaticEntity()` to attach multiple components to the same entity:

```cpp
// Create entity with a visual
const auto entity = toolkit
    .setCursor(x, y, z)
    .generateCuboidInstance<Node>("MultiComponent", 2.0F, material);

// Attach a light to the same entity (no new entity created)
toolkit
    .setReusableNode(entity.entity())
    .generatePointLight<Node>("MultiComponent/Light", color, radius, intensity);
```

### Reset

Always call `clearGenerationParameters()` after completing a hierarchy chain or reusable sequence:

```cpp
toolkit.clearGenerationParameters();
```

This resets:
- Node generation policy to `Simple`
- Static entity generation policy to `Simple`
- Previous node/static entity references
- Cursor frame

## Utility Methods

```cpp
// Simple colored material (no texture, flat color)
auto mat = toolkit.getColoredMaterialResource(PixelFactory::Red);

// Random coordinates
auto coords = toolkit.generateRandomCoordinates(count, min, max);

// Debug mode (forces visual debug on all created entities)
toolkit.enableDebug(true);

// Access underlying managers
auto & resources = toolkit.resourceManager();
auto scene = toolkit.scene();
```

## Important Notes

- **Template parameter defaults to `StaticEntity`** — always specify `<Node>` explicitly for dynamic entities
- **`generateSphereInstance`** auto-creates a `SphereCollisionModel` — other generators do not auto-create collision models
- **Renderable scale**: If a renderable has `uniformScale() != 1.0`, the toolkit automatically applies it to the entity's local scale
- **Sub-geometry dispatch**: Geometries with `subGeometryCount() > 1` get wrapped in `MeshResource`, others in `SimpleMeshResource`
- **Cursor reset**: `setCursor()` resets the cursor's orientation to identity (only position changes). Use the `CartesianFrame` overload to set both position and orientation.