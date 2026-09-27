## Development Patterns

### Creating a New GPU Resource
1. Inherit from `AbstractDeviceDependentObject`
2. Implement RAII with appropriate destructor
3. Use VMA for memory allocations
4. Add necessary synchronizations
5. In `createOnHardware()`, after the handle exists and before `setCreated()`, call
   `setVulkanObjectName(device, VK_OBJECT_TYPE_…, handle)` (see *Debug Object Naming* above)

### Adding a New Pipeline
1. Define descriptors and layouts
2. Configure render states
3. Compile and cache SPIR-V shaders
4. Integrate with `LayoutManager`

> [!CRITICAL]
> **Pipeline Caching Rule**: `GraphicsPipeline::getHash()` MUST include the `RenderPass` handle!
>
> Vulkan pipelines are tied to specific render passes. The hash function takes a `RenderPass&` parameter
> and MUST include `renderPass.handle()` as its first hash component.
>
> See [`docs/pipeline-caching-system.md`](../../pipeline-caching-system.md) for complete caching architecture.
>
> ⚠️ **This hash is the engine's IN-MEMORY pipeline-object reuse** (level 3 of the
> ShaderModule / Program / GraphicsPipeline cascade: "have I already built this
> `GraphicsPipeline` object during this run?"). It is a different mechanism from the on-disk
> `VkPipelineCache` documented at the end of this file, which is what spares the DRIVER the
> compilation work. They are complementary — neither replaces the other.

### Headless swap-chain — the window-less run (Sep 2026)

`--window-less` makes the swap-chain **HEADLESS** (`SwapChain::isHeadless()`, from
`Window::isWindowLessMode()`): the same object, the same render passes, the same
`Renderer::renderFrame()` — HDR scene target and post-process chain included — with the color images
**owned by the engine** instead of a `VkSwapchainKHR`. Owner (2026-09-22): "il doit être capable comme le
mode avec fenêtre, ce n'est que du traitement vulkan au final, sans presentation".

| | presented | headless |
|---|---|---|
| images | `vkGetSwapchainImagesKHR` | `Image` created by `createColorBuffer()`, same format/extent/usage |
| create info | surface capabilities | filled by hand: fake window size, `B8G8R8A8_SRGB/UNORM`, 3 or 2 images |
| `acquireNextImage()` | `vkAcquireNextImageKHR` | next index in turn + an EMPTY submit signalling the image-available semaphore |
| `present()` | `vkQueuePresentKHR` | an EMPTY submit consuming the render-finished semaphore |
| final color layout | `PRESENT_SRC_KHR` | `COLOR_ATTACHMENT_OPTIMAL` — `PRESENT_SRC_KHR` is reserved to presentable images |
| capture | `Graphics::FrameCapture`, INSIDE the frame on the acquired image (both modes, since 2026-09-23); `SwapChain::capture()` refuses | same |

