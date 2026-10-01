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

### Framebuffer-properties latch (asynchronous-provider surfaces)

`Surface` exposes two views of the framebuffer properties:

- **`framebufferProperties()`** — the *live*, shared `FramebufferProperties` (a `const&` chain up to the overlay manager). It mutates the instant the OS content scale/size flips (cross-monitor move, fractional-scale change). Engine-facing sizing reads this.
- **`latchedProperties()`** — a *value-copy snapshot* that only advances when the surface calls **`syncPropertiesLatch()`** (protected). Seeded to the live properties in the ctor.

**Why the latch exists.** An asynchronous / out-of-process content provider (an OSR web-view is the archetype) answers scale/size queries and submits frames on its own schedule. If it read the live properties, it could observe a *half-applied* transition — a new `device_scale_factor` with the old view size — and submit a `CompositorFrame` whose scale mismatches the last surface resync (the `cc/mojo_embedder` `last_submitted_device_scale_factor_ != frame.device_scale_factor()` DCHECK / renderer death). Reading `latchedProperties()` guarantees a **coherent (scale, size) tuple** that only moves in lock-step with the provider's own buffer transition, when it calls `syncPropertiesLatch()`.

This is the generic engine primitive; the CEF-specific ordering (which notifications fire around the sync) lives in the consumer. projet-alpha's `WebView::refreshFramebuffer()` calls `syncPropertiesLatch()` then pushes `NotifyScreenInfoChanged()`/`WasResized()`/`Invalidate()` in a strict order. See `projet-alpha/src/UI/AGENTS.md` § Scale transition contract.

**Code references:**
- `Surface.hpp:latchedProperties()` - Provider-facing coherent snapshot
- `Surface.hpp:syncPropertiesLatch()` - Advances the snapshot to the live properties (protected; provider decides *when*)
- Consumer side: the consumer's CEF render handler (`directPaint()`/`indirectPaint()`) request the resize on a size mismatch instead of dropping the frame

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

### Accelerated Source Mode (zero-copy GPU shared texture)

Third content path, exclusive with memory mapping (forces staging layout). Built for CEF
`OnAcceleratedPaint` (Windows/D3D11 and macOS/IOSurface). The surface image is a pure GPU→GPU copy target
(`DEVICE_LOCAL`, `OPTIMAL`, `TRANSFER_DST|SAMPLED`) with **no CPU pixmap and no staging upload**.

