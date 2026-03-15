# Vulkan System

Context for developing the Emeraude Engine Vulkan abstraction layer.

## Module Overview

Vulkan abstraction layer that hides API complexity while providing precise control for modern 3D applications. NEVER call Vulkan functions directly from high-level code.

## Vulkan-Specific Rules

### Mandatory Abstraction
- **NEVER** call Vulkan functions directly from `Graphics/` or client code
- Use abstraction classes: `Device`, `Buffer`, `Image`, `Pipeline`, etc.
- All Vulkan resources must be encapsulated

### GPU Memory Management
- **VMA mandatory** for all GPU memory allocations
- Use `MemoryRegion` and `DeviceMemory` for encapsulation
- RAII for automatic Vulkan resource management

### Synchronization
- Rigorous fence and semaphore management
- Avoid deadlocks through strict acquisition order
- Thread-safe `CommandBuffer` with dedicated pools

### Coordinate Convention
- Projection matrices configured for Y-down
- Vulkan Y-inverted viewport handled automatically
- No conversion in shaders

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

## Important Files

- `Device.cpp/.hpp` - Vulkan logical device abstraction
- `Buffer.cpp/.hpp` - Buffer management with VMA
- `Image.cpp/.hpp` - Texture and image management
- `GraphicsPipeline.cpp/.hpp` - Render pipelines
- `CommandBuffer.cpp/.hpp` - Command recording (uses std::span)
- `TransferManager.cpp/.hpp` - CPU-GPU transfers
- `LayoutManager.cpp/.hpp` - Shared descriptor set layout and pipeline layout manager (thread-safe)
- `DescriptorSetLayout.cpp/.hpp` - Descriptor set layout creation and binding declarations

## Critical: LayoutManager Thread Safety

> [!CRITICAL]
> **`LayoutManager` is accessed concurrently by the resource loading thread pool.**
>
> Materials (Basic, Standard, PBR) share descriptor set layouts via `LayoutManager`.
> Multiple materials with the same identifier (e.g., `"MaterialBasicResourceSimple"`) can
> be loaded in parallel by `Container::getOrCreateResource()` which dispatches to a thread pool.
>
> **Thread-safety mechanism:**
> - `LayoutManager` protects all map access with `m_access` mutex
> - `createDescriptorSetLayout()` tolerates duplicate UUIDs (returns `true` silently)
> - Material `createDescriptorSetLayout()` re-fetches the layout from the manager after
>   creation to get the canonical instance (another thread may have won the race)
>
> **Bug pattern (fixed Mar 2026):**
> ```cpp
> // BROKEN - TOCTOU race: two threads see nullptr, both try to register
> m_layout = layoutManager.getDescriptorSetLayout(id);  // nullptr
> if (!m_layout) {
>     m_layout = prepare + declare + create;  // second thread fails!
> }
>
> // CORRECT - create with local, re-fetch canonical instance
> auto newLayout = layoutManager.prepareNewDescriptorSetLayout(id);
> // ... declare bindings ...
> layoutManager.createDescriptorSetLayout(newLayout);  // tolerates duplicates
> m_layout = layoutManager.getDescriptorSetLayout(id); // get canonical
> ```
>
> **Code references:**
> - `LayoutManager.hpp:m_access` - Mutex protecting both maps
> - `LayoutManager.cpp:createDescriptorSetLayout()` - Duplicate-tolerant registration
> - `Material/BasicResource.cpp:createDescriptorSetLayout()` - Re-fetch pattern
> - `Material/StandardResource.cpp:createDescriptorSetLayout()` - Same pattern
> - `Material/PBRResource.cpp:createDescriptorSetLayout()` - Same pattern

## Critical: Buffer Descriptor Offset

> [!CRITICAL]
> **`Buffer::getDescriptorInfo(offset, range)` MUST use the offset parameter!**
>
> This function returns `VkDescriptorBufferInfo` for descriptor set binding.
> The `offset` parameter specifies where to start reading in the buffer.
>
> **Bug pattern (fixed Jan 2026):**
> ```cpp
> // BROKEN - ignores offset, all descriptors point to offset 0
> getDescriptorInfo (uint32_t /*offset*/, uint32_t range) {
>     descriptorInfo.offset = 0;  // WRONG!
> }
>
> // CORRECT - uses the provided offset
> getDescriptorInfo (uint32_t offset, uint32_t range) {
>     descriptorInfo.offset = static_cast<VkDeviceSize>(offset);
> }
> ```
>
> **Impact:** SharedUniformBuffer stores multiple materials. If offset is ignored,
> ALL materials read from offset 0, causing Material B to display Material A's properties.

