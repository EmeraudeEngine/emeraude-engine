# Net System

Context for developing the Emeraude Engine network download system.

## Module Overview

Network resource download system based on ASIO. Seamless integration with the Resources system for loading assets from URLs.

## Net-Specific Rules

### Main Objective
- **Resource download**: Download assets/files from URLs
- **NOT multiplayer**: Net is not for gameplay networking
- **Resources integration**: Seamless workflow with loading system

### ASIO Architecture
- **ASIO-based**: Boost.Asio or standalone for network management
- **Supported protocols**: HTTP/HTTPS and other ASIO protocols
- **Asynchronous**: Non-blocking downloads
- **Internal management**: ASIO handles timeouts, retries, network errors

### Resources Integration

**Automatic workflow**:
```
1. Resource load() detects a URL instead of a file path
2. Resources delegates to Net for download
3. Net downloads asynchronously
4. Net returns downloaded file to Resources
5. Resources finalizes loading normally
```

**Transparent for client**:
```cpp
// Client code identical, whether local file or URL
auto texture = resources.container<TextureResource>()->getResource("logo.png");
// or
auto texture = resources.container<TextureResource>()->getResource("https://example.com/logo.png");

// Net automatically handles download if URL detected
```

### Local Cache System
- **Automatic cache**: Downloaded resources saved locally
- **Avoids re-downloads**: Cache check before download
- **Transparent management**: Cache automatically managed by Net

### Asynchronous Downloads
- **Non-blocking**: No freeze during downloads
- **Async Resources integration**: Compatible with Resources async loading
- **Status tracking**: Resources can track progress via observables

## Development Commands

```bash
# Net tests
ctest -R Net
./test --filter="*Net*"
```

## Important Files

- `Manager.cpp/.hpp` - Main manager, download requests
- Local cache (location to be documented)
- `@docs/resource-management.md` - Resources integration

## Development Patterns

### Usage via Resources (automatic)
```cpp
// Resources detects URL and uses Net automatically
auto mesh = resources.container<MeshResource>()->getResource(
    "https://cdn.example.com/assets/character.obj"
);

// Mesh starts Loading (download in progress)
// When download complete → parsing → Loaded
// If download fails → Default mesh (fail-safe)
```

### Explicit Request (rare, advanced usage)
```cpp
// If direct download control needed
netManager.requestDownload(
    "https://example.com/file.dat",
    "/local/cache/path",
    [](bool success, const std::string& localPath) {
        if (success) {
            // File available at localPath
        } else {
            // Download failed
        }
    }
);
```

### Cache Management
```cpp
// Check if resource is cached
bool cached = netManager.isCached("https://example.com/texture.png");

// Clear cache (maintenance)
netManager.clearCache();

// Force re-download (ignore cache)
netManager.forceDownload(url, callback);
```

## Critical Points

- **ASIO handles complexity**: Timeouts, retries, network errors handled by ASIO
- **Thread safety**: ASIO handles threading, Net thread-safe by design
- **Local cache**: Check available disk space for cache
- **URLs in stores**: Resources stores can contain URLs instead of paths
- **Fail-safe integration**: Download failure → Resources returns neutral resource
- **No multiplayer**: Net is for assets, not gameplay networking

## Detailed Documentation

Related systems:
- @docs/resource-management.md - Automatic integration with Resources
- @src/Resources/AGENTS.md - Fail-safe loading system
- ASIO documentation - Protocol details and network management