**API:**
- `enableAcceleratedSource()` — **before** `createOnHardware()` (like the other modes).
- `importAcceleratedFrame(const Vulkan::ExternalImageDescriptor &)` — callable from the content
  provider's thread. Synchronous by contract: import the external texture (platform dispatch
  `importExternalImage()` → `Image::importFromWin32Handle` on Windows,
  `Image::importFromIOSurface` on macOS), acquire barrier (`VK_QUEUE_FAMILY_EXTERNAL` → graphics
  on Windows; plain layout transition on macOS — the metal_objects image is not Vulkan external
  memory, and MoltenVK transitions are Metal no-ops so the content is preserved),
  `vkCmdCopyImage` (extent clamped to min), restore `SHADER_READ_ONLY`, submit on
  **`Renderer::graphicsQueue()`** (single frame queue → FIFO ordering vs in-flight frames — this is
  why the copy needs no cross-queue semaphores) and **wait the fence before returning** (the
  producer's handle is only valid during its callback — CEF 126 has no keyed mutex).
- Frame routing reuses the transition-buffer machinery (`determineTargetBuffer`): a
  transition-sized frame commits the swap inline; a mismatching frame is clamp-copied into the
  active buffer + `requestTransitionBufferResize` (same convergence rules as the CPU paths).
- One-shot GPU resources (transient `CommandPool` + `CommandBuffer` + `Fence`) are created lazily
  per surface; `createOnHardware()` caches the `Renderer *` for provider-thread access.

**Popup overlay (`importAcceleratedPopupFrame` + `setAcceleratedPopupVisible/Position`):**
external popup frames (e.g. CEF `<select>` dropdowns) are copied into a persistent surface-owned
GPU cache image and composited at the popup position on the active buffer. Because every
accelerated view frame is a FULL copy (erasing the popup), the view path re-composites the cache
after each frame while visible. On hide the cache is released — the next view frame erases the
popup on screen. Uses the region overload of `CommandBuffer::copyImage` (explicit dst offset).

**Limitations:**
- **Alpha-test event blocking degrades**: `isEventBlocked()` reads the CPU pixmap, which does not
  exist in this mode — surfaces needing per-pixel alpha routing must stay on the CPU path
  (the consumer decides per surface; see the consumer's `WebView`).
- Windows/D3D11 (`Win32D3D11Texture`) and macOS/IOSurface (`IOSurface`, via VK_EXT_metal_objects)
  are implemented; DmaBuf (Linux) is a descriptor placeholder.

### The transition-buffer placeholder is invisible (⚠️ measured cost)

On the **CPU-pixmap path only**, `updatePhysicalRepresentation()` builds a placeholder for the newly
sized transition buffer by bilinear-resizing the whole active pixmap
(`Processor< uint8_t >::resize(...)`, **serially** — the optional `ThreadPool *` argument is not
passed). The stated intent is "a placeholder image while waiting for new content".

**That placeholder is never displayed.** `descriptorSet()` only ever serves the active buffer, and a
transition buffer becomes active through `commitTransitionBuffer()` (or the inline swap of
`importAcceleratedFrame()`), both of which the content provider calls **after** writing a full frame
into it. The placeholder pixels are computed, uploaded, then overwritten before they could be
sampled. `TransitionBufferStatus::Ready` versus `WaitingForContent` changes nothing either:
`isTransitionBufferReady()` only excludes `Resizing`.

Measured on an Apple M4 Pro (`-O2`), one full-screen retina surface, 3456x1964 → 3456x1900:

| Step | Cost |
|---|---|
| `Processor::resize`, serial — what runs today | **28.6 ms** |
| the same with the 14-thread `ThreadPool` | 4.4 ms |
| `pixmap.initialize()` alone — the fallback when the copy is disabled | **1.6 ms** |

`Overlay::Manager::onWindowResized()` runs this for **every** surface on **every** resize event
(the only damping is `Core`'s `m_windowChanged` boolean, which merely coalesces what arrives between
two main-loop turns). With two full-screen CPU-pixmap surfaces that was ~56 ms of dead main-thread
work per event.

> [!IMPORTANT]
> **`disablePixmapCopyInTransitionBuffer(true)` is the answer, not a thread pool.** Parallelising
> the resize would still burn fourteen cores computing pixels nobody sees, and would still be three
> times slower than not doing it. AppSystem sets the flag on every `WebView` (`src/UI/WebView.cpp`,
> constructor). An engine surface that genuinely displays its transition buffer before content
> arrives would need the copy — none does today.

### Initial clear of the surface image (⚠️ not cosmetic — and only on the ACTIVE buffer)

Neither asynchronous-provider path writes a single pixel at creation time — the mapped path waits
for its CPU producer, the accelerated path for its first GPU→GPU copy. A buffer that can be
**sampled** before its first frame therefore shows raw device memory, which is `UNDEFINED`: desktop
drivers happen to hand back zeroed pages, while **Metal/MoltenVK hands back real garbage, reading as
a uniform magenta rectangle**. That was the pink flash the Lychee Slicer splash screen showed on
macOS before its image appeared (fixed 2026-09-20), on every CEF web-view and not only the splash.
Same class of bug, same fix, as `Graphics::IntermediateRenderTarget::create()`.

`createFramebufferResources()` takes a **`clearOnCreate`** parameter, and the caller decides:

| Call site | Buffer | `clearOnCreate` | Why |
|---|---|---|---|
| `createOnHardware()` | active | **true** | Sampled from the very next frame |
| `updatePhysicalRepresentation()`, single-buffer mode | active | **true** | Recreated in place and sampled immediately, nothing hides the gap |
| `updatePhysicalRepresentation()`, transition buffer | transition | **false** | Never sampled |
| `recreateTransitionBufferToRequestedSize()` | transition | **false** | Never sampled |

> [!IMPORTANT]
> **The transition buffer is never sampled, and that is what keeps the clear off the resize path.**
> `descriptorSet()` always serves `m_activeBuffer`, and a transition buffer only becomes active
> through `commitTransitionBuffer()` (or the inline swap in `importAcceleratedFrame()`), both of
> which the content provider calls **after** writing its frame. Clearing it would be pure cost:
> the two transition call sites are the window-**resize** path, which runs on every drag event, for
> every surface.
>
> ⚠️ **This is an invariant, not an optimisation detail.** Anyone who makes the transition buffer
> sampleable, or commits one without content, must flip those two call sites to `true` — otherwise
> the magenta comes back, and only on Apple hardware.

The two clear mechanisms differ because the two images differ:

| Path | Image | How it is cleared |
|---|---|---|
| Accelerated source | `OPTIMAL`, `TRANSFER_DST\|SAMPLED` | `UNDEFINED` → `TRANSFER_DST_OPTIMAL`, `TransferManager::clearColorImage()` to transparent black, → `SHADER_READ_ONLY_OPTIMAL` |
| Memory mapping | `LINEAR`, `SAMPLED` only, host visible | `memset` of the mapped memory through `Framebuffer::writeWithMapping()`, **after** the transition (a transition *from* `UNDEFINED` may discard the contents) |

> [!WARNING]
> `vkCmdClearColorImage()` is **illegal on the memory-mapping image**: it carries no
> `VK_IMAGE_USAGE_TRANSFER_DST_BIT` (see the usage flags above). Hence the two mechanisms.
> When `clearOnCreate` is false the image is transitioned straight to `SHADER_READ_ONLY_OPTIMAL`,
> exactly as before this fix existed.

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

### On-Demand Redraw Signal

Supports the engine's `RenderingMode::OnDemand` (see [`../AGENTS.md`](../../../src/AGENTS.md) § Core - On-Demand
Rendering). The overlay is the main dynamic content in an overlay-centric app, so it is the primary
source of "the screen changed, re-render" signals.

- **`Manager::RedrawRequested`** notification + **`Manager::requestRedraw()`** (emits it). `Core`
  observes the manager and turns it into `Core::requestRedraw()`. Harmless in `Continuous` mode
  (observers ignore it). Application code may also call `requestRedraw()` for a change not covered below.
- **Propagated requester callback** (`std::function< void () >`): `Manager::createScreen` installs a
  `[this]{ requestRedraw(); }` on each `UIScreen` (`UIScreen::setRedrawRequester`), which forwards it to
  every `Surface` it creates (`Surface::setRedrawRequester`). This is how a deep surface change reaches
  the manager without making `Surface` observable. Empty by default (continuous rendering).
- **Surface-level triggers** (`Surface::notifyRedrawRequired()`, fired by): `invalidate()` and
  `setVideoMemoryOutdated()` (content/size), `setPosition()`/`move()`/`setStackIndex()` (geometry & stack
  order — the six `UIScreen` reorder ops go through `setStackIndex`, so they are covered transitively),
  `show()`/`hide()` (visibility). Plus **`Surface::requestRedraw()`** — a public "re-composite without
  marking video memory outdated" used by a memory-mapped CEF paint (`directPaint` writes pixels straight
  to device memory, so it needs a wake but no re-upload).
