# Overlay System

Context for developing the Emeraude Engine 2D overlay system.

## Module Overview

2D abstraction system for displaying elements on top of 3D rendering. Hierarchical architecture Manager → Screen → Surface with generic bitmap support, ImGui for debug, and possible CEF integration.

## Overlay-Specific Rules

### Hierarchical Architecture

**Manager**: Main overlay system manager
**Screen**: Logical Surface group (no graphical dimensions, just organization)
**Surface**: Graphical element with Pixmap (bitmap), position, dimensions, Z coordinate

### Surface Concept
- **Pixmap**: Bitmap/image representing Surface content
- **Position**: X, Y screen coordinates
- **Dimensions**: Width, height in pixels
- **Z-ordering**: Z coordinate for Surface layering
- Multiple Surfaces can coexist in a Screen

### Double Buffering System (Transition Buffer)

For asynchronous content providers (e.g., CEF browsers), Surface supports a transition buffer system to handle resize smoothly without visual glitches.

**Key concepts:**
- **Active Buffer**: Currently displayed framebuffer
- **Transition Buffer**: New-size buffer waiting for content during resize
- **Framebuffer struct**: Contains `image`, `imageView`, `sampler`, `pixmap`, `descriptorSet`, plus `width()` and `height()` accessors

**Lifecycle:**
1. `enableTransitionBuffer()` - Enable async content provider mode
2. On resize: transition buffer created at new size, active buffer continues displaying
3. Content provider fills transition buffer via `transitionPixmap()` or `writeTransitionBufferWithMapping()`
4. `commitTransitionBuffer()` - Swap buffers when new content ready
5. Callbacks: `onActiveBufferReady()`, `onTransitionBufferReady()` for notifications

**Code references:**
- `Surface.hpp:Framebuffer` struct definition
- `Surface.cpp:commitTransitionBuffer()` - Buffer swap logic
- `Surface.hpp:isTransitionBufferReady()` - Check if transition buffer awaits content

### Direct GPU Memory Mapping

For performance optimization, Surface supports direct GPU memory writes bypassing the staging buffer path.

**When to use:**
- High-frequency content updates (video, browser rendering)
- When content provider already has pixel data in memory
- Reduces CPU→GPU copy overhead

**Requirements:**
- Call `enableMapping()` **before** `createOnHardware()` (typically in constructor)
- Image created with `VK_IMAGE_TILING_LINEAR` and `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`
- Image layout transitioned to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` after creation

**API:**
```cpp
// In constructor (BEFORE createOnHardware is called):
this->enableMapping();

// Write to active buffer:
bool success = this->writeActiveBufferWithMapping([&](void* ptr, VkDeviceSize rowPitch) {
    // Copy pixel data respecting rowPitch
    return true;
});

// Write to transition buffer:
bool success = this->writeTransitionBufferWithMapping([&](void* ptr, VkDeviceSize rowPitch) {
    // Copy pixel data respecting rowPitch
    return true;
});
```

**C++20 type safety:** `writeWithMapping()` uses `requires` constraint:
```cpp
requires std::invocable<write_func_t, void*, VkDeviceSize> &&
         std::convertible_to<std::invoke_result_t<write_func_t, void*, VkDeviceSize>, bool>