### UniformBufferObject Element Index Conversion

`UniformBufferObject::getDescriptorInfo(elementOffset)` receives an **element index** (0, 1, 2...)
but must pass a **byte offset** to `Buffer::getDescriptorInfo()`:

```cpp
// In UniformBufferObject.cpp
VkDescriptorBufferInfo
UniformBufferObject::getDescriptorInfo (uint32_t elementOffset) const noexcept
{
    // Convert element index to byte offset
    return this->getDescriptorInfo(elementOffset * m_blockAlignedSize, m_blockAlignedSize);
}
```

**Files involved:**
- `Buffer.hpp:getDescriptorInfo()` - Must use offset parameter
- `UniformBufferObject.cpp:getDescriptorInfo()` - Must multiply by block size

## Development Patterns

### Creating a New GPU Resource
1. Inherit from `AbstractDeviceDependentObject`
2. Implement RAII with appropriate destructor
3. Use VMA for memory allocations
4. Add necessary synchronizations

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
> See [`docs/pipeline-caching-system.md`](../../docs/pipeline-caching-system.md) for complete caching architecture.

### Data Transfers
1. Use `TransferManager` for async transfers
2. Automatic staging buffers for large transfers
3. Fence synchronization for coherence
4. Batching of small transfers

## Queue Family Ownership Transfer (BLAS Building)

When a buffer is created with `VK_SHARING_MODE_EXCLUSIVE` and needs to be accessed by a different queue family (e.g., transfer queue → graphics queue for acceleration structure building), a **two-sided ownership transfer** is required:

1. **Release barrier** (source queue): After the transfer operation completes
2. **Acquire barrier** (destination queue): Before the new queue accesses the buffer

```cpp
// Release side (in BufferTransferOperation, transfer queue)
VkBufferMemoryBarrier barrier{};
barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
barrier.dstAccessMask = 0;
barrier.srcQueueFamilyIndex = transferFamilyIndex;
barrier.dstQueueFamilyIndex = graphicsFamilyIndex;

// Acquire side (in AccelerationStructureBuilder, graphics queue)
VkBufferMemoryBarrier barrier{};
barrier.srcAccessMask = 0;
barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
barrier.srcQueueFamilyIndex = transferFamilyIndex;
barrier.dstQueueFamilyIndex = graphicsFamilyIndex;
```

**Code references:**
- `BufferTransferOperation.cpp` — Release barrier after buffer copy
- `AccelerationStructureBuilder.cpp:buildBLAS()` — Acquire barrier before AS build
- `AccelerationStructureBuilder.hpp` — `m_graphicsFamilyIndex`, `m_transferFamilyIndex` members

## TLAS Async Build (Inline Command Buffer Recording)

> [!CRITICAL]
> **TLAS builds MUST be recorded inline into the render command buffer, NOT submitted synchronously.**
>
> The old `buildTLAS()` method used a dedicated command buffer with a fence wait per frame,
> causing massive scheduler overhead (`sched_yield` dominated profiles). The new async API
> splits TLAS building into CPU-side preparation and GPU-side command recording.
>
> **API:**
> - `prepareTLAS(instances, instanceCount)` — CPU-side: creates/resizes buffers, uploads instance data. Returns `TLASBuildRequest`.
> - `recordTLASBuild(commandBuffer, request)` — GPU-side: records `vkCmdBuildAccelerationStructuresKHR` + barrier into an external command buffer.
>
> **TLASBuildRequest** owns the TLAS + instance buffer + scratch buffer for the current build.
> After recording, the request is moved into a retirement deque for frames-in-flight safety.
>
> **Buffer lifetime:** TLAS buffers are per-request (not persistent). Each build creates fresh
> buffers. Retired requests are kept in a `std::deque` and popped from the front when the
> deque exceeds `framesInFlight()` entries.
>
> **Pipeline barrier:** The barrier after TLAS build uses `FRAGMENT_SHADER_BIT | COMPUTE_SHADER_BIT`
> as destination stage (NOT `RAY_TRACING_SHADER_BIT_KHR`). The engine uses **ray queries**
> (`GL_EXT_ray_query`) in fragment/compute shaders, not RT pipelines. Using
> `VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR` requires `VK_KHR_ray_tracing_pipeline` which
> is not enabled.
>
> **Measured impact:** `sched_yield` -79%, scheduler overhead -88% (perf profiling).
>
> **Code references:**
> - `AccelerationStructureBuilder.hpp` — `TLASBuildRequest` struct, `prepareTLAS()`, `recordTLASBuild()`
> - `AccelerationStructureBuilder.cpp` — Implementation with retired request deque
> - `Scenes/SceneMetaData.cpp:recordTLASBuild()` — Delegates to builder
> - `Graphics/Renderer.cpp:renderFrameWithInternal/renderFrameDirect` — Calls `scene->recordTLASBuild()` after `prepareRender()`, before `beginRenderPass()`