- **Screen/manager-level triggers**: `UIScreen::setVisibility` (whole-screen show/hide),
  `UIScreen::destroySurface`/`clearSurfaces` (removal), `Manager::createScreen`/`destroyScreen`/
  `clearScreens`/`enable` (screen lifecycle & overlay master switch). Per-screen enable/disable
  (`Manager::enableScreen`/`disableScreen`/`toggleScreen`/`disableAllScreens`) go through
  `UIScreen::setVisibility` and are covered transitively.

> [!IMPORTANT]
> All triggers fire from the **producer/app thread** (never the render thread — `processUpdates()` reads
> the dirty flags but does not set them, and `updateModelMatrix()` is deliberately NOT a trigger because
> it is also called from `processUpdates`). This avoids the render thread re-arming its own redraw.
>
> **ImGUI screens are NOT wired** (immediate-mode, separate `m_ImGUIScreens`): an animating ImGUI overlay
> needs an explicit `Manager::requestRedraw()` or `Continuous` mode.

### Resize & scale notifications

`Manager::onWindowResized()` (driven by `Core::onWindowChanged()` after the renderer recreates the swap-chain) re-reads the window state, invalidates every surface, then emits:

- **`Manager::OverlayResized`** — always, on any settled window change. Payload `std::array< uint32_t, 2 >{framebufferWidth, framebufferHeight}` (physical pixels). Use it for size-driven work.
- **`Manager::OverlayScaleChanged`** — *in addition*, only when the HiDPI content scale actually changed (cross-monitor move / fractional-scale change; diff tolerance `0.001`). Payload `std::array< float, 2 >{contentXScale, contentYScale}`. The manager owns the previous-scale comparison, so observers no longer cache and diff the scale themselves — they key on this signal to react to scale specifically (e.g. an OSR-provider surface promptly re-latching + re-notifying its provider, without waiting a debounce).

