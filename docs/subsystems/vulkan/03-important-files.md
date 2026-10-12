## Important Files

- `Device.cpp/.hpp` - Vulkan logical device abstraction; **owns the `VkPipelineCache` objects** — the live one and the restored one, one of them published (`pipelineCache()`, `createPipelineCache()`, `restorePipelineCache()`, `getPipelineCacheData()` — see [`18-vkpipelinecache-…`](18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md))
- `Buffer.cpp/.hpp` - Buffer management with VMA
- `Image.cpp/.hpp` - Texture and image management
- `GraphicsPipeline.cpp/.hpp` - Render pipelines
- `CommandBuffer.cpp/.hpp` - Command recording (uses std::span)
- `TransferManager.cpp/.hpp` - CPU-GPU transfers
- `LayoutManager.cpp/.hpp` - Shared descriptor set layout and pipeline layout manager (thread-safe)
- `GPUProfiler.cpp/.hpp` - Per-pass GPU timing (timestamp queries, one pool per frame in flight)
- `DescriptorSetLayout.cpp/.hpp` - Descriptor set layout creation and binding declarations
