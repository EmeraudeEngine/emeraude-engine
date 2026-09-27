## 11. Animated Texture Cubemap System

### Overview

The Animated Texture Cubemap system provides animated cubemap textures stored as **Vulkan cube arrays**. It consists of two resource layers:

| Resource | File | Purpose |
|----------|------|---------|
| `CubemapMovieResource` | `Graphics/CubemapMovieResource.hpp/cpp` | CPU-side frame data (6 face pixmaps per frame + duration) |
| `AnimatedTextureCubemap` | `Graphics/TextureResource/AnimatedTextureCubemap.hpp/cpp` | GPU-side Vulkan TextureCubeArray wrapping a CubemapMovieResource |

**Primary use case:** Color projection textures for **point lights** (animated light patterns, fire flicker, etc.).

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ CubemapMovieResource (CPU)                                   │
│  std::vector< pair< CubemapPixmaps, uint32_t > >            │
│  Frame 0: [6 face pixmaps] + duration (ms)                   │
│  Frame 1: [6 face pixmaps] + duration (ms)                   │
│  ...                                                         │
│  Frame N: [6 face pixmaps] + duration (ms)                   │
└──────────────────────┬──────────────────────────────────────┘
                       │ load()
                       ▼
┌─────────────────────────────────────────────────────────────┐
│ AnimatedTextureCubemap (GPU)                                  │
│  VkImage (CUBE_COMPATIBLE, arrayLayers = 6 × frameCount)     │
│  VkImageView (CUBE_ARRAY)                                    │
│  VkSampler ("AnimatedCubemap", no mipmap, no anisotropy)     │
└─────────────────────────────────────────────────────────────┘
```

**Memory layout:** All frames are packed into a single cube array image. Each frame occupies 6 consecutive array layers. `totalLayers = CubemapFaceCount × frameCount`.

**Frame indexing:** The W coordinate selects the frame index at runtime. `request3DTextureCoordinates()` returns `true`.

### CubemapMovieResource JSON Formats

**Data store directory:** `./data-stores/CubemapMovies/`

#### Parametric Loading (numbered sequence)

Generates cubemap names from a pattern with zero-padded indices:

```json
{
    "BaseCubemapName": "FireProjection/frame_{3}",
    "FrameCount": 30,
    "FrameRate": 24,
    "IsLooping": true
}
```

- `BaseCubemapName`: Pattern with `{N}` where N = zero-padding width. E.g. `{3}` → `001`, `002`, ...`030`
- `FrameCount`: Total frames (required)
- Frame timing (pick one, priority order):
  - `FrameRate`: FPS → `duration = 1000 / fps`
  - `AnimationDuration`: Total ms → `duration = total / frameCount`
  - `FrameDuration`: Per-frame ms (default: `1000/30 ≈ 33ms`)
- `IsLooping`: Loop animation (default: `true`)

Each generated name (e.g. `FireProjection/frame_001`) is resolved as a `CubemapResource` from the cubemap container.

#### Manual Loading (explicit frame list)

```json
{
    "Frames": [
        { "Cubemap": "Effects/fire_burst_01", "Duration": 50 },
        { "Cubemap": "Effects/fire_burst_02", "Duration": 50 },
        { "Cubemap": "Effects/fire_burst_03", "Duration": 100 }
    ]
}
```

- `Frames`: Array of frame objects
  - `Cubemap`: CubemapResource name (required)
  - `Duration`: Frame duration in ms (default: `33ms`)

### Sampler Configuration

The `AnimatedTextureCubemap` sampler differs from regular texture samplers:

| Property | AnimatedTextureCubemap | Regular Textures |
|----------|------------------------|------------------|
| Mag/Min Filter | Settings-driven (linear/nearest) | Settings-driven |
| Mipmap Mode | `NEAREST` | `LINEAR` |
| Anisotropy | `VK_FALSE` | Settings-driven |
| Max LOD | `0.0` | Computed |

**Rationale:** No mipmaps are generated for animated textures (single mip level), so mipmap filtering and anisotropy are disabled.

### Resource Container Registration

Both resources are registered in `Resources::Manager`:

```cpp
// Container aliases
using CubemapMovies = Resources::Container< Graphics::CubemapMovieResource >;
using AnimatedTextureCubemaps = Resources::Container< Graphics::TextureResource::AnimatedTextureCubemap >;
```

Both containers share the `"CubemapMovies"` local store directory.

### Usage Pattern

```cpp
// Get the default animated cubemap resource (for color projection)
auto animCubemap = resources.container< TextureResource::AnimatedTextureCubemap >()->getDefaultResource();

// Set as color projection texture on a point light
lightComponent.setColorProjectionTexture(animCubemap);
```

**Default resource:** In debug mode, generates 3 frames (Red, Green, Blue) at 32×32. In release, generates 5 noise frames.

### Animation Timing

- `frameCount()`: Number of frames in the animation
- `duration()`: Total animation duration in ms (sum of all frame durations)
- `frameIndexAt(sceneTime)`: Returns frame index for a given scene time point
  - Loops via `timePoint % duration` when looping is enabled
  - Clamps to last frame when not looping and past duration

### Procedural Caustics Generation

`CubemapMovieResource::loadCaustics()` generates animated Voronoi water caustics programmatically:

```cpp
bool loadCaustics(
    uint32_t faceSize,        // Cubemap face resolution (e.g. 128)
    uint32_t frameCount,      // Number of animation frames (e.g. 60)
    uint32_t frameDuration,   // Duration per frame in ms (e.g. 33)
    float scale = 4.0F,       // Voronoi cell density (higher = finer)
    uint32_t seed = 0,        // Random seed for pattern
    float baseIntensity = 0.7F,    // Background brightness [0,1]
    float causticIntensity = 1.0F  // Caustic line brightness [0,1]
) noexcept;
```

**Algorithm:** Inverted Voronoi F2-F1 distance (bright at cell edges = caustic lines, dark at cell centers). Temporal animation uses a circular sin/cos path through noise space for seamless looping. See: `Base/Algorithms/VoronoiNoise.hpp` for the underlying noise.

**Usage pattern:** See `projet-alpha/src/Builtin/PoolRooms.cpp:onSetupLighting()`.

### Code References

- `Graphics/CubemapMovieResource.hpp/cpp` — CPU frame storage, JSON loading, procedural caustics
- `Graphics/TextureResource/AnimatedTextureCubemap.hpp/cpp` — Vulkan cube array texture resource
- `Base/Algorithms/VoronoiNoise.hpp` — Voronoi noise: `evaluate()`, `caustic()` (F2-F1 clamped)
- `Resources/Manager.cpp` — Container registration (lines 531-533)
- `projet-alpha/src/Actor/Fire.cpp:77` — Usage: fire point light color projection
- `projet-alpha/src/Builtin/LightAndShadowDebug.cpp:113` — Usage: debug scene color projection
- `projet-alpha/src/Builtin/PoolRooms.cpp` — Usage: procedural caustics in closed room
