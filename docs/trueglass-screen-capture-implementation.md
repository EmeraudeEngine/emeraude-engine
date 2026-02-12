# TrueGlass Material - Screen-Space Refraction Implementation

## Overview

This document describes the implementation status of the `TrueGlassResource` material, which provides screen-space refraction for glass and water effects in the Emeraude Engine.

## Objective

Create a material that renders transparent objects (glass, water) with physically-based refraction by sampling what's behind them from a captured screen texture.

### Key Features
- Screen-space refraction with IOR-based distortion
- Schlick Fresnel approximation for reflection/refraction blending
- Depth-based absorption for colored glass/water
- Roughness-based blur for frosted glass effect

## Architecture

### Two-Pass Rendering Concept
```
Pass 1: Render opaque objects → Screen capture texture (color + depth)
Pass 2: Render glass objects → Final target, sampling from Pass 1 texture
```

### Files Created/Modified

#### New Files
- `src/Graphics/Material/TrueGlassResource.hpp` - Material class declaration
- `src/Graphics/Material/TrueGlassResource.cpp` - Material implementation with shader generation
- `src/Graphics/ScreenCaptureManager.hpp` - Screen capture resources manager
- `src/Graphics/ScreenCaptureManager.cpp` - Framebuffer, render pass, and texture management

#### Modified Files
- `src/Graphics/Renderer.hpp` - Added ScreenCaptureManager member and renderScreenCapture method
- `src/Graphics/Renderer.cpp` - Initialization, registration with bindless manager
- `src/Graphics/BindlessTextureManager.hpp` - Added `updateTexture2DRaw()` method and reserved slots (4, 5)
- `src/Graphics/BindlessTextureManager.cpp` - Implementation of raw texture update
- `src/Scenes/Scene.hpp` - Added `renderOpaqueOnly()` and `hasScreenSpaceRefractionObjects()`
- `src/Scenes/Scene.rendering.cpp` - Added `renderOpaqueOnly()` implementation, ScreenSpaceRefraction render list handling
- `src/Saphir/Keys.hpp` - Added shader variable keys (if needed)
- `src/Graphics/Material/Interface.hpp` - Added `requiresScreenCapture()` virtual method

## Current Status

### Working
- TrueGlassResource material class with all properties (IOR, refraction/reflection strength, absorption, roughness, thickness)
- Shader generation (vertex and fragment) with screen-space refraction algorithm
- ScreenCaptureManager initialization with framebuffer, render pass, images, and sampler
- Registration of screen capture textures with bindless texture manager (slots 4 and 5)
- Material renders without errors (but without actual refraction effect)

### GrabPass Infrastructure (New)
The rendering pipeline now supports a 3-category sort for transparent objects:

1. **`requiresGrabPass()` propagation**: `Material::Interface` declares a virtual `requiresGrabPass()` method (default `false`), overridden by `PBRResource`. This is propagated through `Renderable::Abstract::requiresGrabPass(uint32_t layerIndex)` to all concrete renderables (`MeshResource`, `SimpleMeshResource`, `SpriteResource`, `BasicSeaResource`, `BasicGroundResource`, `TerrainResource`). Background/sky renderables always return `false`.

2. **`isOpaque()` integration**: `Material::Interface::isOpaque()` now returns `false` when `requiresGrabPass()` is `true`, because a material requiring a grab pass is inherently non-opaque. This ensures automatic correct sorting without additional scene-side logic.

3. **Three-way render list dispatch** in `Scene::insertIntoRenderLists()`:
   - **Opaque / OpaqueLighted**: Front-to-back (early-Z optimization)
   - **Translucent / TranslucentLighted**: Back-to-front, materials that do NOT require grab pass
   - **TranslucentGB / TranslucentGBLighted**: Back-to-front, materials that DO require grab pass

   Rendering order: Opaque → Translucent → TranslucentGB. The grab pass capture will happen between Translucent and TranslucentGB passes.

