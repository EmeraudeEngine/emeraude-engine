# Render Targets & Off-Screen Rendering

This document details the architecture for rendering to off-screen targets, including textures, shadow maps, and cubemaps.

## Render Target Types

| Type | Vulkan Image Type | Use Case |
|---|---|---|
| **Texture** | `2D` | Security cameras, portals, mirrors (Color + Depth). |
| **ShadowMap** | `2D` | Depth-only rendering from light perspective. |
| **View** | `Swapchain` | Main window rendering. |
| **Cubemap** | `Cube` | 360° environment, reflection probes (6 faces). |

```cpp
// Example: Security camera feed
auto cameraFeed = RenderTarget::Texture::create(
    "security_camera", 512, 512, precisions, viewDistance, isOrthographic
);
material->setTexture("diffuse", cameraFeed->colorTexture());
```

## Image Usage Flags — who may be cleared, who may be read back

Every image a render target allocates must declare, **at creation**, the transfer rights the
engine will later exercise on it. A layout transition grants a layout, never a usage; getting
this wrong produces a VUID cascade in which only the first message names the real fault (see
`docs/caution-points.md` § Vulkan Validation).

| Target | Image | Usage | Why |
|---|---|---|---|
| `ShadowMap` | depth | `DEPTH_STENCIL_ATTACHMENT \| SAMPLED \| TRANSFER_DST` | `TRANSFER_DST` is required by the initial `clearDepthImage(1.0F)` — see § 4 below. Dropping it disables the shadow pass entirely. `+ TRANSFER_SRC` ONLY in a run launched with `--enable-depth-readback` (`Renderer::isDepthReadbackEnabled()`): the depth is then readable back; never by default, it is the usage that can cost depth compression. |
| `Texture` | color | `COLOR_ATTACHMENT \| SAMPLED \| TRANSFER_SRC` (`+ STORAGE` when GGX-convolved) | `TRANSFER_SRC` makes the target readable back: `capture()` and the `dumpRenderTarget` console command, one array layer per call (see below). |
| `Texture` | depth | `DEPTH_STENCIL_ATTACHMENT` | Deliberately **no** `TRANSFER_SRC`: nothing reads it back, and that flag is the one that costs depth-compression metadata on some architectures. |
| `SceneRenderTarget` | all color + depth | `… \| TRANSFER_SRC` | Every G-buffer attachment is blitted to the grab pass. |
| `SwapChain` | color | `COLOR_ATTACHMENT \| TRANSFER_SRC` | Screenshots. |

> [!IMPORTANT]
> Adding a readback (`TransferManager::downloadImage()`, `vkCmdCopyImageToBuffer`) or a clear
> (`clearDepthImage()`, `clearColorImage()`) to an existing target means **editing its
> `createImages()` first**. `downloadImage()` refuses an image without `TRANSFER_SRC_BIT`
> outright — there is no fallback, by design.

### Reading a target back — one array layer, as bytes or as floats (2026-11-14)