## Critical: Ray Query vs RT Pipeline Stage Flags

> [!CRITICAL]
> **The engine uses `GL_EXT_ray_query` (ray queries in fragment/compute shaders), NOT `VK_KHR_ray_tracing_pipeline`.**
>
> This means:
> - TLAS access happens in **fragment shaders** and **compute shaders**
> - Pipeline barriers must use `VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT`
> - **NEVER use** `VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR` — it requires enabling the RT pipeline extension
> - Access mask: `VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR` is correct for both approaches

## Multi-Draw Indirect Support

The command buffer supports `drawIndexedIndirect()` for GPU-driven rendering. Device features enabled in `Instance.cpp`:
- `multiDrawIndirect`, `drawIndirectFirstInstance` (VK 1.0)
- `shaderInt64` (VK 1.0) — `uint64_t` for BDA address reconstruction
- `shaderDrawParameters` (VK 1.1) — `gl_DrawID` in vertex shaders

**Buffer types for MDI:**
- `IndirectBuffer` (`IndirectBuffer.hpp`) — `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`, host-visible
- Per-draw SSBO — Created via `Buffer` directly with `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT` (NOT via `ShaderStorageBufferObject` which lacks the BDA flag)

> [!WARNING]
> **`ShaderStorageBufferObject` does NOT include `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`.**
> For BDA-accessible SSBOs, use `Buffer` directly with both `STORAGE_BUFFER_BIT` and `SHADER_DEVICE_ADDRESS_BIT`.

**Code references:**
- `CommandBuffer.cpp:drawIndexedIndirect()` — Wraps `vkCmdDrawIndexedIndirect`
- `IndirectBuffer.hpp` — Convenience buffer subclass
- `Instance.cpp` — Feature enablement (MDI + shaderInt64 + shaderDrawParameters)

## Critical Points

- **Ordered destruction**: Destroy resources in reverse creation order
- **Thread safety**: CommandPool per thread, CommandBuffers not shared
- **Memory barriers**: Correct state transitions for images
- **Queue family ownership**: Two-sided barriers for exclusive-mode cross-queue access
- **TLAS barriers**: Use `FRAGMENT_SHADER_BIT | COMPUTE_SHADER_BIT`, NOT `RAY_TRACING_SHADER_BIT_KHR` (ray queries, not RT pipelines)
- **Validation layers**: Always active in development (note: ~6% CPU overhead, ~41% when combined with rwlock)
- **Never direct calls**: Graphics, Resources, Saphir use Vulkan abstractions
- **VMA mandatory**: All GPU allocation via VMA, never direct vkAllocateMemory
- **Y-down setup**: Viewport and projection configured for engine Y-down

## Detailed Documentation

For Vulkan platform:
- Official Vulkan documentation - Complete API specifications

Related systems:
- @docs/coordinate-system.md - Y-down configuration for Vulkan
- @src/Graphics/AGENTS.md - Uses Vulkan abstractions (Buffer, Image, Pipeline)
- @src/Saphir/AGENTS.md - Generates SPIR-V for Vulkan pipelines
- @src/Resources/AGENTS.md - GPU upload via TransferManager
