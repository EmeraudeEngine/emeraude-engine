# ADR-010: Vulkan Abstraction Layer

## Status
**Accepted** - Mandatory abstraction preventing direct Vulkan API usage

## Context

Vulkan is a low-level, explicit graphics API that provides maximum performance and control but at the cost of significant complexity:

**Raw Vulkan Characteristics:**
- **Verbose**: 500+ lines of code to render a single triangle
- **Error-prone**: Manual memory management, synchronization, validation
- **Platform-specific**: Different implementations across vendors/platforms
- **Steep learning curve**: Requires deep graphics programming expertise
- **Manual resource management**: Explicit allocation, deallocation, lifetime tracking

**Example Raw Vulkan Code:**
```cpp
// Create a simple buffer (simplified excerpt)
VkBuffer buffer;
VkDeviceMemory memory;
VkBufferCreateInfo bufferInfo = {};
bufferInfo.size = dataSize;
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

VkMemoryRequirements memReq;
vkGetBufferMemoryRequirements(device, buffer, &memReq);
VkMemoryAllocateInfo allocInfo = {};
allocInfo.allocationSize = memReq.size;
allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
vkAllocateMemory(device, &allocInfo, nullptr, &memory);
vkBindBufferMemory(device, buffer, memory, 0);
// ... plus error checking, synchronization, cleanup ...
```

**Problems with Direct Vulkan Usage:**
- **Development velocity**: Extremely slow to implement basic features
- **Bug susceptibility**: Easy to forget synchronization, leak resources, misconfigure
- **Knowledge barrier**: Requires specialized Vulkan expertise from all developers
- **Maintenance burden**: Platform-specific quirks, driver differences
- **Code duplication**: Same Vulkan patterns repeated throughout codebase

## Decision

**Emeraude Engine implements a mandatory Vulkan abstraction layer that completely hides Vulkan complexity behind RAII-based C++ classes.**

**Core Principle:**
> "Never expose Vulkan API directly to client code. All Vulkan access goes through abstraction layer."

**Abstraction Strategy:**
- **High-level classes**: Device, Buffer, Image, Pipeline, CommandBuffer
- **RAII resource management**: Automatic cleanup via destructors
- **VMA integration**: Vulkan Memory Allocator for all GPU memory management
- **Thread-safe design**: Safe command buffer building across threads
- **Error handling**: Vulkan errors converted to exceptions/logs
- **Platform abstraction**: Hide platform-specific Vulkan differences

## Architecture Layers

**Layer Separation:**
```
Graphics System (user code)
    ↓ NEVER calls Vulkan directly
Vulkan Abstraction (Device, Buffer, Image, Pipeline)
    ↓ ONLY layer that calls Vulkan
Raw Vulkan API (vk* functions)
```

**Key Abstraction Classes:**

**Device (Logical Device Management):**
```cpp
class Device {
public:
    // High-level buffer creation
    std::unique_ptr<Buffer> createBuffer(size_t size, BufferUsage usage);
    
    // High-level image creation
    std::unique_ptr<Image> createImage(uint32_t width, uint32_t height, ImageFormat format);
    
    // Pipeline creation from SPIR-V
    std::unique_ptr<GraphicsPipeline> createGraphicsPipeline(const PipelineCreateInfo& info);
    
private:
    VkDevice m_device;          // Raw Vulkan device
    VmaAllocator m_allocator;   // VMA allocator
    // Hide all Vulkan details
};
```

**Buffer (GPU Buffer Management):**
```cpp
class Buffer {
public:
    // Upload data to GPU buffer
    void upload(const void* data, size_t size, size_t offset = 0);
    
    // Map buffer memory for CPU access
    void* map();
    void unmap();
    
    // RAII cleanup
    ~Buffer();  // Automatically destroys VkBuffer + VmaAllocation
    
private:
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    // Client never sees these
};
```

**Image (GPU Texture Management):**
```cpp
class Image {
public:
    // Upload texture data
    void upload(const void* pixels, size_t size);
    
    // Create image view for shaders
    VkImageView getImageView() const;
    
    // RAII cleanup
    ~Image();  // Automatically destroys VkImage + VmaAllocation
    
private:
    VkImage m_image;
    VkImageView m_imageView;
    VmaAllocation m_allocation;
};
```

## Implementation Strategy

**RAII Resource Management:**
```cpp
// Client code (Graphics layer)
auto buffer = device.createBuffer(1024, BufferUsage::Vertex);
buffer->upload(vertexData.data(), vertexData.size());
// Buffer automatically cleaned up when unique_ptr goes out of scope

// vs Raw Vulkan (what happens inside abstraction)
VkBuffer rawBuffer;
VmaAllocation allocation;
// 50+ lines of creation code...
// Must manually call vkDestroyBuffer + vmaFreeMemory
```

**VMA Integration:**
```cpp
// All GPU memory goes through VMA
class Device {
    std::unique_ptr<Buffer> createBuffer(size_t size, BufferUsage usage) {
        VkBufferCreateInfo bufferInfo = makeBufferCreateInfo(size, usage);
        VmaAllocationCreateInfo allocInfo = makeAllocationInfo(usage);
        
        VkBuffer buffer;
        VmaAllocation allocation;
        vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
        
        return std::make_unique<Buffer>(buffer, allocation, size);
    }
};
```

