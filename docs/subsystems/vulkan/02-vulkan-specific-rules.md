## Vulkan-Specific Rules

### Mandatory Abstraction
- **NEVER** call Vulkan functions directly from `Graphics/` or client code
- Use abstraction classes: `Device`, `Buffer`, `Image`, `Pipeline`, etc.
- All Vulkan resources must be encapsulated

### Debug Object Naming (mandatory for every Vulkan object)

> [!CRITICAL]
> **Every device-owned Vulkan object MUST forward its identifier to Vulkan** so validation
> messages and GPU captures (RenderDoc) show readable names instead of raw handles
> (`VkImageView 0x...`). This is not cosmetic: a multi-scene device-lost crash was diagnosed only
> because named objects revealed `PostProcessorService-…-Descriptor` `uPrimarySampler` = `0x0`
> (see [`docs/caution-points.md`](../../caution-points.md)).

- `setIdentifier(...)` only stores a **CPU-side** name. It runs **before** the handle exists, so
  it can NOT name the Vulkan object by itself.
- `AbstractObject::setVulkanObjectName(device, objectType, handle)` (in `AbstractObject.hpp`)
  forwards the stored identifier via `vkSetDebugUtilsObjectNameEXT`. It is a **no-op** when
  `VK_EXT_debug_utils` is unavailable (i.e. `EnableDebug` is off — see *Validation & debug-utils
  configuration* below) or the name is empty.
- **Call it inside `createOnHardware()`, right after the handle is created and before
  `setCreated()`:**
  ```cpp
  if ( const auto result = vkCreateXxx(device, &m_createInfo, nullptr, &m_handle); result != VK_SUCCESS ) { … }

  this->setVulkanObjectName(this->device()->handle(), VK_OBJECT_TYPE_XXX, reinterpret_cast< uint64_t >(m_handle));

  this->setCreated();
  ```
- **Coverage:** all device-owned objects are named — `Image`, `ImageView`, `DescriptorSet`,
  `AccelerationStructure`, `Buffer`, `CommandPool`, `ComputePipeline`, `DescriptorPool`,
  `DescriptorSetLayout`, `DeviceMemory`, `Framebuffer`, `GraphicsPipeline`, `PipelineLayout`,
  `RenderPass` (both v1 and v2 paths), `Sampler`, `ShaderModule`. **Any new Vulkan object type
  added to this layer MUST do the same.** (`DescriptorSet` is an `AbstractObject`, not an
  `AbstractDeviceDependentObject`, so it uses `m_descriptorPool->device()->handle()`.)

### Validation & debug-utils configuration

`EnableDebug` (`Core/Video/VulkanInstance/EnableDebug`, or the `--debug-vulkan` CLI switch) is the
**single master switch** for the whole `VK_EXT_debug_utils` channel. There is **no** separate
`UseDebugMessenger` key (removed 2026-06-22 — folded into this model). Behaviour:

| `EnableDebug` | `RequestedValidationLayers` | `debug_utils` ext (object naming) | `AvailableValidationLayers` mirrored to settings | validation layers loaded | debug messenger |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **false** | *(ignored)* | ✗ | ✗ | ✗ | ✗ |
| **true**  | empty       | ✓ | ✓ | ✗ | ✗ |
| **true**  | non-empty   | ✓ | ✓ | ✓ | ✓ |

- When `EnableDebug` is off, nothing debug-related touches the settings file.
- When on, the engine mirrors the system's available layers into `AvailableValidationLayers` so a
  human editing settings knows what to put in `RequestedValidationLayers` (that array is the only
  thing meant to be hand-edited; `Available…` is informational, the engine does not consume it).
- The debug messenger (routes validation messages into the engine `Tracer`) is created **only** when
  at least one validation layer is actually requested:
  `isUsingDebugMessenger()` == `m_debugMode && !m_requiredValidationLayers.empty()`. A
  `VkDebugUtilsMessengerEXT` cannot exist without the `debug_utils` extension, hence the dependency
  on `EnableDebug`.
- Object naming therefore works whenever `EnableDebug` is on, **independently of validation layers**
  — useful for clean RenderDoc/Nsight captures without validation overhead.

### GPU device-lost diagnostics (automatic)