⚠️ **Every reader of a finished frame starts from `SwapChain::finalColorLayout()`**
(`Renderer::swapChainFinalColorLayout()`): the frame capture (`Graphics::FrameCapture`), the post-processor's grab source, the video
recorder's three read-backs (`Recorder.cpp`, `finalColorState()`: a presented image is left by the
presentation engine — `MEMORY_READ` at `BOTTOM_OF_PIPE` — a headless one by a color write).
It replaced a separate 8-bit `WindowLessView` drawn by a one-pass forward `renderOffscreenFrame()`,
which showed neither the HDR target nor the post-process chain and could not be captured; both are
DELETED. Measured 2026-09-22 on `relief`: window-less captures on the RTX 3070 Ti AND on the Intel
iGPU (which cannot present in the owner's Wayland session), 0 VUID, 0 UNASSIGNED. The frame size is the
fake window's (`m_state`, 1280×720 by default).

### SwapChain render passes (three variants)

`SwapChain` owns three render passes sharing the same attachments (color + depth,
single-sample outside the main MSAA pass):

| Pass | Load ops / initial layouts | Used by |
|------|---------------------------|---------|
| `createRenderPass()` | CLEAR from UNDEFINED (MSAA variant: 4 attachments + resolve) | Direct path RP1 — scene draws into the swap chain |
| `createPostProcessRenderPass()` | LOAD from ATTACHMENT | Direct path RP2 — content must survive the mid-frame grab-pass blit (end/restart) |
| `createOffscreenCompositeRenderPass()` | CLEAR from UNDEFINED, depth store DONT_CARE | Internal-target path RP-final — single pass doing transition + clear + composite + present (replaced the empty layout-establishing pass) |

The composite pass reuses the pipelines created against the `postProcess` pass, which is
only legal because the two passes are **compatible** — and compatibility requires the
**subpass dependency lists to be identical** (only load/store ops and layouts are
exempt). Never edit one pass's dependencies without mirroring the other, or every draw
fails `VUID-vkCmdDrawIndexed-renderPass-02684`. Full pipeline description:
[`docs/post-processing-pipeline.md`](../../post-processing-pipeline.md).

### Data Transfers
1. Use `TransferManager` for async transfers
2. Automatic staging buffers for large transfers
3. Fence synchronization for coherence
4. Batching of small transfers

#### The usage flags are part of the contract — declare them at CREATION

`TransferManager` operates on images the *caller* created, and Vulkan grants transfer rights
through `VkImageCreateInfo::usage` alone. **A layout transition never substitutes for a missing
usage flag** — this has bitten the engine four times in two months, in both directions:

| Operation | Required at image creation |
|---|---|
| `clearDepthImage()` / `clearColorImage()`, any barrier to/from `TRANSFER_DST_OPTIMAL` | `VK_IMAGE_USAGE_TRANSFER_DST_BIT` |
| `downloadImage()`, `vkCmdCopyImageToBuffer`, `vkCmdBlitImage` as source | `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` |

`downloadImage()` **refuses** an image that does not declare `TRANSFER_SRC_BIT` and traces the
missing flag. It used to "fall back" on transitioning the source to `VK_IMAGE_LAYOUT_GENERAL` and
blitting it into a scratch image — invalid by construction (`vkCmdBlitImage` requires the usage
bit on `srcImage` whatever the layout), so that path was deleted rather than repaired. If a new
image must be read back, **add the flag where it is created**; do not reintroduce a fallback.

See `docs/caution-points.md` § Vulkan Validation for the three logged occurrences and the VUID
cascade a missing flag produces (only the FIRST VUID names the real fault).

#### Full image upload: a 3D image is ONE layer of `depth` slices (Sep 2026)

`ImageTransferOperation::transfer()` copies the WHOLE extent, depth included, each array layer
offset by `width × height × depth × pixelBytes`; `finalizeForGPU()` blits every mip with
`max(extent >> level, 1)` on all three axes. ⚠️ Both wrote a depth of 1 until the volumetric cloud
shapes became the first 3D images ever uploaded — slice 0 only, every other slice undefined, and
**no VUID** (a copy smaller than the image is legal). `docs/caution-points.md` § Vulkan Validation.

#### Partial image upload: `transferRegion()`, and why it needs its own path

`Image::writeDataRegion()` → `TransferManager::uploadImageRegion()` →
`ImageTransferOperation::transferRegion()` updates a **sub-region** of an image already resident on
the GPU, leaving every pixel outside it untouched. Reference consumer: the overlay `Surface`, which
uploads only the rows a CEF paint actually changed.

> [!CAUTION]
> ⚠️⚠️ **Do NOT implement a partial upload by reusing `transfer()` or `transferCompressed()` with a
> smaller region.** Both barrier from **`VK_IMAGE_LAYOUT_UNDEFINED`**, and `UNDEFINED` explicitly
> permits the driver to **discard the existing contents**. For a full-image upload that is correct
> and even optimal — nothing is preserved because everything is rewritten. For a partial one it is
> undefined behaviour: the rows you did not copy may come back as garbage.
>
> The trap is that most drivers do not actually discard, so the mistake **works on the machine you
> test it on** and corrupts the display somewhere else. `transferRegion()` therefore barriers from
> `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` instead, and **refuses** any image not currently in
> that layout rather than guessing.
>
> ⚠️ That refusal is also the *feature*: an image only reaches `SHADER_READ_ONLY_OPTIMAL` after a
> complete upload, so the layout check **is** the "has been fully uploaded at least once" predicate.
> A fresh image, a recreated one (resize, DPI change) is `UNDEFINED` and gets a full upload, which
> re-arms the partial path for the frames after it. No caller-side flag to keep in sync, so none to
> get wrong. A caller must still handle `false` by falling back to a full `writeData()`.
