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

## Critical Points

- **Ordered destruction**: Destroy resources in reverse creation order
- **Thread safety**: CommandPool per thread, CommandBuffers not shared
- **Memory barriers**: Correct state transitions for images
- **Validation layers**: Always active in development
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