`TransferManager::downloadImage(image, arrayLayer, layout, aspect, pixmap)` copies ONE array layer of mip 0: a cubemap
face (0 = +X, 1 = -X, 2 = +Y, 3 = -Y, 4 = +Z, 5 = -Z), a shadow cascade, 0 for a plain image. The layer is never
clamped: one the image does not have is refused (the caller would get another layer's pixels without knowing it). The
barriers move the WHOLE image to `TRANSFER_SRC_OPTIMAL` and back, so every layer must be in the layout the caller
names. Two overloads, chosen by the pixmap: `Pixmap< uint8_t >` for the 8-bit colour and the depth / stencil formats,
`Pixmap< float >` for `R16G16B16A16_SFLOAT`. Each refuses the other's formats; both share the refusals
(`isImageLayerDownloadable()`) and the GPU copy (`copyImageLayerToHostBuffer()`), and size the staging buffer from the
image's real format.

- Until 2026-11-14 the function had no layer parameter and copied `baseArrayLayer = 0`: `ShadowMap::capture()` and
  `Texture::capture()` validated their `layerIndex`, then returned layer 0 (found by cppcheck, `unreadVariable`).
  They now pass it; `SceneRenderTarget` and `ImposterAtlas` have single-layer images and pass 0;
  `ImposterBake`, `SelectionDepthTarget` and `SwapChain` do not download.
- ⚠️ **Formats**: as bytes, 8-bit RGB(A) / BGR(A) (UNORM or sRGB) and the depth / stencil formats; as floats,
  `R16G16B16A16_SFLOAT` (the environment probes, the scene's HDR target), decoded with the foundation's
  `Math::halfToFloat()` — the values the GPU wrote, linear, a NaN or an infinity included. Anything else is REFUSED
  (owner ruling 2026-10-01: sized as RGBA8, a 16-bit target overflowed its staging buffer).
- `RenderTarget::*::capture()` still fills 8-bit pixmaps: on a floating-point target it fails with the refusal above
  (no caller in the cascade today). The console command `dumpRenderTarget` is the reader of an HDR target: it writes
  the linear values as a Radiance `.hdr` and an exposed 8-bit PNG (`docs/ai-runtime-control.md` § *Dumping a render-to-texture target or a shadow map*).
- ⚠️ A `Texture`'s depth image has no `TRANSFER_SRC` (table above): `capture(…, withDepthBuffer = true, …)` gets a
  warning and an empty `result[1]`.
- **A depth is read back as floats** (2026-11-14). The `Pixmap< float >` overload takes `VK_IMAGE_ASPECT_DEPTH_BIT`
  and gives a one-channel pixmap in [0, 1]: `D32_SFLOAT` / `D32_SFLOAT_S8_UINT` as they are, `D16_UNORM(_S8_UINT)`
  over 65535, `X8_D24_UNORM_PACK32` / `D24_UNORM_S8_UINT` from the low 24 bits over 16777215. A combined depth /
  stencil image is moved to the transfer layout by BOTH aspects; a shadow map is read from
  `DEPTH_STENCIL_READ_ONLY_OPTIMAL`, where its render pass leaves it. ⚠️ Only `D32_SFLOAT` (what a shadow map gets
  on the measured GPU) has RUN: the two UNORM decodings and the combined formats are written from the specification.
- The 8-bit overload REFUSES a depth format at once. Until 2026-11-14 it sized the depth at four bytes per pixel and
  handed them to a one-channel 8-bit pixmap, which refused that size after the GPU copy: no depth had ever been read
  back. `ShadowMap::capture()`, `Texture::capture(withDepthBuffer)` and `SceneRenderTarget::capture()` call that
  overload: they still fail on their depth (no caller in the cascade); the stencil of a combined format is not read.
- **A shadow map is dumped by `dumpRenderTarget`** (it walks the render-to-textures, then the shadow maps): a grey
  PNG, the depths found stretched from black (nearest) to white, and a `.pfm` holding the depths as 32-bit floats
  (emeraude-base `FileFormatPFM`; a Radiance file keeps about 1 % of relative precision — 0.990 and 0.995 would be
  one value). The reply gives the minimum, the maximum, the mean and the count of pixels still at the far plane.
  ⚠️ The shadow maps must have been created readable: launch with `--enable-depth-readback`, else the command says so.
- **Measured, the depth read-back** (2026-11-14, `light-and-shadow-debug`, RTX 3070 Ti, Debug, validation layer
  loaded, shadow maps in `D32_SFLOAT`). Without the switch: "holds a depth image created without read-back: launch
  with --enable-depth-readback". With it: `SunShadowMap` (4 cascades of 2048²) layers 0 and 3 dumped, layer 4 refused;
  `BulbShadowMap` (a cube of 1024²) layer 0 — depths 0.139505 to 1, 566 073 of 1 048 576 pixels at the far plane — and
  layer 5, entirely at 1; `SpotShadowMap` depths 0.987627 to 1 (a perspective depth crowds near 1). 0 `VUID-`,
  0 `[Error]`, exit 0. Checked on the files: the minimum, maximum, mean and far-plane count recomputed from each
  `.pfm` equal the reply's, and every sampled PNG byte equals the stretch of its float (the `.pfm` rows read from
  the bottom). The images show the ground, a sphere, a cube and a palm tree as the lights see them.
- **Measured** (`offscreen-rendering`, RTX 3070 Ti, Debug, validation layer loaded, 0 `VUID-`): the 2D target
  (`SecurityCubemap` that day, `SecurityTexture2D` since its renaming; one layer) dumps layer 0 and refuses 1 to 6 ("has 1 layer(s)"); the real cubemap
  (`OffscreenRenderingCubemap`) refuses layer 6 ("has 6 layer(s)") and, being HDR, cannot be downloaded. With the probe
  created in 8 bits and without convolution FOR THE MEASUREMENT (a local change, not committed), its six faces were
  dumped: every pair differs on 47 % to 83 % of its pixels, layer 0 dumped twice 21 s apart is identical bit for bit,
  and the faces read as the four sides, the sky (+Y, layer 2) and the ground (-Y, layer 3) of the night city.
- **Measured, the HDR read-back** (2026-11-14, `offscreen-rendering`, RTX 3070 Ti, Debug, validation layer loaded,
  no local change): the six faces of `OffscreenRenderingCubemap` (1024², `R16G16B16A16_SFLOAT`) are dumped, each as
  a PNG and a `.hdr`; automatic exposures between 2.37 and 3.03 EV (99th percentiles 0.123 to 0.193: a night
  scene); layer 6 refused; `…, 0, -3` gives the same face at -3 EV; `…, 0, 100` is refused; the 8-bit
  `SecurityTexture2D` still gives its PNG, with a warning when an exposure is passed. 0 `VUID-`, 0 `[Error]`,
  exit 0. Checked on the files: the percentile recomputed from each `.hdr` matches the reply within the RGBE
  precision (0.147461 for 0.147705), and on 241 980 samples per face every PNG byte lies inside the interval its
  `.hdr` value allows; the six faces differ pairwise on 47 % to 83 % of their pixels, as the 8-bit probe did.
- **Peers (2026-11-14, engine `466f2527`):** macOS (Apple M6) and Windows (MSVC 14.44, RTX 3500 Ada), Release,
  validation layer loaded, `offscreen-rendering`: `SecurityTexture2D` dumps its layer 0 (a 1024×1024 RGBA image) and
  refuses layers 1 and -1, `OffscreenRenderingCubemap` refuses layer 6 and its HDR download (the one expected
  `[Error]`), no target answers to `SecurityCubemap` any more; 0 `VUID-`, exit 0. In a Release build the refusal line
  names the image `''`: the debug identifier is not kept there, read the target's name in the command's reply.
- **Peers, the HDR and the depth read-back (2026-11-15, engine `a909e0d2`, base `ce85f4a`):** macOS (Apple M6,
  MoltenVK) and Windows (MSVC 14.44, RTX 3500 Ada), Release, validation layer loaded, 0 `VUID-`, 0 `[Error]`, exit 0.
  `OffscreenRenderingCubemap` layer 0: exposure 2.75802 EV / percentile 0.147827 on macOS, 2.75921 / 0.147705 on
  Windows as on Linux; layer 2: 2.37247 EV / 0.193115 on the three; each `.hdr` 4 194 355 bytes; the refusals of
  layer 6 and of an exposure of 100 word for word; an 8-bit target ignores the exposure with its warning. Depth
  (`light-and-shadow-debug`, `--enable-depth-readback`): without the switch the refusal; with it `SunShadowMap`
  layer 1 mean 0.606477 (macOS) / 0.606481 (Windows), 1 433 600 pixels at the far plane on both, `.pfm` 16 777 234
  bytes; `SpotShadowMap` depths 0.987627 to 1 on both; `BulbShadowMap` layer 2 right on macOS (845 117 pixels at
  the far plane) and ENTIRELY at the far plane on Windows — a read racing the cube's pass, see the second ⚠️ below.
  No OS states its depth format in the log; the 32-bit float is inferred from the data (199 257 distinct depths
  in one face): the D16 / D24 decodings have still never run.
- ⚠️ **A uniform dump is first of all a picture** (2026-11-15; the former item
  `dump-render-target-first-dump-solid-blue`, opened 2026-10-01, closed that day). On Windows the first
  `dumpRenderTarget(SecurityTexture2D)` of a session of `offscreen-rendering`, 30 s after the start line, was
  sometimes ONE colour — (0, 0, 255, 255) on all 1 048 576 pixels, a 6502-byte PNG — and the next dump always
  normal: 3 launches of 6 on 2026-10-01 and again on 2026-11-15 (RTX 3500 Ada), 0 of 6 in a series with slower
  launches; 0 `VUID-`. It was read for six weeks as a wrong read (a console thread racing the render thread). It
  is what that camera renders for a quarter of each of its turns:
  - thirty dumps about 1.3 s apart on Linux (RTX 3070 Ti, Release) are solid blue at 9.30 and 10.55 s, 19.66 and
    20.92 s, 30.06 and 31.31 s, 40.46 and 41.72 s after the start line, and normal at the 22 other instants: two
    samples out of eight, with a period of 10.4 s — the camera's (its entity is yawed by 0.01 rad per logic tick,
    one turn in 10.47 s);
  - the `TV` of the scene, which shows the target, is the same single colour at those moments ((0, 0, 25) on a
    screenshot, after the exposure);
  - a blue phase starts between 28.75 and 30.06 s and ends between 31.31 and 32.56 s: a dump at 30 s sits on its
    leading edge (3 of 6), one at 32 s on its trailing edge (0 of 6). The "first" dump was never special: the
    30th second was.
  What the camera looks at during that quarter turn is the item `camera-debug-model-in-its-own-target`.
  **Rule:** before suspecting the transfer, read the target SEVERAL times over a few seconds, and look at whatever
  shows it in the scene. The blue is not the clear colour either (render-to-texture clears with the renderer's,
  black).