**Thread-Safe Command Building:**
```cpp
class CommandBuffer {
    // Thread-safe recording
    void begin();
    void bindPipeline(const GraphicsPipeline& pipeline);
    void bindVertexBuffer(const Buffer& buffer);
    void draw(uint32_t vertexCount);
    void end();
    
private:
    VkCommandBuffer m_commandBuffer;
    VkCommandPool m_commandPool;  // Per-thread pool
};
```

**Error Handling:**
```cpp
// Vulkan errors converted to exceptions
class VulkanException : public std::exception {
    VkResult m_result;
    std::string m_message;
};

void Device::checkResult(VkResult result, const std::string& operation) {
    if (result != VK_SUCCESS) {
        throw VulkanException(result, operation);
    }
}
```

## Consequences

### Positive
- **Developer productivity**: Graphics code focuses on rendering logic, not Vulkan mechanics
- **Reduced bugs**: RAII prevents resource leaks, abstraction prevents common errors
- **Maintainability**: Vulkan-specific code isolated in one layer
- **Platform consistency**: Hide driver differences and platform quirks
- **Learning curve**: Developers don't need Vulkan expertise to use graphics system
- **Code clarity**: High-level intent rather than low-level implementation details

### Negative
- **Performance overhead**: Small cost from abstraction layer (minimal)
- **Flexibility reduction**: Some advanced Vulkan features may not be exposed
- **Debugging complexity**: Harder to debug Vulkan validation errors through abstraction
- **Development cost**: Significant effort to build and maintain abstraction layer

### Neutral
- **Industry standard**: Most modern engines use similar abstraction approaches
- **Abstraction vs control**: Trade fine-grained control for development velocity

## Abstraction Principles

**What Gets Abstracted:**
- **Resource creation**: Buffers, images, pipelines, command buffers
- **Memory management**: All GPU memory allocation via VMA
- **Synchronization**: Fences, semaphores managed internally
- **Error handling**: Vulkan errors converted to exceptions/logs
- **Platform differences**: Hide vendor/driver specific behaviors

**What Remains Exposed:**
- **Performance characteristics**: Client can reason about performance
- **Resource types**: Different buffer/image types still available
- **Rendering concepts**: Still think in terms of pipelines, draw calls
- **Memory types**: Different memory types for different use cases

**Enforcement Mechanisms:**
```cpp
// Graphics layer cannot include Vulkan headers
// #include <vulkan/vulkan.h>  // FORBIDDEN in Graphics/

// All Vulkan types hidden behind abstractions
void Graphics::render(const Mesh& mesh) {
    auto& buffer = mesh.getVertexBuffer();  // Returns Buffer&, not VkBuffer
    commandBuffer.bindVertexBuffer(buffer);  // Abstraction method
    commandBuffer.draw(mesh.getVertexCount());
}
```

## Integration Points

**Graphics System Integration:**
```cpp
// Graphics creates resources via abstraction
class Renderable {
    void createGPUResources(Device& device) {
        m_vertexBuffer = device.createBuffer(vertices.size(), BufferUsage::Vertex);
        m_indexBuffer = device.createBuffer(indices.size(), BufferUsage::Index);
        m_vertexBuffer->upload(vertices.data(), vertices.size());
        m_indexBuffer->upload(indices.data(), indices.size());
    }
};
```

**Saphir Integration:**
```cpp
// Saphir generates GLSL, Vulkan layer compiles to SPIR-V
class ShaderManager {
    std::unique_ptr<GraphicsPipeline> createPipeline(const std::string& glslSource) {
        auto spirv = compileGLSL(glslSource);  // GLSLang compilation
        PipelineCreateInfo info;
        info.vertexShader = spirv.vertex;
        info.fragmentShader = spirv.fragment;
        return device.createGraphicsPipeline(info);  // Vulkan abstraction
    }
};
```

**Resource System Integration:**
```cpp
// Resources use abstraction for GPU uploads
class TextureResource {
    bool onDependenciesLoaded() override {
        auto& device = serviceProvider.vulkanDevice();
        m_image = device.createImage(m_width, m_height, ImageFormat::RGBA8);
        m_image->upload(m_pixels.data(), m_pixels.size());
        return true;
    }
};
```

## Testing Strategy

**Unit Testing:**
- Abstract classes can be mocked for testing
- Graphics logic tested without actual GPU
- Vulkan layer tested separately with validation layers

**Integration Testing:**
- Full pipeline testing with real Vulkan
- Cross-platform validation (Windows, Linux, macOS)
- Driver compatibility testing

**Performance Testing:**
- Measure abstraction overhead (should be <5%)
- Compare against raw Vulkan in synthetic benchmarks
- Profile real-world rendering workloads

## Related ADRs
- ADR-002: Vulkan-Only Graphics API (this abstraction makes Vulkan usable)
- ADR-005: Graphics Instancing System (built on this Vulkan abstraction)
- ADR-004: Saphir Shader Generation (uses this abstraction for pipeline creation)

## References
- `src/Vulkan/AGENTS.md` - Vulkan abstraction implementation context
- `src/Graphics/AGENTS.md` - Graphics system usage of Vulkan abstractions
- Vulkan Memory Allocator documentation - Memory management strategy