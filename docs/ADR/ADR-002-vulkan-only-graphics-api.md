# ADR-002: Vulkan-Only Graphics API

## Status
**Accepted** - Vulkan 1.2+ is the exclusive graphics API

## Context

Modern game engines typically support multiple graphics APIs to maximize platform compatibility:
- **DirectX 11/12**: Windows-specific, well-established
- **OpenGL/OpenGL ES**: Cross-platform, mature, easier to use
- **Metal**: Apple-specific, high performance on iOS/macOS
- **Vulkan**: Cross-platform, explicit control, maximum performance
- **WebGPU**: Emerging standard for web platforms

**Trade-offs of Multi-API Support:**
- **Pros**: Broader platform support, fallback options, developer familiarity
- **Cons**: Massive maintenance burden, lowest-common-denominator features, abstraction complexity

**Vulkan Characteristics:**
- **Explicit control**: Manual memory management, synchronization, pipeline state
- **Performance**: Minimal driver overhead, multi-threaded command building
- **Complexity**: Verbose API requiring extensive boilerplate
- **Platform support**: Windows, Linux, macOS (via MoltenVK), Android

## Decision

**Emeraude Engine uses Vulkan 1.2+ as the exclusive graphics API.**

**Implementation Strategy:**
- Build high-level abstractions on top of Vulkan
- Provide OpenGL-style declarative interface for ease of use
- Hide Vulkan complexity behind Graphics system abstractions
- Use Vulkan Memory Allocator (VMA) for GPU memory management
- Target Vulkan 1.2+ for modern feature set

**Platform Coverage:**
- **Windows 11**: Native Vulkan 1.2+ support
- **Linux**: Native Vulkan support (driver dependent)
- **macOS SDK 12.0+**: Vulkan via MoltenVK translation layer

## Consequences

### Positive
- **Maximum performance**: Direct access to modern GPU capabilities
- **Explicit control**: Fine-grained management of GPU resources and synchronization
- **Future-proof**: Vulkan is the modern standard with active development
- **Multi-threading**: Efficient command buffer building across threads
- **No abstraction penalty**: Single API means zero overhead from multi-API abstraction
- **Modern features**: Access to latest GPU features (compute, ray tracing, mesh shaders)
- **Simplified codebase**: No need to maintain multiple rendering backends

### Negative
- **Development complexity**: Vulkan requires significant expertise and boilerplate
- **Limited platform support**: Older systems or exotic platforms may lack Vulkan drivers
- **macOS dependency**: Relies on MoltenVK translation layer (potential compatibility issues)
- **Learning curve**: Steeper learning curve for developers unfamiliar with Vulkan
- **Debug complexity**: Vulkan debugging is more complex than OpenGL

### Neutral
- **Industry adoption**: Many modern engines are moving to Vulkan-first approaches
- **Driver quality**: Vulkan driver quality varies by vendor and platform

## Implementation Strategy

**Abstraction Layers:**
```
High Level:     Graphics System (Geometry, Material, Renderable)
                       ↓
Medium Level:   Vulkan Abstractions (Device, Buffer, Pipeline)
                       ↓  
Low Level:      Raw Vulkan API (vk* functions)
```

**Key Principles:**
- **Never expose Vulkan directly**: User code only sees Graphics abstractions
- **Hide complexity**: OpenGL-style declarative interface
- **Manage resources**: Automatic lifetime management with RAII
- **Thread safety**: Safe multi-threaded command building
- **Performance**: Zero unnecessary overhead in critical paths

**Code Example:**
```cpp
// User code (high level)
auto geometry = resources.get<GeometryResource>("cube");
auto material = resources.get<MaterialResource>("wood");
auto renderable = Renderable::create(geometry, material);
node->newVisual(renderable);  // Vulkan complexity hidden

// vs Raw Vulkan (what user doesn't see)
// 500+ lines of buffer creation, pipeline setup, descriptor management...
```

## Risk Mitigation

**Platform Compatibility:**
- **macOS**: Test thoroughly with MoltenVK, maintain compatibility matrix
- **Driver support**: Document minimum driver versions, provide fallback messages
- **Hardware support**: Define minimum Vulkan feature requirements

**Development Complexity:**
- **High-level abstractions**: Hide Vulkan details behind Graphics system
- **Documentation**: Comprehensive guides for Graphics system usage
- **Examples**: Clear patterns for common rendering tasks
- **Debugging tools**: Integration with Vulkan validation layers

## Related ADRs
- ADR-001: Y-Down Coordinate System (aligns with Vulkan's Y-down viewport)
- ADR-003: Fail-Safe Resource Management (provides fallback for rendering failures)
- ADR-004: Saphir Shader Generation (auto-generates Vulkan-compatible GLSL)

## References
- `src/Vulkan/AGENTS.md` - Vulkan abstraction layer documentation
- `src/Graphics/AGENTS.md` - High-level graphics system built on Vulkan
- `README.md` - Platform requirements and Vulkan version specifications