`VK_ERROR_DEVICE_LOST` is reported **late**: the `vkQueueSubmit`/`vkWaitForFences` that returns it
is rarely the culprit — the GPU faulted on an *earlier* submission. To self-document the real fault
even in normal/release runs, the engine wires two **vendor-complementary** extensions whenever the
device advertises them (enabled in `Instance.cpp`, zero runtime cost until a fault occurs):

- **`VK_EXT_device_fault`** → faulting GPU virtual addresses (Mesa/AMD/Intel; **absent on the NVIDIA
  proprietary driver** as of 550.x). Requires the `VkPhysicalDeviceFaultFeaturesEXT.deviceFault`
  feature, chained in `DeviceRequirements`.
- **`VK_NV_device_diagnostic_checkpoints`** → the last GPU command region reached per queue (NVIDIA).

**`Device::dumpDeviceLostDiagnostics(context)`** is the single facility. It is called at every
DEVICE_LOST observation site — `Queue::submit`/`present`, `Fence::wait`/`waitAndReset`,
`Device::waitIdle` — and is **self-guarded (reports once per device)** and **takes no device lock**
(safe to call from inside a locked submit/wait path). It logs `device_fault` addresses + the last
checkpoint marker(s) reached. **The marker is the answer**: it names the GPU region executing when
the device died.

**Placing markers** — `Device::setCheckpoint(commandBuffer, "literal")` records a checkpoint
(no-op when the extension is absent). The marker **MUST** be a string literal (static storage —
it is read back *after* the loss). Markers are currently placed at the two crash-window submissions:
`AS-build:begin`/`:end` (`AccelerationStructureBuilder::submitOneShot`, covers all BLAS builds) and
`transfer:image-layout-transition` (`TransferManager`). **Add a `setCheckpoint` at any new
GPU-recording site you want to be able to blame** (render passes, TLAS inline build, compute
dispatches).

### GPU Memory Management
- **VMA mandatory** for all GPU memory allocations
- Use `MemoryRegion` and `DeviceMemory` for encapsulation
- RAII for automatic Vulkan resource management

> [!IMPORTANT]
> **`vk_mem_alloc.h` is `.cpp`-only — never include it from a header.** VMA's interface
> section is ~20 000 lines of templates, and `Device.hpp` sits at the root of the wrapper
> hierarchy: including it there re-parsed those lines in nearly every Vulkan/Graphics TU.
> The three headers that need a VMA type (`Device.hpp` → `VmaAllocator`, `Buffer.hpp` and
> `Image.hpp` → `VmaAllocation`) **forward-declare the opaque handle** instead:
> ```cpp
> typedef struct VmaAllocator_T * VmaAllocator;   // identical to VMA's own VK_DEFINE_HANDLE
> ```
> Redeclaring an identical typedef is legal C++, so this coexists with the real include in
> the `.cpp`. Only the three TUs that call `vma*` include the real header
> (`Device.cpp`, `Buffer.cpp`, `Image.cpp`).
>
> **`VMA_IMPLEMENTATION` lives in `Device.cpp`** and is compiled in that TU only. Its
> defines (`VK_USE_PLATFORM_WIN32_KHR`, the `VMA_ASSERT` override, `VMA_IMPLEMENTATION`)
> **MUST precede** the `#include "vk_mem_alloc.h"` — do not reorder them, and do not move
> the include after `Device.hpp`.
>
> If a new TU hits `error C2027: use of undefined type 'VmaAllocator_T'`, it is
> dereferencing the handle: add the include **to that `.cpp`**, never back into a header.

### Synchronization
- Rigorous fence and semaphore management
- Avoid deadlocks through strict acquisition order
- Thread-safe `CommandBuffer` with dedicated pools

**Binary semaphores: one signal, one wait, and you must be able to PROVE the wait happened.**
A binary semaphore may not be re-signaled while an operation still waits on it. What differs
between primitives is the *proof* available that the wait completed:

| Waiter | Proof of completion | Therefore index the semaphore by |
|---|---|---|
| `vkQueueSubmit()` | the batch's **fence** | frame in flight |
| `vkQueuePresentKHR()` | **none** — no fence observes a present | **swap-chain image** (its re-acquisition is the only proof) |

