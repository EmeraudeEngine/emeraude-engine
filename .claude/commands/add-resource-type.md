---
description: Génère un template pour nouveau type de resource avec fail-safe
---

Generate a complete template for a new ResourceTrait-based resource type following Emeraude Engine conventions.

**Task:**
1. Ask user for resource type name (e.g., "MyResource")
2. Generate header (.hpp) and implementation (.cpp) files with:
   - Proper includes (ResourceTrait, ServiceProvider)
   - Class inheriting from ResourceTrait
   - **MANDATORY neutral resource:** `load(ServiceProvider&)`
   - File/data loading: `load(ServiceProvider&, const std::filesystem::path&)`
   - JSON loading: `load(ServiceProvider&, const Json::Value&)`
   - Dependency finalization: `onDependenciesLoaded()`
   - Memory tracking: `memoryOccupied()` override

**Template structure:**

```cpp
// Header: MyResource.hpp
#pragma once
#include "Resources/ResourceTrait.hpp"

namespace Emeraude::Resources {

class MyResource : public ResourceTrait {
public:
    // Neutral resource (MANDATORY - must always succeed)
    bool load(ServiceProvider& provider) override;

    // Load from file
    bool load(ServiceProvider& provider, const std::filesystem::path& path) override;

    // Load from JSON data
    bool load(ServiceProvider& provider, const Json::Value& data) override;

    // Finalize after dependencies loaded
    bool onDependenciesLoaded() override;

    // Memory tracking
    size_t memoryOccupied() const override;

private:
    // Resource-specific data
};

} // namespace Emeraude::Resources
```

**Include detailed comments explaining:**
- Neutral resource must be procedural/no I/O
- Fail-safe philosophy (never nullptr)
- When to use addDependency()
- GPU upload in onDependenciesLoaded()

**Also generate:**
- Registration code snippet for Manager
- Example usage in client code
- Test file template (Testing/)

Ask clarifying questions about resource specifics (GPU resource? has dependencies? etc.)
