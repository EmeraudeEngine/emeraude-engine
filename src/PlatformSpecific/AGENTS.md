# PlatformSpecific System

Context for developing Emeraude Engine platform-specific code.

## Module Overview

Isolation of OS-specific code (Windows, Linux, macOS) with maximum abstractions to provide uniform cross-platform APIs.

## PlatformSpecific-Specific Rules

### STRICT Isolation Philosophy
- **Separate OS code**: Each OS in its own space
- **NEVER contaminate**: Windows code must NEVER touch Linux/macOS and vice-versa
- **Maximum abstraction**: Common interface, OS-specific implementations
- **Strict isolation**: OS-specific code is NOT found in common space

### Abstracted Features

**System calls**:
- Execution of OS-specific system commands
- Process and environment management

**Dialog boxes**:
- Message dialog boxes (info, warning, error)
- File open dialog (file selection to open)
- File save dialog (save location selection)

**System application**:
- Taskbar notifications (flash taskbar)
- Window focus and attention
- Other window/application specific interactions

### Code Organization

**Mixed approach (in preference order)**:

1. **constexpr if (C++17+)** - Preferred when possible
```cpp
if constexpr (std::is_same_v<Platform, Windows>) {
    // Windows code
} else if constexpr (std::is_same_v<Platform, Linux>) {
    // Linux code
} else if constexpr (std::is_same_v<Platform, macOS>) {
    // macOS code
}
```

2. **#ifdef** - If constexpr not applicable
```cpp
#ifdef _WIN32
    // Windows code
#elif __linux__
    // Linux code
#elif __APPLE__
    // macOS code
#endif
```

3. **Separate .cpp files** - For large code
```
PlatformSpecific/
├── DialogBox.hpp          // Common interface
├── DialogBox_Windows.cpp  // Windows implementation
├── DialogBox_Linux.cpp    // Linux implementation
└── DialogBox_macOS.cpp    // macOS implementation
```
CMake selects the right file based on target platform.

### Fallback Management
- **No fixed rule**: Decision case by case
- **General warning**: If no implementation possible → console warning
- **Graceful degradation**: Feature disabled rather than crash
- **Documentation**: Indicate OS limitations if applicable

## Development Commands

```bash
# Platform-specific tests
ctest -R PlatformSpecific
./test --filter="*Platform*"
```

## Important Files

- `DialogBox.*` - OS dialog box abstractions
- `SystemCall.*` - System command execution
- `WindowManager.*` - Window and notification management
- CMakeLists.txt - Platform file selection

## Development Patterns

### Adding a New Platform-Specific Feature

**1. Define the common abstract interface**
```cpp
// MyFeature.hpp
class MyFeature {
public:
    static bool doSomething(const std::string& param);
};
```

**2. Implement for each OS**

**Option A: constexpr if in single .cpp**
```cpp
// MyFeature.cpp
bool MyFeature::doSomething(const std::string& param) {
    if constexpr (Platform::isWindows()) {
        // Windows implementation
        return windowsDoSomething(param);
    } else if constexpr (Platform::isLinux()) {
        // Linux implementation
        return linuxDoSomething(param);
    } else if constexpr (Platform::isMacOS()) {
        // macOS implementation
        return macosDoSomething(param);
    }
}
```

**Option B: Separate files + CMake**
```cpp
// MyFeature_Windows.cpp
bool MyFeature::doSomething(const std::string& param) {
    // Windows only
    return /* ... */;
}

// MyFeature_Linux.cpp
bool MyFeature::doSomething(const std::string& param) {
    // Linux only
    return /* ... */;
}

// MyFeature_macOS.cpp
bool MyFeature::doSomething(const std::string& param) {
    // macOS only
    return /* ... */;
}
```

**CMakeLists.txt**
```cmake
if(WIN32)
    set(PLATFORM_SOURCES MyFeature_Windows.cpp)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES MyFeature_Linux.cpp)
elseif(APPLE)
    set(PLATFORM_SOURCES MyFeature_macOS.cpp)
endif()

add_library(PlatformSpecific ${PLATFORM_SOURCES} ...)
```

### Usage from Common Code
```cpp
// In engine code (common)
#include "PlatformSpecific/DialogBox.hpp"

void showError(const std::string& message) {
    // Abstract API, transparent OS-specific implementation
    DialogBox::showError("Error", message);
}
```

### Handling Unsupported Features
```cpp
bool MyFeature::doSomething(const std::string& param) {
    if constexpr (Platform::isWindows()) {
        // Windows implementation
        return true;
    } else if constexpr (Platform::isLinux()) {
        // Linux implementation
        return true;
    } else {
        // macOS: not yet implemented
        Log::warning("MyFeature::doSomething not implemented on macOS");
        return false;  // Graceful failure
    }
}
```

## CRITICAL Attention Points

- **STRICT isolation**: Windows code NEVER touches Linux/macOS and vice-versa
- **No #ifdef in common code**: All platform-specific MUST stay in PlatformSpecific/
- **Cross-platform testing**: Test on all 3 OS before commit
- **Abstract API**: Common .hpp interface, separate implementations
- **CMake correctly configured**: Verify file selection per OS
- **Explicit warnings**: If feature unsupported, log clear warning
- **Documentation**: Indicate OS limitations in comments

## Detailed Documentation

Related systems:
- CMakeLists.txt - Cross-platform build configuration
- README.md - Supported platforms and requirements