Both fire from the manager under `m_physicalRepresentationUpdateMutex`. `contentXScale/Y` is the fractional window content scale (`glfwGetWindowContentScale`, e.g. 1.5 at 150%), never the integer-rounded per-monitor scale.

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
   - **Press observers get the release too.** During the top-down press walk, the surfaces
     walked *before* the consumer received `onButtonPress` without consuming it (typically an
     upper web-view processing unblocked events over a transparent zone). They are recorded
     (`m_pressObserverSurfaces` + `m_pressObserverButtons`) and `onButtonRelease` delivers the
     matching release to them alongside the captor (their return value is ignored — the captor
     alone decides the blocking). Without this, an observer was left with a press-without-release:
     its CEF/Blink side held a phantom button, which on Windows also shifted the synthesized DOM
     `contextmenu` (fired at mouseup there) to a later, wrong position. The list is cleared when
     the capture is freed, and unused when no surface consumed the press (the release then follows
     the normal stack walk and reaches the observers by itself).
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
- **Why it exists:** a control on an upper surface (e.g. a slider on a UI overlay) consumes the press and thus **captures** the pointer, so a lower surface (e.g. a 3D view) would normally see no moves during the drag. The tap lets that lower surface keep receiving the live move stream — e.g. to update a 3D scene in real time while the slider is dragged — without disturbing the capture/consume semantics. See the consumer's `Manager::setPointerMoveTapWebView` and `AppControl.overlayManager.setWebViewPointerMoveTap`.

> [!NOTE]
> Capture attaches to the surface that **consumes** the press (returns `true`). A surface that forwards events to its content without blocking propagation (`processUnblockedPointerEvents`-style) does not become the captor — the natural owner of a drag is the blocker beneath it. The CEF consumer side still needs its own "I already sent the mousedown, so I must send the matching mouseup" tracking for the case where the release lands on a now-transparent pixel of the captor itself (see the consumer's `WebView::m_CEFButtonsDown`). The two layers are complementary: `UIScreen` decides **which** surface gets the event; the surface decides **whether** to forward it to its backend.

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

### Dirty rects reach the GPU — the row-band upload

`processUpdates()` uploads **only the rows the provider actually changed**, read from
`Pixmap::updatedRegion()` and sent through `Image::writeDataRegion()`. Everything outside that band
is preserved on the GPU. It falls back to a full-image upload whenever a partial one is not provably
safe: no valid touched region, an image that never received a complete upload, a size mismatch, more
than one array layer or mip level, a band covering every row, or a failed partial transfer.

> [!CAUTION]
> ⚠️⚠️ **The upload used to be full-frame, and this section used to claim otherwise.** It read *"both
> paths support partial updates via CEF's `dirtyRects` parameter for optimal performance"* — true of
> the **blit into the pixmap**, false of everything after it. The upload submitted
> `MemoryRegion{pixmap.data().data(), pixmap.bytes()}`, `Buffer::writeData()` memcpy'd and flushed
> all of it, and `ImageTransferOperation` issued **one** `vkCmdCopyBufferToImage` at the full image
> extent. A single hovered button cost the same GPU upload as a full-page repaint.
>
> The band is full-width on purpose: the staging buffer layout is the linear image, so a range of
> rows is contiguous in both and needs no stride arithmetic and no row-by-row copy. A tight 2D
> sub-rectangle would move fewer bytes and costs exactly that.

> [!CAUTION]
> ⚠️ **`processUpdates()` CONSUMES the pixmap's updated-region marker** (`resetUpdatedRegionMarker()`
> after each successful upload). Before this, `Pixmap::updatedRegion()` was **write-only telemetry**:
> `Processor::blit()` maintained it on every blit and nothing in the engine, emeraude-base or a
> consuming application read it, so it accumulated the union of every blit since the surface was
> created. Consuming it is what makes the region mean "changed since the last upload" — i.e. what
> makes the row band correct. **A second consumer would now share this reset**: coordinate the
> ownership rather than adding another one.

### GPU upload statistics (diagnostic)

`Surface::UploadStatistics` accounts every GPU upload of a measurement window, and
`Manager::dumpUploadStatistics()` emits one `[UPLOAD-STATS]` tracer line per painted surface — at
most once per second — then clears the counters. Enable with
**`Core/Video/Overlay/EnableUploadStatistics`** (default `false`, read once at `Manager`
initialization).

| Counter | Meaning |
|---|---|
| `uploadCount` / `partialCount` | uploads performed, and how many took the row-band path |
| `uploadedBytes` | what was **actually** moved |
| `fullBytes` | what a full-image upload would have moved — the baseline the realised gain is computed against |
| `regionBytes` | what a tight bounding-box upload would move — the ceiling above the row band |
| `bandBytes` | what the current strategy targets |
| `writeDurationUS` | time spent in the upload, on the render thread |
| `saturatedCount` | uploads whose touched region already covered the whole pixmap |
| `unknownRegionCount` | uploads with no valid region — charged at full price, never ignored |