- ⚠️ **…and a layer read EMPTY from a target redrawn every frame is first of all a racing read** (2026-11-15,
  item `render-target-read-back-races-the-render-pass`). The dump copies the image from the console thread, on
  another queue than the one which renders it: nothing orders the copy against the pass. In some launches (3 of 24
  on Linux, Release) faces 4 and 5 of a point light's shadow cube read entirely at the far plane at EVERY read, the
  four others right — the copy lands between the two groups of views the driver draws. It read like a hole in the
  shadow map for a few hours. The proof that it is not: switch the light off (its map is suspended, nobody draws the
  image) and read again — the six faces are there; a RenderDoc capture of the same launch replays six right faces.
  **Rule:** to read a target that is redrawn every frame, suspend it first; and never conclude on the CONTENT from a
  read which changed after the scene changed.
- ⚠️ **Two dumps of the same layer within one second: the second is refused** ("the file … already exists", an
  `[Error]` of `FileIO::write()`): the file name holds the second (`rt-dump-<seconds>-layer<N>`). Space the dumps
  of a series by more than a second (item `dump-render-target-same-second-refused`).

## Architecture

### 1. Image Layout Management
Render targets transition layouts automatically during the frame:
1.  **UNDEFINED** → Initial state.
2.  **SHADER_READ_ONLY_OPTIMAL** → Before/After render (Reading as texture).
3.  **COLOR_ATTACHMENT_OPTIMAL** → During render (Writing).