**Code references:**
- `Graphics/Material/Interface.hpp:isOpaque()` — returns false if `requiresGrabPass()` is true
- `Graphics/Material/Interface.hpp:requiresGrabPass()` — virtual, default false
- `Graphics/Material/PBRResource.hpp:requiresGrabPass()` — override
- `Graphics/Renderable/Abstract.hpp:requiresGrabPass()` — pure virtual
- `Graphics/Renderable/MeshResource.cpp:requiresGrabPass()` — layer dispatch
- `Scenes/Scene.hpp` — `TranslucentGB{5UL}`, `TranslucentGBLighted{6UL}` constants, 7-element render list array
- `Scenes/Scene.rendering.cpp:insertIntoRenderLists()` — 3-way dispatch logic

### Not Working - Screen Capture Pass

The screen capture render pass is **disabled** due to Vulkan validation errors.

#### Problem Description

When attempting to render opaque objects to the ScreenCaptureManager's framebuffer, validation errors occur:

```
VK_FORMAT_R8G8B8A8_UNORM (screen capture) != VK_FORMAT_B8G8R8A8_UNORM (swapchain)
VK_SAMPLE_COUNT_1_BIT (screen capture) != VK_SAMPLE_COUNT_4_BIT (swapchain MSAA)
```

#### Root Cause

The graphics pipelines are created for a specific render pass (the swapchain's). When we try to use these pipelines inside a different render pass (screen capture), Vulkan validation fails because:

1. **Format mismatch**: Screen capture uses `R8G8B8A8_UNORM`, swapchain uses `B8G8R8A8_UNORM`
2. **MSAA mismatch**: Screen capture has no MSAA (1 sample), swapchain has 4x MSAA
3. **Render pass incompatibility**: Different attachment configurations and subpass dependencies

The current implementation calls `scene.renderOpaqueOnly(mainRenderTarget(), commandBuffer)` which passes the swapchain as the render target for pipeline selection, but the actual render pass is the screen capture's - causing the mismatch.

## Remaining Work

### Option A: Proper RenderTarget Implementation (Recommended)

1. **Create a RenderTarget wrapper for ScreenCaptureManager**
   - Implement `RenderTarget::ScreenCapture` class inheriting from `RenderTarget::Abstract`
   - Provide proper `framebuffer()`, `renderPass()`, `viewMatrices()` etc.
   - This allows the rendering system to create compatible pipelines

2. **Match format to swapchain**
   - Use `VK_FORMAT_B8G8R8A8_UNORM` for color attachment
   - Or query the swapchain format dynamically

3. **Handle MSAA**
   - Either use the same MSAA sample count as swapchain
   - Or implement MSAA resolve before sampling (more complex)

4. **Submit as separate command buffer**
   - Similar to `renderRenderToTextures()` pattern
   - Use semaphore synchronization

### Option B: Post-Opaque Copy Approach

Instead of a separate render pass, capture the framebuffer content mid-frame:

1. After rendering opaques in main pass, insert a pipeline barrier
2. Copy/blit the color and depth attachments to the screen capture textures
3. Continue rendering ScreenSpaceRefraction objects
4. **Challenge**: MSAA resolve required, may need to exit/re-enter render pass

### Option C: Input Attachments (Subpass Approach)

Use Vulkan subpasses with input attachments:

1. Subpass 0: Render opaques
2. Subpass 1: Render glass objects, reading from subpass 0 as input attachment
3. **Challenge**: Requires significant render pass restructuring

## Code Locations

### Disabled Code
In `Renderer.cpp`, the screen capture pass is commented out around line 915:
```cpp
/* TODO: Screen capture pass for screen-space refraction (TrueGlass).
 * Currently disabled because the screen capture render pass has different
 * format/MSAA settings than the swapchain, causing pipeline incompatibilities.
 */
/*if ( scene != nullptr && m_screenCaptureManager.isInitialized() )
{
    this->renderScreenCapture(currentFrameScope, *scene, *commandBuffer);
}*/
```

### Key Methods to Review
- `Renderer::renderScreenCapture()` - The disabled screen capture implementation
- `Scene::renderOpaqueOnly()` - Renders only opaque objects
- `ScreenCaptureManager::registerWithBindlessManager()` - Texture registration

## References

- Bindless texture slots: Color=4, Depth=5 (defined in `BindlessTextureManager.hpp`)
- Shader variables: See `TrueGlassResource::generateFragmentShaderCode()`

## Related Documentation

- [`docs/graphics-system.md`](graphics-system.md) - Graphics system overview
- [`docs/render-targets.md`](render-targets.md) - Render target architecture
- [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) - Graphics subsystem context
