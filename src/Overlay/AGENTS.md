# Overlay System

Context for developing the Emeraude Engine 2D overlay system.

## Module Overview

2D abstraction system for displaying elements on top of 3D rendering. Hierarchical architecture Manager → Screen → Surface with generic bitmap support, ImGui for debug, and possible CEF integration.

## Overlay-Specific Rules

### Hierarchical Architecture

**Manager**: Main overlay system manager
**Screen**: Logical Surface group (no graphical dimensions, just organization). **Owns the stack ordering** of its surfaces.
**Surface**: Graphical element with Pixmap (bitmap), position, dimensions. Carries no public depth/Z concept.

### Surface Concept
- **Pixmap**: Bitmap/image representing Surface content
- **Position**: X, Y screen coordinates
- **Dimensions**: Width, height in pixels
- **Stack order**: Defined and owned by the parent UIScreen — see *Stack Ordering* below
- Multiple Surfaces can coexist in a Screen

### Stack Ordering (UIScreen)

**The screen is a pile of sheets.** Surfaces are arranged in a stack vector where:
- **Index `0` = bottom** (drawn first, behind every other surface)
- **Index `N-1` = top** (drawn last, visible above every other)

This convention matches DOM paint order, UIKit `addSubview`, and Photoshop layer panels: items added later or moved up appear visually on top.

**Input dispatch is the inverse**: events traverse the stack from top to bottom (`std::views::reverse`), so the topmost surface receives a pointer/key event first — visually consistent with what the user sees.

**Surfaces never carry a public depth.** The internal `m_depth` member exists only to feed the Z translation of the model matrix (avoids any z-fighting if the 2D pipeline uses a depth test); it is recomputed automatically by `UIScreen::recomputeDepths()` after every stack mutation. Applications never see or set this value.

**Stack mutation API on `UIScreen`** (all methods take a surface name; the parent screen owns the order):

| Method | Effect |
|---|---|
| `bringToFront(name)` | Moves the surface to index `N-1` (top). |
| `sendToBack(name)` | Moves the surface to index `0` (bottom). |
| `bringForward(name)` | Swaps the surface with its upper neighbor (one step up). |
| `sendBackward(name)` | Swaps the surface with its lower neighbor (one step down). |
| `moveAbove(name, reference)` | Inserts the surface just above `reference`. |
| `moveBelow(name, reference)` | Inserts the surface just below `reference`. |
| `indexOf(name)` | Returns `std::optional< size_t >` — current stack index or `nullopt`. |
| `stackOrder()` | Returns the names in stack order, bottom to top. |

**Creation always pushes onto the top** of the stack. To start a surface elsewhere, create it then call `sendToBack()` / `moveBelow()` immediately after.

**Code references:**
- `UIScreen.hpp` — Stack ordering API declarations
- `UIScreen.cpp:recomputeDepths()` — Internal depth assignment (bottom = `0.0F`, step `0.001F` per index)
- `Surface.hpp:setStackIndex()` — Private, friend-accessed by `UIScreen` only
- `Surface.cpp:updateModelMatrix()` — Consumes the internal `m_depth` for the Z translation

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

**Status state machine (`TransitionBufferStatus`):** `Ready` → `Resizing` (GPU resources being (re)created, drawing/commit forbidden, `isTransitionBufferReady()` returns false) → `WaitingForContent` (sized, awaiting provider frame) → back to `Ready` on commit.