### 2. GPU Synchronization
Main scene rendering must wait for off-screen rendering to complete.
- **Semaphores**: Each RT has a semaphore signaled on completion.
- **Wait Stages**: Main render waits at `FRAGMENT_SHADER_BIT` (sampling) for these semaphores.

### 3. Cubemap Rendering (Multiview)

**Goal**: Render 6 faces of a cubemap in a single pass.

**Traditional vs Multiview:**
- **Traditional**: 6 Render Passes. Expensive.
- **Multiview**: **1 Render Pass**. GPU broadcasts geometry to 6 layers.

**Implementation:**
- **Layered Framebuffer**: 6 layers (+X, -X, +Y, -Y, +Z, -Z).
- **Shader Access**: `gl_ViewIndex` (0-5) identifies the face.
- **Vulkan Extension**: `VK_KHR_multiview` + `GL_EXT_multiview` GLSL extension.

**UBO Layout:**
```glsl
struct CubemapFace {
    mat4 viewMatrix;              // Per-face view matrix
};

layout(std140, set = 0, binding = 0) uniform CubemapView {
    CubemapFace instance[6];      // Per-face data (indexed by gl_ViewIndex)
    mat4 projectionMatrix;        // Shared: 90° FOV, 1:1 aspect
    vec4 positionWorldSpace;      // Shared: camera position
    vec4 ambientLightColor;       // Shared: lighting
    float ambientLightIntensity;
} ubView;

// Shader access pattern
mat4 view = ubView.instance[gl_ViewIndex].viewMatrix;  // Per-face
mat4 proj = ubView.projectionMatrix;                    // Shared
```

**Matrix Sources in Cubemap Mode:**

| Matrix | Source | Reason |
|--------|--------|--------|
| Projection | UBO (shared) | Same 90° FOV for all faces |
| View | UBO (indexed) | Different orientation per face |
| Model | Push Constant | Per-object transform |