```

**Code references:**
- `Surface.hpp:enableMapping()` - Enable memory mapping mode
- `Surface.hpp:isMemoryMappingEnabled()` - Check if enabled
- `Surface.hpp:Framebuffer::writeWithMapping()` - RAII mapping with lambda
- `Surface.cpp:createFramebufferResources()` - Image creation with LINEAR tiling
- `Vulkan/Image.cpp:mapMemory()` / `unmapMemory()` - VMA memory mapping

**CRITICAL:** `enableMapping()` must be called before `createOnHardware()`. If called after, the image is already created without host-visible memory and mapping will fail.

### Supported Content Types

**Generic Surface**: Modifiable Pixmap bitmap (main use case)
**ImGui**: Integration for rapid development/debug
**CEF offscreen**: Web pages via CEF rendered into generic Surface (external integration)
**Future**: Basic integrated UI system (buttons, widgets, etc.)

### Rendering Integration
- **2D Pipeline via Saphir**: OverlayManager uses OverlayGenerator
- **Render order**: Renderer does 3D then 2D overlay
- **No lighting**: Pure 2D screen-space rendering
- **Alpha blending**: Transparency and multi-layer support

### Input Integration
- **OverlayManager is InputManager client**: Receives mouse/keyboard events
- **Hierarchical dispatch**: Manager → Screen → Surface
- **Interaction handling**: Clicks, hover, keyboard focus
- **Z-ordering**: Higher Z Surfaces receive events first

### CEF Integration (external)
- CEF not integrated in framework (external dependency)
- Applications can use CEF in offscreen mode
- CEF rendering → generic Surface Pixmap
- OverlayManager displays Surface normally

**Two rendering paths for CEF OnPaint():**

1. **Staging Buffer (Classic)**: CEF buffer → local Pixmap → staging buffer → GPU
   - Uses `activePixmap()` / `transitionPixmap()` + `setVideoMemoryOutdated()`
   - Compatible with all hardware

2. **Direct Memory Mapping**: CEF buffer → GPU mapped memory (bypasses staging)
   - Uses `writeActiveBufferWithMapping()` / `writeTransitionBufferWithMapping()`
   - Better performance, requires `enableMapping()` in constructor
   - Handle `rowPitch` differences between CEF (width*4) and GPU tiling

**Dirty rects support:** Both paths support partial updates via CEF's `dirtyRects` parameter for optimal performance.

## Development Commands

```bash
# Overlay tests
ctest -R Overlay
./test --filter="*Overlay*"
```

## Important Files

- `Manager.cpp/.hpp` - Main manager, Screen coordination, InputManager client
- `Screen.cpp/.hpp` - Logical Surface group
- `Surface.cpp/.hpp` - Graphical element with Framebuffer, position, Z-order, transition buffer system
- `Surface.hpp:Framebuffer` - Struct with image, imageView, sampler, pixmap, descriptorSet, width()/height()
- `Surface.hpp:writeWithMapping()` - C++20 template with requires constraint for type-safe GPU writes
- `FramebufferProperties.cpp/.hpp` - Screen resolution and scaling properties
- `ImGui/` - ImGui integration for debug/dev

### Additional Documentation
- `@docs/saphir-shader-system.md` - OverlayGenerator for 2D pipeline

## Development Patterns

### Creating a Screen with Surfaces
```cpp
// Create a Screen
auto hudScreen = overlayManager.createScreen("hud");

// Create Surfaces in Screen
auto healthBar = hudScreen->createSurface("health_bar");
healthBar->setPosition(10, 10);
healthBar->setDimensions(200, 20);
healthBar->setZ(10);  // Z-ordering

auto minimap = hudScreen->createSurface("minimap");
minimap->setPosition(screenWidth - 210, 10);
minimap->setDimensions(200, 200);
minimap->setZ(5);  // Behind health bar if overlap
```

### Modifying Surface Content
```cpp
// Access Surface Pixmap
auto& pixmap = surface->pixmap();

// Draw in bitmap
pixmap.fill(Color::Black);
pixmap.drawRectangle(10, 10, 50, 30, Color::Red);
pixmap.drawText(20, 20, "HP: 100", font, Color::White);

// Mark as modified for GPU re-upload
surface->markDirty();
```

### Using ImGui for Debug
```cpp
// ImGui integrated for rapid development
overlayManager.beginImGuiFrame();

ImGui::Begin("Debug Info");
ImGui::Text("FPS: %.1f", fps);
ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
ImGui::End();