That second row is the whole reason `Renderer::m_presentSemaphores` is indexed by the value
`SwapChain::acquireNextImage()` returned and not by the frame index — see
`src/Graphics/AGENTS.md` § 16 Rule 5 and `docs/caution-points.md` § "Present semaphore was
indexed by frame in flight". `VK_KHR_swapchain_maintenance1` (present fence) is the extension
that would supply the missing proof; the engine does not require it, so it relies on
re-acquisition instead.

**⚠️⚠️ `Device::getGraphicsQueue(priority)` is a ROTATING DISPENSER, not "the graphics queue".**
Every queue of the family is created and registered High (`Device::installQueues()`), and
`DeviceQueueConfiguration::queue()` returns `queueList[nextQueueIndex.fetch_add(1) % size]` — 16
different `VkQueue` on NVIDIA, one per call. The renderer caches ONE (`Renderer::m_graphicsQueue`,
`Renderer::graphicsQueue()`). Two queues are ordered by nothing: work that must see a frame goes
INTO the frame's command buffer (the `FrameCapture` and RushMaker hooks) or waits on a semaphore the
frame signals — never a later submit on "a graphics queue". The RushMaker did that until 2026-09-25
and its videos stepped back 3-4 frames whenever the GPU ran behind (`docs/caution-points.md`
§ *RushMaker stepped back 3-4 frames*). The one-shot callers that submit then wait for their own work
(transfers, bakes) are fine. ⚠️ `Queue::waitIdle()` holds the DEVICE-wide mutex for the whole wait,
blocking every submit and present on every queue meanwhile (item
`docs/todo/queue-waitidle-holds-the-device-lock.md`).

**Abandoning a frame is not free.** Once a semaphore has been signaled, something must wait on
it exactly once. `Queue::submit(const SynchInfo &)` is the engine contract for that: a
synchronization-only submission (`commandBufferCount = 0`) that drains pending signals and
optionally signals a fence. Never just `return` out of a frame that already signaled
semaphores.

### Coordinate Convention
- The world is **Y-UP**; Vulkan's NDC is Y-down. The single reconciling flip is the NEGATIVE `[Col1Row1]` of `Matrix::perspectiveProjection()` / `orthographicProjection()`
- No Y conversion anywhere else — not in shaders, not in viewports, not per-asset
- ⚠️ Cubemap targets are the ONE documented exception: they cancel that flip and invert the front face (`GraphicsPipeline::configureRasterizationState(..., mirroredViewport)`), because the cube-face convention is left-handed. See `@src/Saphir/AGENTS.md`

### Compute Shader Support
- `ComputePipeline` — Full compute pipeline with `setShaderModule()` for shader stage init
- `CommandBuffer::dispatch(groupX, groupY, groupZ)` — vkCmdDispatch wrapper
- `Buffer::setHostReadable(true)` — Enables `HOST_CACHED_BIT` for fast GPU→CPU readback
- Use device-local SSBO for GPU writes + host-cached staging buffer + `vkCmdCopyBuffer` for optimal readback
- `Queue::waitIdle()` for synchronous compute completion
- Compute shaders compiled via `Saphir::ShaderManager::getShaderModuleFromSourceCode()`

### Mesh Shader Support (optional, Sep 2026)

