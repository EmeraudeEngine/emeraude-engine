# Animations System

Context for developing the Emeraude Engine animations system.

## Module Overview

**Status: UNDER CONSTRUCTION** - Skeletal animation system in development. Data types are stabilized in `Libs/Animation/`; runtime evaluation lives here.

## Animations-Specific Rules

### System Scope
- **Skeletal animations**: Skinning, mesh deformation via skeleton
- **Animation interfaces**: Possibility to animate values via interfaces
- **NOT transform animations**: Use scene graph for position/rotation/scale animation

### Architecture

The animation system is split into two layers:

**Data layer (`Libs/Animation/`)** — Stabilized, header-only:
| Type | File | Purpose |
|------|------|---------|
| `Joint` | `Libs/Animation/Joint.hpp` | Joint struct: name, parentIndex, local T/R/S, inverseBindMatrix |
| `Skeleton` | `Libs/Animation/Skeleton.hpp` | Ordered joint collection, name lookup, hierarchy validation |
| `AnimationChannel` | `Libs/Animation/AnimationChannel.hpp` | Per-joint keyframes (VectorKeyFrame for T/S, QuaternionKeyFrame for R), 3 interpolation modes (Step, Linear, CubicSpline) |
| `AnimationClip` | `Libs/Animation/AnimationClip.hpp` | Named channel collection, duration auto-computed, skeleton-independent |
| `Skin` | `Libs/Animation/Skin.hpp` | Mesh-to-skeleton binding: joint index remapping, inverse bind matrices, GLTF JOINTS_0 indirection |

**Runtime layer (`src/Animations/`)** — This module. Responsible for:
- Keyframe sampling and interpolation at a given time
- Skeleton pose evaluation (joint hierarchy traversal)
- Skinning matrix computation (joint matrices for GPU upload)
- Animation blending (crossfade, layered, additive)
- Animation state machine / controller

### Data Flow
```
Loaders (GLTF, MD5) → Libs/Animation types → Shape carries Skeleton+Skin
                                            → AnimationClips stored per-model
        ↓
src/Animations/ (runtime) → evaluates clips → produces skin matrices → GPU SSBO
```

### Loader Integration

**GLTF** (`Scenes/GLTFLoader.cpp`):
- `loadSkins()` builds Skeleton from GLTF node hierarchy, reads inverse bind matrices, creates Skin, attaches to Shapes
- `loadAnimations()` reads animation channels/samplers, maps node indices to joint indices, creates AnimationClips
- Pipeline order: Images -> Materials -> Meshes -> Skins -> Animations -> Nodes

**MD5** (`VertexFactory/FileFormatMDx.hpp`):
- `loadMD5()` builds Skeleton (world->local transforms, inverse bind matrices), vertex influences/weights (top-4 by bias with renormalization), Skin (1:1 mapping), attaches to Shape

### Planned Features (remaining)
- Runtime keyframe sampling and interpolation
- Skeleton pose evaluation (joint hierarchy traversal)
- Skinning matrix computation (joint matrices for GPU upload)
- Animation blending (crossfade, layered, additive)
- Animation state machine / controller
- GPU skinning via compute shader or vertex shader

## Development Commands

```bash
# Animation data type tests (in Testing/)
ctest -R MathTransformConversions
./test --filter="*TransformConversion*"

# Animation tests (when available)
ctest -R Animations
./test --filter="*Animation*"
```

## Important Files

### Data Types (Libs/Animation/)
- `Libs/Animation/Joint.hpp` - Joint struct
- `Libs/Animation/Skeleton.hpp` - Joint collection with validation
- `Libs/Animation/AnimationChannel.hpp` - Keyframe channels
- `Libs/Animation/AnimationClip.hpp` - Named clip (collection of channels)
- `Libs/Animation/Skin.hpp` - Mesh-to-skeleton binding

### Math Support
- `Libs/Math/TransformUtils.hpp` - TRS decomposition/composition for joint transforms
- `Libs/Math/Quaternion.hpp` - `toRotationMatrix4()` for skeleton matrix computation
- `Libs/Math/CartesianFrame.hpp` - `fromQuaternion()`, `toQuaternion()` for joint frame conversion

### Geometry Integration
- `Libs/VertexFactory/Shape.hpp` - `skeleton()`, `skin()` optional members
- `Libs/VertexFactory/FileFormatMDx.hpp` - MD5 skeletal loader
- `Scenes/GLTFLoader.hpp/.cpp` - GLTF skin/animation loader

### Tests
- `Testing/test_MathTransformConversions.cpp` - 52 tests: Quat<->Mat4, Quat<->CartesianFrame, CartesianFrame<->Mat4, compose/decompose roundtrips, drift, consistency

## Development Patterns

### Reading Skeletal Data from a Loaded Shape
```cpp
const auto& shape = loadedShape;
if (shape.hasSkeleton()) {
    const auto& skeleton = shape.skeleton();
    // skeleton.joints() — ordered joint array
    // skeleton.findJointIndex("Hips") — name lookup
}
if (shape.hasSkin()) {
    const auto& skin = shape.skin();
    // skin.inverseBindMatrices() — per-joint IBMs
    // skin.jointIndices() — skeleton-to-mesh joint remapping
}
```

## Critical Points

- **Rotation matrix convention**: Use `Quaternion::toRotationMatrix4()` (standard column-major), NOT `rotationMatrix()` (row-major data in column-major storage). See `Libs/AGENTS.md` for details.
- **Under active development**: Runtime evaluation architecture subject to change
- **Data types stabilized**: `Libs/Animation/` types are stable and used by loaders
- Consult developers before major modifications to the runtime layer

## Detailed Documentation

- **Libs/Animation data types**: See [`src/Libs/AGENTS.md`](../Libs/AGENTS.md)
- **Math/TransformUtils**: See [`src/Libs/AGENTS.md`](../Libs/AGENTS.md) (Math section)
- **GLTF loading pipeline**: See [`src/Scenes/AGENTS.md`](../Scenes/AGENTS.md)
