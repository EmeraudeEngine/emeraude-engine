# ADR-003: Fail-Safe Resource Management

## Status
**Accepted** - Core principle implemented throughout resource system

## Context

In game engines, resource loading (textures, meshes, audio, etc.) can fail for numerous reasons:
- File not found or corrupted
- Network timeout for remote resources
- Out of memory during loading
- Unsupported file format
- Dependency chain failures

**Traditional Approaches:**
1. **Return nullptr/null**: Caller must check for errors and handle gracefully
2. **Throw exceptions**: Propagate errors up the call stack for handling
3. **Error codes**: Return status codes with optional resource data
4. **Logging + crash**: Log error and terminate application

**Problems with Traditional Approaches:**
- **Client code complexity**: Every resource access requires error handling
- **Fragile error paths**: Easy to forget null checks, leading to crashes
- **Development friction**: Game logic obscured by error handling boilerplate
- **Inconsistent handling**: Different developers handle errors differently
- **Production instability**: Missing resources cause application crashes

## Decision

**Emeraude Engine implements a fail-safe resource management system that NEVER crashes the application due to resource failures.**

**Core Principle:**
> "The resource manager's job is to provide a resource, no matter what. The client's job is to use it. That's it."

**Implementation Strategy:**
- **Never return nullptr**: All resource requests return valid, usable resources
- **Neutral resources**: Every resource type has a built-in default/fallback version
- **Transparent fallback**: Resource loading failures automatically return neutral resources
- **Error logging**: Failures are logged for debugging but don't propagate as exceptions
- **Visual identification**: Neutral resources are easily identifiable (e.g., pink checkerboard textures)

## Architecture

**Three-Layer System:**
```
Manager (Central coordinator)
    ↓
Container<ResourceType> (Type-specific caching & loading)  
    ↓
ResourceTrait (Individual resources with lifecycle management)
```

**Resource Loading Scenarios:**
1. **Success**: File found and loaded → return real resource
2. **Not found**: File missing from resource index → return neutral resource + warning log
3. **Load failure**: File exists but loading fails → return neutral resource + error log

**Neutral Resource Requirements:**
- Created without external dependencies (no file I/O, no network)
- Always succeeds during creation (cannot fail)
- Immediately usable (valid GPU resources, valid data)
- Easily identifiable by developers (visual/audio placeholders)

## Consequences

### Positive
- **Zero crashes**: Application never terminates due to missing/broken assets
- **Simple client code**: No need for null checks or error handling in game logic
- **Robust development**: Broken assets don't stop development workflow
- **Clear debugging**: Visual placeholders make missing assets obvious
- **Graceful degradation**: Application continues running with placeholder content
- **Fail-fast feedback**: Developers immediately see when assets are missing

### Negative
- **Hidden failures**: Resource errors might go unnoticed without checking logs
- **Performance overhead**: Always creating neutral resources (minimal impact)
- **Memory usage**: Neutral resources consume memory even when unused
- **Development dependency**: Developers might become complacent about asset quality

### Neutral
- **Design trade-off**: Robustness over strict correctness
- **Production vs development**: Different error tolerance in different contexts

## Implementation Examples

**Neutral Resource Patterns:**
```cpp
// TextureResource - Pink checkerboard (obvious placeholder)
bool TextureResource::load(ServiceProvider& provider) override {
    m_pixels = generateCheckerboard(64, 64, Color::Pink, Color::Black);
    m_width = 64;
    m_height = 64;
    return true;  // Always succeeds
}

// MeshResource - Gray unit cube (functional geometry)
bool MeshResource::load(ServiceProvider& provider) override {
    m_vertices = generateCubeVertices(1.0F);
    m_indices = generateCubeIndices();
    m_material = provider.container<MaterialResource>()->getDefaultResource();
    return true;  // Always succeeds, no dependencies
}

// SoundResource - Brief silence or neutral tone
bool SoundResource::load(ServiceProvider& provider) override {
    m_samples = generateSilence(1.0F);  // 1 second of silence
    m_sampleRate = 44100;
    return true;  // Always succeeds
}
```

**Client Usage (Zero Error Handling):**
```cpp
// Traditional approach (ERROR-PRONE)
auto texture = resources.loadTexture("logo.png");
if (texture == nullptr) {
    Log::error("Failed to load logo!");
    texture = resources.loadTexture("default.png");
    if (texture == nullptr) {
        Log::fatal("No fallback texture!");
        std::exit(1);  // Application crash!
    }
}
material.setTexture(texture);

// Emeraude approach (FAIL-SAFE)
auto texture = resources.get<TextureResource>("logo.png");
material.setTexture(texture);  // Always works, no checks needed
// If "logo.png" missing → pink checkerboard displayed, warning logged
```

## Risk Mitigation

**Neutral Resource Quality:**
- Neutral resources must be functional, not just placeholders
- Must support all operations the real resource would
- Performance characteristics should be reasonable

**Developer Awareness:**
- Clear logging when neutral resources are used
- Visual/audio cues make placeholders obvious
- Development tools to identify missing assets
- Asset validation pipeline to catch missing resources

**Production Considerations:**
- Asset validation before release builds
- Telemetry to track neutral resource usage in production
- Option to make resource failures more strict in development builds

## Related ADRs
- ADR-002: Vulkan-Only Graphics API (graphics resources need fail-safe GPU resources)
- ADR-004: Saphir Shader Generation (shader generation failures fall back to neutral shaders)
- ADR-006: Component Composition Over Inheritance (resources used as dependencies in components)

## References
- `docs/resource-management.md` - Complete resource system architecture
- `src/Resources/AGENTS.md` - Resource system implementation context
- `src/Graphics/AGENTS.md` - Graphics resources fail-safe implementation