overlayManager.endImGuiFrame();
```

### CEF Integration (external)
```cpp
// WebView constructor - enable features BEFORE createOnHardware()
WebView::WebView(...) : Surface{...} {
    this->enableTransitionBuffer();  // For smooth resize

    if (settings.get<bool>("CEF/UseMemoryMapping")) {
        this->enableMapping();  // MUST be before createOnHardware()
    }
}

// CEF OnPaint callback - dual path implementation
void WebView::OnPaint(const void* buffer, int width, int height, const RectList& dirtyRects) {
    const auto widthU = static_cast<uint32_t>(width);
    const auto heightU = static_cast<uint32_t>(height);

    // PATH 1: Direct GPU Memory Mapping
    if (this->isMemoryMappingEnabled()) {
        const auto* srcBuffer = static_cast<const uint8_t*>(buffer);
        const size_t srcPitch = widthU * 4;  // CEF provides BGRA

        auto writeFunction = [&](void* mappedPtr, VkDeviceSize rowPitch) -> bool {
            auto* dstBuffer = static_cast<uint8_t*>(mappedPtr);
            for (uint32_t y = 0; y < heightU; ++y) {
                std::memcpy(dstBuffer + y * rowPitch, srcBuffer + y * srcPitch, srcPitch);
            }
            return true;
        };

        // Choose buffer based on size match
        if (this->isTransitionBufferReady() && transitionBuffer().width() == widthU) {
            this->writeTransitionBufferWithMapping(writeFunction);
            this->commitTransitionBuffer();
        } else {
            this->writeActiveBufferWithMapping(writeFunction);
        }
        return;
    }

    // PATH 2: Staging Buffer (Classic)
    auto& pixmap = (isTransitionBufferReady() && matchesTransitionSize)
                   ? this->transitionPixmap()
                   : this->activePixmap();

    // Copy to pixmap using PixelFactory::Processor
    Processor<uint8_t> processor{pixmap};
    processor.blit(rawData, clip);

    if (isTransitionBuffer) {
        this->commitTransitionBuffer();
    } else {
        this->setVideoMemoryOutdated();
    }
}
```

### Handling Input Events
```cpp
// OverlayManager dispatches automatically
// Implement in Surface if needed
class CustomSurface : public Surface {
    void onMouseClick(int x, int y, MouseButton button) override {
        // Handle click on this Surface
    }

    void onMouseHover(int x, int y) override {
        // Handle hover
    }
};
```

## Critical Points

- **Z-ordering**: Z coordinate determines render order and input priority
- **Pixmap dirty flag**: Mark Surface dirty after modification for GPU re-upload
- **Screen organization**: Logically group Surfaces by functionality
- **Performance**: Avoid too frequent Pixmap modifications (GPU upload cost)
- **Alpha blending**: Use transparency for layered Surfaces
- **ImGui temporary**: For debug/dev, not for final production UI
- **CEF external**: No framework dependency, integration by application

### Memory Mapping Critical Rules
- **TIMING:** `enableMapping()` MUST be called before `createOnHardware()` (in constructor)
- **Row pitch:** GPU memory may have different row pitch than source data; always use the `rowPitch` parameter
- **Image tiling:** Mapped images use `VK_IMAGE_TILING_LINEAR` (vs OPTIMAL for staging path)
- **Layout transition:** Engine handles `UNDEFINED → SHADER_READ_ONLY_OPTIMAL` transition automatically

### Transition Buffer Critical Rules
- **Size matching:** Always compare frame size with `activeBuffer().width()/height()`, not pixmap dimensions
- **Commit timing:** Call `commitTransitionBuffer()` only after content is fully written
- **Callback order:** `onTransitionBufferReady()` fires when new buffer is ready for content

## Detailed Documentation

Related systems:
- @docs/saphir-shader-system.md - OverlayGenerator (2D pipeline)
- @src/Input/AGENTS.md - Input system (polling + events)
- @src/Graphics/AGENTS.md - Renderer and pipelines