**Push Constant in Cubemap Mode:**
```glsl
// Cubemap mode: ONLY model matrix
layout(push_constant) uniform Matrices {
    mat4 modelMatrix;  // NOT modelViewProjectionMatrix!
} pcMatrices;
```

**Shader MVP Computation:**
```glsl
// Cubemap mode: combine from UBO + push constant
mat4 MVP = ubView.projectionMatrix
         * ubView.instance[gl_ViewIndex].viewMatrix
         * pcMatrices.modelMatrix;
```

**CPU-Side Coordination:**
See `RenderableInstance/Unique.cpp:pushMatricesForRendering()` - pushes only `modelMatrix` when `passContext.isCubemap` is true.

**Saphir Generator Adaptation:**
- `RenderPassContext.isCubemap` flag propagates to shader generators
- `VertexShader.isCubemapModeEnabled()` gates matrix source selection
- All code using view matrix must check: UBO (`ViewUB`) vs push constant (`MatrixPC`)
- See: `src/Saphir/AGENTS.md` section "Cubemap Rendering Mode"

### 4. Shadow Map Render Targets

Shadow maps are depth-only render targets created per light.

**Types by Light:**
- **Directional/Spot:** 2D depth texture
- **Point:** Cubemap depth texture (6 faces)

**Global Control:**
Shadow mapping can be globally disabled via `GraphicsShadowMappingEnabledKey` setting. When disabled:
1. `Scene::renderShadowMaps()` returns early
2. Shadow map images remain in `VK_IMAGE_LAYOUT_UNDEFINED`
3. Lighting passes use base pass types (no shadow sampling in generated shaders)

**Critical:** If shadow maps are not rendered but their descriptors are bound, Vulkan validation errors occur due to layout mismatch.

See [`docs/shadow-mapping.md`](shadow-mapping.md) for complete shadow mapping architecture.

### 5. Recursive Rendering
**Limitation**: A texture showing itself (e.g., TV showing camera filming TV) causes infinite recursion / read-after-write hazard.
**Status**: Triggers validation warning. Future fix: Double buffering.

## View Matrices Lifecycle (CRITICAL)

Every render target owns a `ViewMatricesInterface` GPU resource (UBO + descriptor set),
returned by `viewMatrices()` and bound every frame by `RenderableInstance::Abstract::render()`
(`renderTarget->viewMatrices().descriptorSet()->handle()`).

**Ownership rule:** the view-matrices GPU resource lives and dies **with the render target**,
NOT with the input camera (the AVConsole device feeding it):

- **Created** in `RenderTarget::Abstract::createRenderTarget()` (after `onCreate()` succeeds),
  via the polymorphic `this->viewMatrices().create(renderer, this->id())`.
- **Destroyed** in `RenderTarget::Abstract::destroyRenderTarget()`, via `this->viewMatrices().destroy()`.
- **Camera connect/disconnect** (`onInputDeviceConnected` / `onInputDeviceDisconnected`) must
  only update the matrices **DATA** (`updateDeviceFromCoordinates()`), never the GPU resource
  lifecycle. These handlers are intentionally absent on all render targets (SwapChain, Texture,
  ShadowMap, View, SceneRenderTarget) — the base no-op is used.

**Why this matters (was a use-after-free):** previously each render target created its view
matrices on camera connect and **destroyed them on camera disconnect**. The rendering thread
synchronizes scene swaps only through `Scenes::Manager::m_activeSceneSharedAccess` (shared vs
exclusive lock); camera teardown goes through a *different* channel (AVConsole device
disconnection) that does **not** take that lock. So destroying the primary camera while the
scene was still active (e.g. `Stage::unloadActiveAct` resetting the player Act before disabling
the scene) freed the swap-chain descriptor set under the render thread → `descriptorSet()`
returned `nullptr` → crash in `DescriptorSet::handle()`. Tying the resource to the render
target lifetime removes the freeing path entirely.

**Init-order constraint:** because the swap-chain creates its view matrices inside
`createRenderTarget()` at renderer init, the main `DescriptorPool` must be created **before**
the swap-chain (headless in a window-less run, `src/Vulkan/AGENTS.md` § Headless swap-chain) in `Renderer::onInitialize()` (it is now created right after
`initializeSubServices()`, before the swap-chain creation).

A resize goes through `SwapChain::recreate()` / `fullRecreate()`, which only call
`updateViewProperties()` (data) — the view-matrices GPU resource survives a resize untouched.