> [!CRITICAL]
> **The content provider's painted size is the source of truth — not the engine's surface-size formula.**
> A frame is committed only when its pixel size matches the transition buffer *exactly*
> (`determineTargetBuffer()` / `matchesSize()` use strict equality, no tolerance). The transition
> buffer is initially sized via `FramebufferProperties::getSurfaceWidth/Height()`
> (`round(round(resolution·geom)·screenScale)`, double-round, per-axis scale), while an OSR
> provider like CEF paints at `providerRounding(viewRect · device_scale_factor)` using its own
> internal rounding and a single `maxScreenScale()`. **On fractional display scales (e.g. 125%)
> these can diverge by ±1 px**, so the painted frame matches neither buffer and the resize commit
> stalls — the active buffer keeps the old size → **black/stale render until the next resize**.
> This was reproducible by maximizing the window (single deterministic size jump) on 125%-scaled
> Windows monitors; manual drag hid it because it sweeps many sizes.
>
> **Resolution — provider-driven transition resize (convergence loop):** when the provider paints a
> size matching neither buffer, it calls `requestTransitionBufferResize(width, height)` (thread-safe
> setter, no GPU work, guarded by a dedicated mutex). On the next `processUpdates()` (render thread),
> `recreateTransitionBufferToRequestedSize()` recreates the transition buffer at that exact painted
> size (dedicated path — it does **not** recompute via the surface-size formula, and deliberately
> does **not** fire `onTransitionBufferReady()`, which would re-trigger a CEF `WasResized()` and risk
> looping). The next identical frame then matches and commits. Converges within one render iteration.

**Code references:**
- `Surface.hpp:Framebuffer` struct definition
- `Surface.cpp:commitTransitionBuffer()` - Buffer swap logic
- `Surface.hpp:isTransitionBufferReady()` - Check if transition buffer awaits content
- `Surface.cpp:requestTransitionBufferResize()` - Provider-thread setter recording the authoritative painted size
- `Surface.cpp:recreateTransitionBufferToRequestedSize()` - Render-thread dedicated recreation path (called from `processUpdates()` Step 1.b)
- Consumer side: `app_system/src/UI/WebView.CefRenderHandler.cpp:directPaint()/indirectPaint()` request the resize on a size mismatch instead of dropping the frame

### Sampler ownership (shared cache)

