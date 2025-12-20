# Libs (Libraries)

Context for developing Emeraude Engine utility libraries.

## Module Overview

**Engine foundation** - Platform-agnostic utility libraries providing base concepts, mathematics, data manipulation, and external integrations. All engine systems depend on Libs for uniformity.

## Libs-Specific Rules

### Philosophy: Critical Agnostic Foundation
- **Engine foundation**: All systems (Graphics, Physics, Audio, Scenes) use Libs
- **Agnostic**: NO dependencies on high-level systems (Scenes, Physics, Graphics, etc.)
- **Uniformity**: Provide common types and concepts for the entire engine
- **Reusable**: Generic code, not specific to any use case

### Architecture by Domain

**Algorithms/** - Useful multimedia algorithms
- Simple implementations of classic algorithms
- Optimized for real-time usage
- **DiamondSquare**: Procedural terrain heightmap generation (see below)

**Compression/** - Compression/decompression abstraction
- Standardized data compression logic
- Wrappers: zlib, lzma
- Common interface for all algorithms

**Debug/** - Stats and development tools
- Profiling, timings, statistics
- Debug helpers

**GamesTools/** - Game utility classes
- Gameplay-specific helpers
- Generic game concepts

**Hash/** - Hashing algorithms
- MD5, SHA, and other classic algorithms
- For checksums, identifiers, caching

**IO/** - Generic file read/write
- Cross-platform I/O abstractions
- Archive support (ZIP via external lib)
- File/folder manipulation

**Math/** - Complete 2D/3D math library
- **Vector**: 2D/3D/4D vectors
- **Matrix**: Transformation matrices
- **Quaternion**: 3D rotations
- **CartesianFrame**: Coordinate system (position + orthonormal basis)
- **Primitives**: Point, Line, Segment, Sphere, Capsule, Triangle, AACuboid
- **Collision/Intersection**: Geometric detection between primitives
- **Bezier curves**: Smooth interpolation
- All current and future 2D/3D math logic

**Network/** - Web logic generalization
- Network protocol helpers
- Communication abstractions

**PixelFactory/** - Image manipulation
- Load/save image formats
- Pixel transformations (resize, crop, filters)
- Procedural image generation
- **TextProcessor**: Text rendering on Pixmap with bounds protection
- **Pixmap**: Image container with `blendPixel()` (assert) and `blendFreePixel()` (bounds-safe)

**VertexFactory/** - 3D geometry manipulation
- Procedural mesh generation
- Geometric transformations
- Normal, tangent, UV calculations
- **Grid**: Terrain height/normal queries with edge clamping (see below)

**WaveFactory/** - Audio manipulation - See [`@WaveFactory/AGENTS.md`](WaveFactory/AGENTS.md)
- Audio format loading (WAV, FLAC, OGG via libsndfile)
- Procedural audio from JSON definitions (mono Synthesizer, multi-track via SFXScript)
- MIDI to synthesized audio conversion
- Audio sample transformations

**Time/** - Temporal management
- Chronometers
- Timers
- Timing helpers via objects and interfaces

**General concepts (Libs/ root)**:
- **Observer/Observable**: Event pattern
- **Versioning**: Version management
- **JSON**: Fast JSON parsing/writing
- **Flags**: Binary flag management
- **Traits**: Helpers (NamingTrait, etc.)
- **Strings**: String manipulation
- **TokenFormatter**: Case style detection and conversion (camelCase, snake_case, PascalCase, etc.)
- **ThreadPool**: High-performance thread pool
- **KVParser**: INI-style file parsing (sections, key-values)
- **SourceCodeParser**: Source code parsing with annotations and formatting

### Integrated External Dependencies
- **libsndfile**: Audio format loading (WaveFactory)
- **zlib**: Compression (Compression)
- **lzma**: Compression (Compression)
- **ZIP library**: Archives (IO)

## Development Commands

```bash
# Libs tests
ctest -R Libs
./test --filter="*Libs*"

# Tests by category
./test --filter="*Math*"
./test --filter="*IO*"
./test --filter="*ThreadPool*"
./Release/EmeraudeUnitTests --gtest_filter="TokenFormatter*"
```

## Important Files

### Math (critical)
- `Vector.hpp` - 2D/3D/4D vectors
- `Matrix.hpp` - Transformation matrices
- `Quaternion.hpp` - 3D rotations
- `CartesianFrame.hpp` - Coordinate system with orthonormal basis
- `Bezier.hpp` - Bezier curves

### Math/Space3D Primitives
- `Point.hpp` - 3D point
- `Line.hpp` - Infinite line (origin + direction)
- `Segment.hpp` - Finite line segment (start + end)
- `Sphere.hpp` - Sphere (center + radius)
- `Capsule.hpp` - Capsule/Stadium solid (axis segment + radius)
- `Triangle.hpp` - Triangle (3 vertices)
- `AACuboid.hpp` - Axis-aligned bounding box

### Math/Space3D Collisions
- `Collisions/SamePrimitive.hpp` - Same-type collisions (Sphere-Sphere, Capsule-Capsule, etc.)
- `Collisions/CapsulePoint.hpp` - Capsule vs Point
- `Collisions/CapsuleSphere.hpp` - Capsule vs Sphere
- `Collisions/CapsuleCuboid.hpp` - Capsule vs AABB
- `Collisions/CapsuleTriangle.hpp` - Capsule vs Triangle (terrain mesh)

### Math/Space3D Intersections
- `Intersections/LineCapsule.hpp` - Line-Capsule raycasting
- `Intersections/SegmentCapsule.hpp` - Segment-Capsule intersection

### General Concepts
- `Observer.hpp` / `Observable.hpp` - Event pattern
- `ThreadPool.hpp` - High-performance thread pool
- `JSON.hpp` - JSON manipulation
- `FlagTrait.hpp` - Flag management
- `NamableTrait.hpp` - Naming trait
- `TokenFormatter.hpp` - Case detection/conversion (zero-allocation design)
- `KVParser.hpp` - INI-style parser (KVVariable, KVSection, KVParser)
- `SourceCodeParser.hpp` - Source code parser with annotations

### Factories
- `PixelFactory/` - Image manipulation
- `VertexFactory/` - Geometry generation/manipulation
- `WaveFactory/` - Audio manipulation

## Development Patterns

### Math Usage Throughout Engine
```cpp
// Graphics uses Math
Matrix4 projectionMatrix = Math::perspective(fov, aspect, near, far);
Vector3 cameraPos = camera.cartesianFrame().position();

// Physics uses Math
Vector3 force = mass * acceleration;
Quaternion rotation = Quaternion::fromEuler(pitch, yaw, roll);

// Audio uses Math
Vector3 soundPos = emitter.cartesianFrame().position();
float distance = (listenerPos - soundPos).length();

// Entire engine unified via Libs/Math
```

### Observer/Observable Pattern
```cpp
// Observable in Resources
class TextureResource : public ResourceTrait, public Observable {
    void finishLoading() {
        // ...
        notifyObservers(Event::Loaded);
    }
};

// Observer in Scene
class Scene : public Observer {
    void onNotify(Observable* source, Event event) override {
        if (event == Event::Loaded) {
            // React to loading
        }
    }
};
```

### ThreadPool for Async Tasks
```cpp
// Global or local pool
ThreadPool pool(std::thread::hardware_concurrency());

// Submit tasks
auto future1 = pool.enqueue([](int x) { return x * 2; }, 42);
auto future2 = pool.enqueue([](){ loadHeavyResource(); });

// Wait for results
int result = future1.get();  // 84
future2.wait();  // Wait for loading completion
```

### TokenFormatter for Case Conversion
```cpp
// Static methods for direct conversion
std::string snaked = TokenFormatter::toSnakeCase("myVariableName");  // "my_variable_name"
std::string pascal = TokenFormatter::toPascalCase("my_variable_name");  // "MyVariableName"

// Instance for multiple conversions (parses once, converts many times)
TokenFormatter formatter("XMLParser");
formatter.toSnakeCase();       // "xml_parser"
formatter.toCamelCase();       // "xmlParser"
formatter.toKebabCase();       // "xml-parser"
formatter.toTitleCase();       // "Xml Parser"
formatter.toScreamingSnake();  // "XML_PARSER"

// Style detection
CaseStyle style = TokenFormatter::detect("myVar");  // CaseStyle::CamelCase
std::string_view name = TokenFormatter::styleName(style);  // "camelCase"

// Supported styles: CamelCase, PascalCase, SnakeCase, ScreamingSnake,
// KebabCase, TrainCase, FlatCase, UpperFlatCase, LowerSpaced, UpperSpaced, TitleCase
```

**Zero-allocation design:**
- Internal buffer (128 chars max) stores token copy
- Words stored as `std::string_view` into buffer (max 8 words)
- Only output methods allocate (with `reserve()`)
- See: `TokenFormatter.hpp:MaxWords`, `TokenFormatter.hpp:MaxTokenLength`

### KVParser for INI Files
```cpp
// Read configuration file
KVParser parser;
if (parser.read("config.ini")) {
    // Access section (creates if not exists)
    auto& graphics = parser.section("Graphics");

    // Read variables with type conversion
    int width = graphics.variable("width").asInteger();
    bool fullscreen = graphics.variable("fullscreen").asBoolean();
    float gamma = graphics.variable("gamma").asFloat();
    const std::string& path = graphics.variable("path").asString();

    // Check if variable exists
    if (graphics.variable("vsync").isUndefined()) {
        // Variable not found
    }
}

// Write configuration
KVParser config;
config.section("main").addVariable("version", KVVariable{"1.0"});
config.section("Graphics").addVariable("width", KVVariable{1920});
config.section("Graphics").addVariable("fullscreen", KVVariable{true});
config.write("output.ini");
```

**INI file format:**
```ini
[SectionName]
key = value
another_key = 123

# Comment lines (ignored)
@ Header lines (ignored)
```

**Code references:**
- `KVParser.hpp:KVVariable` - Variable with type conversions (bool, int, float, double, string)
- `KVParser.hpp:KVSection` - Named variable collection
- `KVParser.hpp:KVParser` - Main parser with sections
- Uses `std::filesystem::path` for file operations (C++17+)
- Uses `std::string_view` for read-only parameters

### Adding a New Lib (Rules)
```cpp
// FORBIDDEN: Depending on high-level systems
#include "Scenes/Node.hpp"  // NO! Libs does not depend on Scenes

// CORRECT: Agnostic and generic
class MyUtility {
    // Works without knowing Scenes/Physics/Graphics
    static Vector3 interpolate(const Vector3& a, const Vector3& b, float t);
};

// High-level systems use Libs
#include "Libs/MyUtility.hpp"  // Graphics/Scenes/Physics can include Libs
```

## Critical Attention Points

- **Engine foundation**: Everything depends on Libs, critical stability
- **Zero high-level dependencies**: Libs must NEVER include Scenes/Physics/Graphics/etc.
- **Agnostic**: Generic code, not specific to any use case
- **Uniformity**: Math types used EVERYWHERE (Vector, Matrix, CartesianFrame)
- **Thread-safe**: Consider thread-safety for shared utilities
- **Performance**: Critical code (used everywhere), optimize if necessary
- **Documentation**: Document well, many systems depend on Libs
- **Exhaustive tests**: Bug in Libs affects entire engine

## Math/Space3D: Capsule Primitive

### Definition
A **Capsule** (also called Stadium solid) is a swept sphere: a line segment with a radius. It consists of a cylindrical body capped by two hemispheres.

```
    ___
   /   \      ← hemisphere (radius r)
  |     |
  |     |     ← cylinder (height h, radius r)
  |     |
   \___/      ← hemisphere (radius r)
```

### Internal Representation
- `Segment<precision_t> m_axis` - Central axis (start and end points)
- `precision_t m_radius` - Radius of the capsule

### Key Design Decisions

**Degenerate capsules (zero-length axis) are VALID:**
- A capsule with `startPoint == endPoint` behaves as a sphere
- `isValid()` only checks `radius > 0`
- Use `isDegenerate()` to detect zero-length axis

**Code references:**
- `Math/Space3D/Capsule.hpp:isValid()` - Returns true if radius > 0
- `Math/Space3D/Capsule.hpp:isDegenerate()` - Returns true if axis length ≈ 0
- `Math/Space3D/Capsule.hpp:closestPointOnAxis()` - Critical for collision detection

### Collision Convention (MTV)
**MTV (Minimum Translation Vector) pushes the FIRST argument out of the SECOND.**

```cpp
// 4 overloads per collision pair:
isColliding(Capsule, X)           // without MTV
isColliding(Capsule, X, mtv)      // MTV pushes Capsule out of X
isColliding(X, Capsule)           // without MTV
isColliding(X, Capsule, mtv)      // MTV pushes X out of Capsule
```

### Usage Example
```cpp
// Create a capsule (vertical, height 10, radius 2)
Capsule<float> capsule{{0, 0, 0}, {0, 10, 0}, 2.0f};

// Check collision with sphere
Sphere<float> sphere{1.5f, {3, 5, 0}};
Vector<3, float> mtv;
if (isColliding(capsule, sphere, mtv)) {
    // mtv contains vector to push capsule out of sphere
}

// Raycast against capsule
Line<float> ray{{-10, 5, 0}, {1, 0, 0}};
Point<float> hit;
if (isIntersecting(ray, capsule, hit)) {
    // hit contains first intersection point
}
```

### Available Collision Tests
| Pair | File | Notes |
|------|------|-------|
| Capsule-Capsule | `SamePrimitive.hpp` | Segment-segment distance |
| Capsule-Point | `CapsulePoint.hpp` | Point-to-axis distance |
| Capsule-Sphere | `CapsuleSphere.hpp` | Closest point on axis |
| Capsule-Cuboid | `CapsuleCuboid.hpp` | Iterative refinement |
| Capsule-Triangle | `CapsuleTriangle.hpp` | Terrain mesh collision |
| Line-Capsule | `LineCapsule.hpp` | Raycasting |
| Segment-Capsule | `SegmentCapsule.hpp` | Finite ray intersection |

### Unit Tests
All capsule functionality is tested in `Testing/test_MathSpace3D.cpp`:
- Primitive tests (constructor, isValid, isDegenerate, centroid, volume)
- Collision tests (all pairs, with/without MTV)
- Intersection tests (Line, Segment)
- Edge cases (degenerate capsules, touching/not touching)

## PixelFactory: Thread Safety and Resize

### TextProcessor and Pixmap During Resize

During window resize, Pixmap dimensions can change between frames. `TextProcessor` must be resilient:

**Implemented protection:**
1. `TextProcessor::setPixmap()` calls `updateMetrics()` to recalculate `maxColumns`/`maxRows`
2. `blitCharacter()` uses `blendFreePixel()` (ignores out-of-bounds pixels) instead of `blendPixel()` (assert)
3. Notifier checks `pixmap.width() == 0 || pixmap.height() == 0` before rendering

**Code references:**
- `PixelFactory/TextProcessor.hpp:setPixmap()` - Calls `updateMetrics()` after pixmap change
- `PixelFactory/TextProcessor.hpp:blitCharacter()` - Uses `blendFreePixel()` for bounds-safety
- `PixelFactory/Pixmap.hpp:blendPixel()` - Assert on coordinates (development)
- `PixelFactory/Pixmap.hpp:blendFreePixel()` - Silently ignores out-of-bounds (production)

## VertexFactory: Grid Terrain Queries

### Edge Clamping Behavior

`Grid::getHeightAt()` and `Grid::getNormalAt()` clamp out-of-bounds coordinates to the terrain edge instead of returning default values. This ensures smooth physics behavior at terrain boundaries.

**Why this matters:**
- Physics queries at scene boundaries (outside terrain grid) now return the nearest edge height/normal
- Prevents entities from falling through "phantom ground" at Y=0 outside terrain
- Ensures consistent terrain behavior for collision detection

**Code references:**
- `VertexFactory/Grid.hpp:getHeightAt()` - Clamps coordinates with epsilon margin before interpolation
- `VertexFactory/Grid.hpp:getNormalAt()` - Same clamping behavior for normal queries

**Implementation detail:**
```cpp
constexpr auto epsilon = static_cast<vertex_data_t>(0.0001);
const auto clampedX = std::clamp(positionX, -m_halfSquaredSize + epsilon, m_halfSquaredSize - epsilon);
const auto clampedY = std::clamp(positionY, -m_halfSquaredSize + epsilon, m_halfSquaredSize - epsilon);
```

## Algorithms: DiamondSquare

### Overview

The `DiamondSquare` algorithm generates fractal terrain heightmaps. It produces natural-looking landscapes with controllable roughness.

### Key Design: Normalized Output

**Output values are normalized to [-1, 1] range.** This means the `factor` parameter when applying to a Grid represents the actual maximum height displacement in world units (meters).

```cpp
// DiamondSquare generates values in [-1, 1]
// When applied with factor=100, terrain heights will be ±100 meters
grid.applyDiamondSquare({100.0F, 0.5F, 0});  // factor=100m, roughness=0.5
```

### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `size` | `size_t` | Grid dimension (must be 2^n + 1, e.g., 3, 5, 9, 17, 33...) |
| `roughness` | `float` | 0.0-1.0, controls terrain detail (higher = more jagged) |
| `normalize` | `bool` | Default `true`. Set to `false` for raw algorithm output |
| `useSameValueForCorner` | `bool` | If `true`, all four corners start with same random value (tileable edges) |
| `seed` | `int32_t` | Optional random seed for reproducible results |

### Usage Example

```cpp
#include "Libs/Algorithms/DiamondSquare.hpp"

// Direct usage
Algorithms::DiamondSquare<float> ds(42, true);  // seed=42, same corners
if (ds.generate(129, 0.5F)) {  // 129x129 grid, 0.5 roughness
    float height = ds.value(64, 64);  // Query center point
    // height is in [-1, 1] range
}

// Via VertexFactory Grid (typical usage)
Grid grid(8192.0F, 256);  // 8km terrain, 256 subdivisions
grid.applyDiamondSquare({
    100.0F,  // factor: heights will be ±100 meters
    0.5F,    // roughness: 0.5 (moderate detail)
    7        // seed: reproducible terrain
});
```

### Why Normalization Matters

Without normalization, the algorithm produces values proportional to grid size:
- 33×33 grid → values roughly ±32
- 16385×16385 grid → values roughly ±16384

This made the `factor` parameter confusing (factor=1.0 on a 16k grid gave ±16km heights!).

**With normalization (default):**
- All grid sizes → values in [-1, 1]
- `factor` directly represents maximum height in world units
- `factor=100` means ±100 meters regardless of grid resolution

**Code reference:**
- `Algorithms/DiamondSquare.hpp:normalizeData()` - Min/max normalization to [-1, 1]
- `Algorithms/DiamondSquare.hpp:generate()` - `normalize` parameter (default `true`)

## Detailed Documentation

Libs is referenced by all systems:
- @src/Scenes/AGENTS.md - Uses CartesianFrame
- @src/Physics/AGENTS.md - Uses Vector, Matrix, collision detection
- @src/Graphics/AGENTS.md - Uses Math for transformations
- @src/Audio/AGENTS.md - Uses Math for 3D positioning, WaveFactory for sound processing
