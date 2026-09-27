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

### The Physical Camera Panel (Shift+F2)

`Core::initializeCoreScreen()` registers an ImGUI screen named `PhysicalCameraScreen`, hidden by
default and toggled by **Shift+F2** — a shortcut that used to force a swap-chain re-creation
(owner decision 2026-07-26: none of the twelve Shift+F<n> was free, and that experimental debug
path was the least used). Requires `-DEMERAUDE_ENABLE_IMGUI=On`; without it the shortcut only
traces a hint.

It binds the ACTIVE camera, which is the single source of truth for the photographic behaviour of
the image: optics (focal length, f-number, sensor width, focus), the exposure triad (shutter, ISO,
EV compensation) and the effects the camera materializes (depth of field, HDR/tone mapping, lens
glare with its threshold in nits). At the bottom it prints the **EV100 and the resulting exposure
multiplier from the same APEX equation the tone mapper uses** — so a setting that reads wrong in
the panel IS wrong on screen. That readout is the point of the panel, not a decoration.

⚠️ With auto-ISO on, the ISO slider is DISABLED (it shows the manual fallback value, not what is
in effect) and the METERED values are displayed next to it instead: "metered: ISO N | scene avg
X nits", read back from the GPU adaptation history with framesInFlight frames of latency
(`ToneMapping::meteredSensitivity()/meteredLuminance()` via
`PostProcessStack::cameraToneMapping()`). The auto-focus line does the same with the rack-focus
position ("measured: focus at X m", `DepthOfField::meteredFocusDistance()`). Both are
render-thread frame-scope reads, which is exactly where the panel draws.

⚠️ **Scene access from an ImGui/overlay draw callback (2026-07-26)**: draw callbacks run on the
RENDER THREAD, inside the frame's `withSharedActiveScene()` scope (Core's render block holds the
scene manager's shared lock for the whole frame). They MUST read the scene through
`Core::m_frameScene` — the pointer Core publishes for exactly that scope — and NEVER call
`withSharedActiveScene()` again: re-acquiring a `std::shared_mutex` the thread already
share-owns is undefined behaviour, and deadlocks the moment an exclusive writer (pause/resume,
scene replacement) is queued — on Windows SRWLOCK first, and on every OS since the scene manager's
writer-preference gate (2026-09-24, `src/Scenes/AGENTS.md`). The camera panel is the reference example.

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
- **Provider size is authoritative:** the strict-equality commit means a ±1 px divergence between the engine's surface-size formula and the provider's device-scale rounding (fractional display scales, e.g. 125%) stalls the resize → black render. On a no-match frame the provider must call `requestTransitionBufferResize(paintedW, paintedH)` (instead of dropping the frame); the render thread then recreates the transition buffer at the painted size via `recreateTransitionBufferToRequestedSize()` (`processUpdates()` Step 1.b). Do **not** call `onTransitionBufferReady()` from that dedicated path (it re-triggers the provider's resize and risks a loop). **The provider must NOT gate this request on `isTransitionBufferReady()`** — at startup no transition buffer exists yet, so gating it there leaves the *active* buffer permanently 1 px off and the screen black from frame zero (`recreateTransitionBufferToRequestedSize()` creates the buffer from scratch, so the request is valid even with no transition buffer present). Belt-and-braces, the provider should *also* keep a clamped-blit safety net into the active buffer for no-match frames so a sub-pixel divergence never produces a black frame even before convergence — see the consumer's `src/UI/AGENTS.md § Resize Commit` and `WebView.CefRenderHandler.cpp`.
- **Threading:** `requestTransitionBufferResize()` is provider-thread-safe (records size under `m_requestedTransitionSizeMutex`, no GPU work). All GPU (re)creation stays on the render thread inside `processUpdates()` under `m_framebufferAccess`; the `Resizing` status blocks provider writes/commits during recreation.
- **Degenerate (0 px) size is transient, not an error:** during an aggressive resize or a minimize the framebuffer can momentarily report 0 px, so `getSurfaceWidth/Height()` yields a 0-sized surface. `updatePhysicalRepresentation()` guards this at the top (`if ( textureWidth == 0 || textureHeight == 0 ) return true;`) and **defers** recreation, keeping the current buffer. Without the guard, `Pixmap::initialize(0, 0)` fails → `updatePhysicalRepresentation()` returns false → `UIScreen::processSurfaceUpdates()` **permanently disables the whole screen** (every surface on it vanishes). The next resize event back to a valid size re-invalidates and recreates correctly. This protects engine-internal surfaces (Notifier) and external-provider surfaces (CEF) alike.

## Detailed Documentation

Related systems:
- [`../../docs/saphir-shader-system.md`](../../saphir-shader-system.md) - OverlayGenerator (2D pipeline)
- [`../Input/AGENTS.md`](../../../src/Input/AGENTS.md) - Input system (polling + events)
- [`../Graphics/AGENTS.md`](../../../src/Graphics/AGENTS.md) - Renderer and pipelines