`VK_EXT_mesh_shader` is an OPTIONAL capability, like the geometry stage: nothing requires it, a
consumer asks `Device::meshShadersEnabled()` and keeps its classic vertex path otherwise.
- **Detection**: `PhysicalDevice::supportsMeshShaders()`, `meshShaderFeatures()`, `meshShaderProperties()`
  (the device's ceilings: `maxMeshOutputVertices`, `maxMeshOutputPrimitives`, workgroup sizes...), queried
  in a SEPARATE chain only when the extension is advertised (as the portability subset).
- **Enabling** (`Instance.cpp`, beside ray tracing): the extension, `meshShader`, and `taskShader` /
  `multiviewMeshShader` when reported. Left off: the fragment-shading-rate variant, the statistics queries.
  RTX 3070 Ti: task yes, multiview yes, 256 vertices / 256 primitives per workgroup.
- **Drawing**: `CommandBuffer::drawMeshTasks()`, `drawMeshTasksIndirect()`, `drawMeshTasksIndirectCount()`,
  through entry points `Device` loads at creation (null when disabled: the call is refused with an error).
- **Pipelines**: `GraphicsPipeline::finalize()` accepts a pipeline with a mesh stage and no vertex input /
  input assembly state (the spec ignores both).
- ⚠️ **MoltenVK does not expose the extension** (checked 2026-09-22: absent from its supported-extension
  list, the 2024 implementation sits on an unmerged `mesh-shader` branch). Never true on macOS today.
- ⚠️⚠️ **`maintenance4` is REQUESTED for it**: glslang writes a task/mesh workgroup size as
  `OpExecutionMode LocalSizeId` when it targets SPIR-V 1.6, refused without the feature
  (`VUID-RuntimeSpirv-LocalSizeId-06434`) — found by the first compiled mesh shader.

### GPU Profiler (`GPUProfiler.cpp/.hpp`)

Per-pass GPU timing via timestamp queries — the first tool for any "the frame is slow"
question (RenderDoc is for draw-call-level dissection of ONE pass).

- **One `VkQueryPool` per frame in flight** (128 timestamps each = 64 scopes). The results
  of a slot's PREVIOUS submission are harvested right after the slot's in-flight fence
  wait — availability is guaranteed, the read never stalls (no `WAIT` flag).
- Timestamps: classic `vkCmdWriteTimestamp` pair, `TOP_OF_PIPE` at scope begin /
  `BOTTOM_OF_PIPE` at scope end — the only combination giving meaningful approximate
  timings (GPU passes overlap; intermediate stages are not measurable this way).
- The tick→ms conversion uses `limits.timestampPeriod`; the subtraction is masked to the
  graphics queue family's `timestampValidBits` (a mid-frame counter wrap yields the
  correct duration instead of a bogus huge sample).
- **Ownership:** `Renderer::m_GPUProfiler`, created at renderer init when the settings key
  `Core/Graphics/GPUProfiler/Enabled` (default `false`) is on AND the device supports
  timestamps. Null pointer when disabled — every call site null-checks (zero cost path).
  **Torn down explicitly in `Renderer::onTerminate()`** while the device is alive: letting
  the `unique_ptr` member die at Renderer destruction destroys the pools AFTER
  `vkDestroyDevice` (loader error `VUID-vkDestroyQueryPool-device-parameter`).