`Surface`'s sampler comes from the renderer's shared sampler cache
(`Renderer::getSampler("OverlaySurface", …)`) and is **shared by every overlay surface**.
`Surface::destroyFromHardware()` must only **release** its reference (`m_sampler.reset()`), never
`m_sampler->destroyFromHardware()` — destroying it would invalidate it for all other surfaces
(`VUID-vkDestroySampler-sampler-01082`). The cache owns it and destroys it once at renderer
shutdown. (Fixed Jun 2026; see `docs/multi-scene-resource-ownership.md` — "anything from a shared
cache is borrowed, not owned".)

### Direct GPU Memory Mapping

For performance optimization, Surface supports direct GPU memory writes bypassing the staging buffer path.

**When to use:**
- High-frequency content updates (video, browser rendering)
- When content provider already has pixel data in memory
- Reduces CPU→GPU copy overhead

**Modes (`Surface::MemoryMappingMode`, tri-state):**
- `Staging` — always staging upload (`DEVICE_LOCAL` `OPTIMAL` image). Default for plain surfaces.
- `Direct` — force direct mapping (falls back to staging with a warning if the format lacks `LINEAR`+`SAMPLED`).
- `Auto` — decide from the device (see below). This is what the apps set for CEF web-views (setting `App/CEF/TextureUploadStrategy` (`direct` / `staging` / `auto`), default `"auto"`).

**Auto resolution (in `createOnHardware()`):** mapping is enabled only when **both**:
1. the overlay format supports `VK_IMAGE_TILING_LINEAR` + `SAMPLED` (`PhysicalDevice::getFormatProperties`), and
2. `PhysicalDevice::hasMappableDeviceLocalMemory()` is true — a large `DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT` memory type exists (integrated GPUs, software rasterizers, or discrete GPUs with **full Resizable BAR**; the legacy 256 MiB BAR is excluded by a heap-size test). When true, VMA places the CPU-mapped image in device-local memory (sampled without crossing PCIe), so `Auto` chooses direct mapping; otherwise it stages into a `DEVICE_LOCAL` `OPTIMAL` image. (`Image::createWithVMA` logs where VMA actually placed each host-visible image.)

The resolved decision is logged once per surface at creation:
`Surface 'X' memory mapping ENABLED/DISABLED [mode=auto, UMA=yes/no, linearSampled=yes/no]`.

**Requirements:**
- Set the mode **before** `createOnHardware()` (typically in constructor): `setMemoryMappingMode(mode)` or the legacy `enableMapping()` (= `On`).
- When enabled: image created with `VK_IMAGE_TILING_LINEAR` + `HOST_VISIBLE | HOST_COHERENT`, usage `SAMPLED` only (no transfer/staging).

**API:**
```cpp
// In constructor (BEFORE createOnHardware is called):
this->setMemoryMappingMode(Surface::MemoryMappingMode::Auto);
// or from a setting string ("on" / "off" / "auto", tolerant):
this->setMemoryMappingMode(Surface::parseMemoryMappingMode(settingValue));

// Write to active buffer:
bool success = this->writeActiveBufferWithMapping([&](void* ptr, VkDeviceSize rowPitch) {
    // Copy pixel data respecting rowPitch
    return true;
});
```

**Code references:**
- `Surface.hpp:MemoryMappingMode` / `setMemoryMappingMode()` / `parseMemoryMappingMode()` - tri-state mode
- `Surface.hpp:enableMapping()` - legacy alias for `MemoryMappingMode::Direct`
- `Surface.hpp:isMemoryMappingEnabled()` - the **resolved** bool (valid only after `createOnHardware()`)
- `Surface.cpp:createOnHardware()` - resolves `Auto` against the device (UMA + LINEAR/SAMPLED), logs the decision
- `Vulkan/PhysicalDevice.cpp:hasMappableDeviceLocalMemory()` - device-local + host-visible memory detection (UMA / full ReBAR)
- `Surface.hpp:Framebuffer::writeWithMapping()` - RAII mapping with lambda
- `Surface.cpp:createFramebufferResources()` - image creation with LINEAR tiling

**CRITICAL:** set the mode before `createOnHardware()`. `isMemoryMappingEnabled()` only reflects the final decision **after** `createOnHardware()` — `Auto` is resolved there (against the device), not when the mode is set.

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

### UIScreen Rendering Options

UIScreen supports two rendering options that affect shader program selection:

**Premultiplied Alpha** (`setPremultipliedAlpha(bool)` / `premultipliedAlpha()`):
- When true: Uses premultiplied alpha blending formula
- Required for CEF/Chromium which provides premultiplied BGRA pixels
- Default: false (standard alpha blending)

**BGRA Source Format** (`useBGRAFormat(bool)` / `isUsingBGRAFormat()`):
- When true: Shader applies `.bgra` swizzle to convert BGRA → RGBA
- Required for CEF which provides BGRA pixel order
- When false: No swizzle, assumes RGBA source
- Default: false (RGBA)

**Shader Program Variants:**
Manager maintains 4 shader programs for all combinations:
```
Index | Alpha Mode      | Pixel Format | Use Case
------+-----------------+--------------+------------------
  0   | Standard        | RGBA         | Default sources
  1   | Premultiplied   | RGBA         | Premul RGBA sources
  2   | Standard        | BGRA         | Raw BGRA sources
  3   | Premultiplied   | BGRA         | CEF (typical)
```

Program selection uses bitwise index: `(premultipliedAlpha ? 1 : 0) | (isUsingBGRAFormat ? 2 : 0)`

**Code references:**
- `UIScreen.hpp:setPremultipliedAlpha()`, `useBGRAFormat()` - Screen options
- `Manager.hpp:ProgramCount` - 4 program variants
- `Manager.cpp:739` - Program selection logic
- `OverlayRendering.cpp:159-168` - Conditional BGRA swizzle in fragment shader

### Input Integration
- **OverlayManager is InputManager client**: Receives mouse/keyboard events
- **Hierarchical dispatch**: Manager → Screen → Surface
- **Interaction handling**: Clicks, hover, keyboard focus
r- **Top-down resolution**: events traverse the stack from top to bottom (`std::views::reverse`); the first surface that consumes a press stops the propagation.

#### Pointer routing: per-event resolution, exclusive surface, and pointer capture

`UIScreen` resolves the target of every pointer event through three layers, checked in this order (see `UIScreen.cpp` `onPointerMove` / `onButtonPress` / `onButtonRelease` / `onMouseWheel`):

1. **Implicit pointer capture (grab)** — *the* mechanism that keeps a press→drag→release sequence coherent.
   - When a button press is **consumed** by a surface (its `onButtonPress` returns `true`), that surface becomes the **captor**: `m_pointerCaptureSurface` (a `mutable std::weak_ptr< Surface >`) + `m_pointerCaptureButtons` (a `mutable uint8_t` bitmask of the buttons it currently holds).
   - While a capture is active, **every** subsequent move / release / wheel / extra-button press is routed **directly to the captor**, bypassing position (`isBelowPoint`) and alpha testing. This is exactly Win32 `SetCapture` / DOM `setPointerCapture` semantics.
   - The capture is released automatically once the captor's **last** held button is up (`m_pointerCaptureButtons == 0` → `m_pointerCaptureSurface.reset()`). A destroyed captor expires its `weak_ptr` and is dropped on the next event.
   - **Why it exists:** without it, each event independently re-resolves its target by pixel position. With stacked, partially-transparent CEF surfaces, a drag that starts on surface A and ends over (or outside) surface B delivers the release to the wrong surface — A's `mouseUp` never reaches CEF and its JS state (camera rotation, slider drag…) stays stuck, while B receives a phantom `mouseUp` it never saw pressed. Capture binds the whole gesture to A.
2. **Explicit exclusive surface** — app-driven, set via `setInputExclusiveSurface(name)` / `disableInputExclusiveSurface()` / `isInputExclusiveSurfaceEnabled()` / `inputExclusiveSurface()` (`m_inputExclusiveSurface`, a `std::weak_ptr< Surface >`). When set (and no capture is active), all events go to that single surface regardless of the stack.
3. **Normal top-down stack resolution** — the default `std::views::reverse(m_surfaces)` walk.

**Precedence is deliberate:** an active capture **wins over** the explicit exclusive surface, so an in-flight drag is never yanked away by a concurrent `setInputExclusiveSurface()`. Capture is transient (bounded by the button hold); exclusive is a persistent app policy.

##### Pointer-move tap (fan-out) — orthogonal to the three resolution layers

The three layers above each pick **one** target. The **pointer-move tap** is a parallel "tee" on the
move stream only: a single designated surface (`m_pointerMoveTapSurface`, a `std::weak_ptr< Surface >`)
that **also** receives every `onPointerMove`, *in addition to* whichever surface the resolution picked —
regardless of the cursor position, alpha test, or which surface holds the capture.

- API (app-driven, like the exclusive surface): `setPointerMoveTapSurface(name)` / `disablePointerMoveTapSurface()` / `isPointerMoveTapSurfaceEnabled()` / `pointerMoveTapSurface()`.
- It is **non-consuming**: it does not change the routing result, does not block, and only fires on `onPointerMove` (not press/release/wheel). The tap move is delivered **directly** via `Surface::onPointerMove` (no enter/leave bookkeeping), mirroring the capture path.
- **No double delivery:** if the tap surface is the very surface the resolution already delivered the move to (capture / exclusive / the consuming surface in the stack walk), the tap is skipped for that event.
- **Scoping is the application's responsibility** — set it when a gesture begins, clear it when it ends. It is *not* managed internally by the press/release dispatch (unlike capture).
- **Why it exists:** a control on an upper surface (e.g. a slider on a UI overlay) consumes the press and thus **captures** the pointer, so a lower surface (e.g. a 3D view) would normally see no moves during the drag. The tap lets that lower surface keep receiving the live move stream — e.g. to update a 3D scene in real time while the slider is dragged — without disturbing the capture/consume semantics. See app_system `Manager::setPointerMoveTapWebView` and `AppControl.overlayManager.setWebViewPointerMoveTap`.

> [!NOTE]
> Capture attaches to the surface that **consumes** the press (returns `true`). A surface that forwards events to its content without blocking propagation (`processUnblockedPointerEvents`-style) does not become the captor — the natural owner of a drag is the blocker beneath it. The CEF consumer side still needs its own "I already sent the mousedown, so I must send the matching mouseup" tracking for the case where the release lands on a now-transparent pixel of the captor itself (see app_system `WebView::m_CEFButtonsDown`). The two layers are complementary: `UIScreen` decides **which** surface gets the event; the surface decides **whether** to forward it to its backend.

### CEF Integration (external)
- CEF not integrated in framework (external dependency)
- Applications can use CEF in offscreen mode
- CEF rendering → generic Surface Pixmap
- OverlayManager displays Surface normally

**CEF Screen Configuration:**
```cpp
// UIScreen for CEF content requires both options:
screen->setPremultipliedAlpha(true);  // CEF uses premultiplied alpha
screen->useBGRAFormat(true);          // CEF provides BGRA pixels
```

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

- `Manager.cpp/.hpp` - Main manager, Screen coordination, InputManager client, 4 shader programs
- `UIScreen.cpp/.hpp` - Logical Surface group with rendering options (premultiplied alpha, BGRA format)
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
auto hudScreen = overlayManager.createScreen("hud", true, true);

// Create Surfaces — each call pushes the new surface on top of the pile
auto minimap = hudScreen->createSurface("minimap");
minimap->setPosition(screenWidth - 210, 10);
minimap->setSize(200, 200);

auto healthBar = hudScreen->createSurface("health_bar");
healthBar->setPosition(10, 10);
healthBar->setSize(200, 20);
// healthBar is on top of minimap (it was created last)

// Reorder explicitly using the natural pile API:
hudScreen->bringToFront("minimap");          // minimap now on top
hudScreen->sendToBack("health_bar");         // health bar at the bottom
hudScreen->moveAbove("crosshair", "minimap"); // crosshair sits just above minimap

// Inspect the stack
auto order = hudScreen->stackOrder();        // bottom → top names
auto idx   = hudScreen->indexOf("minimap");  // std::optional<size_t>
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

ImGui screens are **retained**: you register a draw callback once via
`createImGUIScreen(name, drawFunction)`, and the manager invokes it every frame
while the screen is visible. There is **no** `beginImGuiFrame()/endImGuiFrame()`
API — the `NewFrame()/Render()` cycle is owned by `Manager::render()` (see below).

```cpp
// Register once (e.g. in a Core/Application init step). Hidden by default.
auto screen = overlayManager.createImGUIScreen("debug", [&] () {
    ImGui::Begin("Debug Info");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
    ImGui::End();
});

screen->setVisibility(true); // toggle on/off at will
```

**Single-cycle, multi-screen rendering model:** ImGui uses a single global
context, so exactly one `NewFrame()/Render()` pair is valid per frame.
`Manager::render()` therefore: (1) checks whether any `ImGUIScreen` is visible,
(2) opens one frame, (3) calls `draw()` on every visible screen between
`NewFrame()` and `Render()`, (4) submits all draw data once via
`ImGui_ImplVulkan_RenderDrawData()`. Individual `ImGUIScreen`s never run their own
frame cycle — `ImGUIScreen::draw()` only emits widgets (calls the draw function).
`Manager::render()`'s early-return guard considers **both** containers: an ImGUI-only
overlay (no `UIScreen`) still renders (`m_screens.empty() && m_ImGUIScreens.empty()`).

**ImGui version / backend API (1.92.8):**
- The Vulkan backend's `ImGui_ImplVulkan_InitInfo` no longer carries `RenderPass`
  / `Subpass`; they live in `PipelineInfoMain`. The overlay draws inside the
  post-process render pass, so `Manager::initImGUI()` sets
  `info.PipelineInfoMain.RenderPass = renderer.overlayFramebuffer()->renderPass()->handle()`
  and `Subpass = 0`. **Without this the backend silently creates no pipeline and
  nothing is drawn** (see `imgui_impl_vulkan.cpp` main-pipeline condition).
- Font atlas is **dynamic** since 1.92 (`ImGuiBackendFlags_RendererHasTextures`):
  `ImGui_ImplVulkan_CreateFontsTexture()` / `DestroyFontsTexture()` were removed
  and the atlas is created/updated automatically. Do not call them.
- `MinImageCount`/`ImageCount` are fed from `Renderer::framesInFlight()` (clamped
  to ≥ 2), not from the swap chain — the swap chain is private to the renderer.
- `info.Queue` needs a raw `VkQueue`; `Vulkan::Queue::handle()` exposes it for
  external-lib interop only. Engine code keeps using `submit()`/`present()`.
```

### CEF Integration (external)
```cpp
// WebView constructor - enable features BEFORE createOnHardware()
WebView::WebView(...) : Surface{...} {
    this->enableTransitionBuffer();  // For smooth resize

    // Texture upload path: "map" / "staging" / "auto" (auto resolved against the device).
    this->setMemoryMappingMode(Surface::parseMemoryMappingMode(
        settings.getOrSetDefault<std::string>("App/CEF/TextureUploadStrategy", "auto")));
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

- **Stack ordering owned by UIScreen**: Surfaces never carry a public depth. Use `bringToFront`, `sendToBack`, `moveAbove`, `moveBelow` etc. on the parent screen. The internal `m_depth` is recomputed automatically and is only used for the model matrix Z translation.
- **Stack convention**: index 0 = bottom (drawn first), index N-1 = top (drawn last, visible above). Input dispatch is the reverse — topmost surface gets events first.
- **Creation pushes on top**: every `createSurface` adds the new surface to the top of the pile. Position elsewhere with a follow-up call to `sendToBack()` / `moveBelow()` if needed.
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
- **Provider size is authoritative:** the strict-equality commit means a ±1 px divergence between the engine's surface-size formula and the provider's device-scale rounding (fractional display scales, e.g. 125%) stalls the resize → black render. On a no-match frame the provider must call `requestTransitionBufferResize(paintedW, paintedH)` (instead of dropping the frame); the render thread then recreates the transition buffer at the painted size via `recreateTransitionBufferToRequestedSize()` (`processUpdates()` Step 1.b). Do **not** call `onTransitionBufferReady()` from that dedicated path (it re-triggers the provider's resize and risks a loop). **The provider must NOT gate this request on `isTransitionBufferReady()`** — at startup no transition buffer exists yet, so gating it there leaves the *active* buffer permanently 1 px off and the screen black from frame zero (`recreateTransitionBufferToRequestedSize()` creates the buffer from scratch, so the request is valid even with no transition buffer present). Belt-and-braces, the provider should *also* keep a clamped-blit safety net into the active buffer for no-match frames so a sub-pixel divergence never produces a black frame even before convergence — see app_system `src/UI/AGENTS.md § Resize Commit` and `WebView.CefRenderHandler.cpp`.
- **Threading:** `requestTransitionBufferResize()` is provider-thread-safe (records size under `m_requestedTransitionSizeMutex`, no GPU work). All GPU (re)creation stays on the render thread inside `processUpdates()` under `m_framebufferAccess`; the `Resizing` status blocks provider writes/commits during recreation.
- **Degenerate (0 px) size is transient, not an error:** during an aggressive resize or a minimize the framebuffer can momentarily report 0 px, so `getSurfaceWidth/Height()` yields a 0-sized surface. `updatePhysicalRepresentation()` guards this at the top (`if ( textureWidth == 0 || textureHeight == 0 ) return true;`) and **defers** recreation, keeping the current buffer. Without the guard, `Pixmap::initialize(0, 0)` fails → `updatePhysicalRepresentation()` returns false → `UIScreen::processSurfaceUpdates()` **permanently disables the whole screen** (every surface on it vanishes). The next resize event back to a valid size re-invalidates and recreates correctly. This protects engine-internal surfaces (Notifier) and external-provider surfaces (CEF) alike.

## Detailed Documentation

Related systems:
- @docs/saphir-shader-system.md - OverlayGenerator (2D pipeline)
- @src/Input/AGENTS.md - Input system (polling + events)
- @src/Graphics/AGENTS.md - Renderer and pipelines