It answers three questions the row-band upload cannot answer about itself: whether the gain holds on
another machine, whether a rising `saturatedCount` means something started forcing full-frame
repaints again, and whether the gap between `bandBytes` and `regionBytes` justifies moving to a true
2D sub-rectangle.

> [!IMPORTANT]
> **Silence is a result, not a failure.** The dump rides `Manager::updateVideoMemory()`, which only
> runs when a frame is rendered, so an idle application produces **no line at all**. Each window's
> elapsed time is measured and divided out, so the rates stay correct however irregular the cadence.
>
> ⚠️ **Render thread only.** The counters are plain integers with no synchronisation: written by
> `processUpdates()`, read by `dumpUploadStatistics()`, both on the render thread.
>
> ⚠️ The **dump** is gated by the setting; the **accounting** is not. It costs a handful of integer
> additions and two clock reads per upload, at most once per painted frame — negligible, but not
> literally zero.

### ImGUI — two threading and input rules (2026-09-29)

> ⚠️⚠️ **ImGUI uploads its textures behind the engine's back.** ImGUI 1.92 (`ImGuiBackendFlags_RendererHasTextures`)
> updates the font atlas — on the first ImGUI frame, then whenever glyphs are added — INSIDE
> `ImGui_ImplVulkan_RenderDrawData()`, with its own `vkQueueSubmit()` + `vkQueueWaitIdle()` on the graphics queue
> given at init, and NOT under the `Vulkan::Device` lock every engine queue access takes (`Vulkan/Queue.cpp`). On a
> single queue family (the Apple M2) the resource loaders' uploads share that queue: macOS reported 11-12
> `UNASSIGNED-Threading-MultipleThreads-Write` (`vkQueueSubmit` from two threads) the moment the editor panel drew its
> first frame while the gizmos uploaded. Linux (NVIDIA, a separate transfer family) showed 0 — a Linux pass proves
> nothing here. `Overlay::Manager::render()` now calls `ImGui_ImplVulkan_UpdateTexture()` itself for every texture
> whose status is not `OK`, under the device lock, before `RenderDrawData()` (which skips an `OK` texture). Any other
> ImGUI backend call that submits must take the same lock. (`ImGui_ImplVulkanH_*` also submits, but only serves
> secondary platform windows — viewports are not enabled.)
>
> **ImGUI reads the pointer through its own GLFW callbacks**, in parallel with the engine's listeners. The overlay
> latches `io.WantCaptureMouse` after each `NewFrame()` (`m_ImGUICapturesPointer`) and CONSUMES a button press or a
> wheel event over an ImGUI window, so the scene and the editor behind it never see it. A RELEASE is never consumed:
> a drag started in the scene must end even over a window. An INJECTED event (`InputManagerService.mouseClick`,
> `keyPress`) reaches the engine callbacks only: it never presses an ImGUI widget.

## Threading and limits (triad 13, 2026-10-01)

- **The render-thread passes iterate SNAPSHOTS.** `Manager::updateVideoMemory()`, `onWindowResized()` and
  `dumpUploadStatistics()` iterate `m_screenSnapshot` (`snapshotScreens()` copies the screen pointers under
  `m_screensAccess`), and `UIScreen::processSurfaceUpdates()` iterates `m_surfaceSnapshot` (copied under
  `m_surfacesMutex`). Iterating `m_screens` / `m_surfaces` directly raced with `createScreen()` / `destroyScreen()`
  and the surface stack operations on the main thread (a rehash or an erase during the iteration). The locks are NOT
  held during the updates: a surface notifies observers (`OverlayResized`…) that may call back into the screen, and
  both mutexes are non-recursive. Both snapshot vectors are reused (no allocation per frame) and cleared after the
  pass, so a screen or a surface destroyed meanwhile does not outlive it. `createImGUIScreen()` takes `m_screensAccess`
  (`render()` reads `m_ImGUIScreens` under it). `UIScreen::surfaces()` returns a COPY taken under the lock.
- **A surface pixel size past the device's `maxImageDimension2D` is refused** (`Surface::fitsDeviceLimits()`), before
  any pixmap allocation, both in `updatePhysicalRepresentation()` and in `recreateTransitionBufferToRequestedSize()`:
  an error naming the surface and the size, the current buffer stays, and the next valid size recreates it (owner
  ruling). A huge size used to abort on the pixmap allocation (`-fno-exceptions`).
- **`FramebufferProperties` never converts an out-of-range float**: a NaN or infinite screen scale is 1 (as `<= 0`
  already was), and the pixel / point sizes go through `roundedToInteger()` (saturated, NaN = 0, which takes the
  existing 0-px transient path).