- **Scope placement:** `openFrame()` right after the fence reset (before the shadow maps), then
  `beginFrame()` right after the main command buffer `begin()` (the V1 GPU-side pool reset,
  OUTSIDE a render pass, when there is no host reset); RAII `GPUProfiler::ScopedZone` around each pass
  (`TLASBuild`, `ScenePass`, `PostFXChain` + one scope per REAL pass inside the chain,
  `FinalComposite`). Effect labels come from `PostProcessEffect::label()` (returns the
  effect's ClassId; override it on every new effect).
- **Interleaving truth:** a shared-denoise effect has NO contiguous per-effect cost — the
  attribution is per pass (`RTGIEffect/trace`, `SharedDenoise`, `RTGIEffect/temporal`,
  `Combine`), mirroring the actual command stream. Do not "fix" this by summing.
- **Side submissions (2026-09-23):** the shadow maps and render-to-textures are separate
  submissions recorded BEFORE the main command buffer, so a pool reset recorded in it would wipe
  their queries on the GPU timeline. With the optional Vulkan 1.2 feature `hostQueryReset`
  (requested when advertised, `Device::hostQueryResetEnabled()`) the pool is reset from the HOST
  in `GPUProfiler::openFrame()`, right after the slot's fence wait, and each side submission gets
  its own top-level scope: `ShadowMap/<target id>` and `RenderToTexture/<target id>` (the
  post-render compute included). Without the feature the profiler keeps the V1 scope, main
  command buffer only, and says so in its "ready" line. Safe on a discarded frame: its empty
  batch still signals the fence, and a fence signal covers every earlier submission of the queue.
- ⚠️ **`ScenePass` does NOT contain the shadow maps**: they are their own submissions. Before
  2026-09-23 a shadow cost was simply invisible in the timings (a peer session once read a
  ScenePass delta as "colour + shadow").
- Console: `Core.RendererService.getGPUTimings([reset])` — see `docs/ai-runtime-control.md` §6.

### Swap-Chain Format Configuration

The swap-chain surface format can be configured via settings:

**Settings keys:** `Video/EnableSRGB` (default: false)

| Format | Key Value | Use Case |
|--------|-----------|----------|
| `VK_FORMAT_B8G8R8A8_UNORM` | false | sRGB content (CEF, web), no automatic conversion |
| `VK_FORMAT_B8G8R8A8_SRGB` | true | Linear content, automatic linear→sRGB conversion |

**Why UNORM for CEF:**
- CEF provides sRGB pixels already
- SRGB format applies gamma correction, causing double correction (washed-out colors)
- UNORM passes pixels through unchanged

**Code references:**
- `SwapChain.cpp:chooseSurfaceFormat()` - Format selection logic
- `SwapChain.hpp:m_sRGBEnabled` - Configuration member

### Present Mode Selection

The swap-chain present mode is selected based on `VSync` and `Triple-Buffering` settings.

**Settings keys:**
- `Core/Video/EnableVSync` (default: true)
- `Core/Video/EnableTripleBuffering` (default: true)

**Available modes and characteristics:**

| Mode | VSync | Blocking | Tearing | Notes |
|------|-------|----------|---------|-------|
| `IMMEDIATE` | No | No | Yes | Lowest latency, may tear |
| `MAILBOX` | Yes | No | No | Triple-buffer, best for games |
| `FIFO` | Yes | Yes | No | Always available, classic vsync |
| `FIFO_RELAXED` | Partial | Partial | If late | Vsync but allows late present |

**Selection matrix:**

| VSync | Triple-Buffer | Priority order |
|-------|---------------|----------------|
| ON | ON | MAILBOX > FIFO_RELAXED > FIFO |
| ON | OFF | FIFO (standard double-buffered vsync) |
| OFF | ON | IMMEDIATE > MAILBOX > FIFO_RELAXED > FIFO |
| OFF | OFF | IMMEDIATE > FIFO_RELAXED > FIFO |

**Platform notes:**
- **Windows**: MAILBOX widely supported on modern GPUs.
- **Linux**: MAILBOX often unavailable (Mesa/NVIDIA). FIFO_RELAXED is a good fallback.
- **macOS**: Limited mode support through MoltenVK, FIFO typically used.

**Linux/NVIDIA/X11 known issue:**
With compositor-based desktops (GNOME, KDE), enabling VSync can cause micro-stuttering due to double sync (driver + compositor). **Recommended solution**: Disable VSync, use Frame Rate Limiter instead, let compositor handle sync.

**Code references:**
- `SwapChain.cpp:choosePresentMode()` - Mode selection logic with full documentation
- `SettingKeys.hpp:VideoEnableVSyncKey`, `VideoEnableTripleBufferingKey`

### Performance: std::span for Barrier APIs

`CommandBuffer` uses `std::span` for synchronization methods:

```cpp
void pipelineBarrier(std::span< const VkImageMemoryBarrier > barriers, ...);
void waitEvents(std::span< const VkEvent > events, ...);
```

**Benefits:**
- Accepts `StaticVector`, `std::vector`, `std::array` without copy
- Zero allocation on caller side with `StaticVector`
- Backward compatible with existing code using `std::vector`

### ⚠⚠ `CommandBuffer::bind()` — a sub-geometry is selected at DRAW time, never at BIND time

Both `bind()` overloads take a `subGeometryIndex` and **both ignore it**. The sub-geometry is
chosen by `draw(geometry, subGeometryIndex, instanceCount)`, which reads
`Geometry::Interface::subGeometryRange(i)` — documented `{firstIndex, indexCount}` — and hands it
to `vkCmdDrawIndexed` as `firstIndex` / `indexCount`. The vertex buffer is therefore **always bound
at offset 0**.

The instanced overload used to bind it at `range[0]`: a first-INDEX used as a vertex-buffer BYTE
offset, on top of the `firstIndex` the draw applied anyway. Measured on an instanced two-layer
procedural tree, offset `405` — neither 4-aligned nor a multiple of the 72-byte stride — and the
driver dropped every draw (`VUID-vkCmdDrawIndexed-None-02721`). Fixed Sep 2026; full account in
`docs/caution-points.md`.

⚠️ If a future overload ever needs a non-zero geometry VBO offset, it is a **byte** offset and it
must be a multiple of the vertex stride. Never derive it from a sub-geometry range.
