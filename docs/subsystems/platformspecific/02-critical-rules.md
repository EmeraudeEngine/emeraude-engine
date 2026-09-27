## Critical Rules

### STRICT Isolation Philosophy
- **Separate OS code**: Each OS in its own implementation file
- **NEVER contaminate**: Windows code must NEVER touch Linux/macOS and vice-versa
- **Maximum abstraction**: Common interface in `.hpp`, OS-specific implementations in separate files
- **Strict isolation**: OS-specific code is NOT found in common headers (except `#if` for includes/types)

### NO Platform Macros in Implementation Files

**CRITICAL**: Platform-specific `.cpp` and `.mm` files must **NOT** contain `#if IS_LINUX`, `#if IS_WINDOWS`, or `#if IS_MACOS` guards around their entire content.

**Why**: CMake conditionally includes files based on the target platform. Wrapping code in platform macros is redundant and masks errors - if a file accidentally ends up in the wrong build, it should fail to compile immediately rather than being silently ignored.

**Correct Pattern**:
```cpp
// OpenFile.linux.cpp
#include "OpenFile.hpp"

/* STL inclusions. */
#include <filesystem>

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
    bool OpenFile::execute(Window* window) noexcept
    {
        // Linux implementation directly - NO #if IS_LINUX wrapper
    }
}
```

**Exception**: Platform macros ARE allowed in **header files** for conditional includes and type definitions:
```cpp
// Helpers.hpp - OK to use macros for conditional includes
#if IS_WINDOWS
    #include <Windows.h>
#endif

#if IS_LINUX
    using ExtensionFilters = std::vector<std::pair<std::string, std::vector<std::string>>>;
#endif
```

---
