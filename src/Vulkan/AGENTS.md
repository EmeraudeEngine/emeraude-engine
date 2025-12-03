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
