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

### Desktop Directory (`Desktop/`)
- `Commands.*` - System commands (taskbar flash, progress, run applications)
- `Dialog/Abstract.*` - Base class for all dialogs
- `Dialog/Types.hpp` - Dialog types and aliases (`MessageType`, `ButtonLayout`, `ButtonLabels`)
- `Dialog/Message.*` - Message dialogs (info, warning, error, question) with preset `ButtonLayout`
- `Dialog/CustomMessage.*` - Custom button dialogs (1-6 buttons with custom labels)
- `Dialog/OpenFile.*` - File/folder open dialogs
- `Dialog/SaveFile.*` - File save dialogs
- `Notification.*` - System notifications (toast/banner)
- CMakeLists.txt - Platform file selection

### File Naming Convention
Platform-specific implementations use suffixes:
- `.linux.cpp` - Linux implementation
- `.mac.mm` - macOS implementation (Objective-C++)
- `.windows.cpp` - Windows implementation

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

## Platform-Specific Implementation Details

### Linux Dialog Implementation (zenity/kdialog)
Linux dialogs use native desktop tools via shell commands. See: `Desktop/Dialog/*.linux.cpp`

**Tool Selection Logic**:
```cpp
// Prefer kdialog on KDE, zenity otherwise
const bool useKdialog = hasKdialog() && (!hasZenity() || isKdeDesktop());
```

**Key Functions** (in anonymous namespace):
- `checkProgram()` - Checks tool availability via `which`
- `hasZenity()` / `hasKdialog()` - Cached tool detection
- `isKdeDesktop()` - Checks `XDG_CURRENT_DESKTOP` for "KDE"
- `escapeShellArg()` - Shell argument escaping with single quotes
- `executeCommand()` - Runs command via `popen`/`pclose`

**Zenity Quirks** (as of 2024+):
- Use `--icon=` not `--icon-name=` (deprecated)
- `--confirm-overwrite` is deprecated (now default behavior)
- Multi-select separator: use `--separator=$'\\n'` (bash syntax for real newline)

### Windows Taskbar Progress
Uses COM `ITaskbarList3` interface. See: `Desktop/Commands.windows.cpp:setTaskbarIconProgression()`

**Progress Modes** (`ProgressMode` enum):
| Mode | Windows Flag | Visual |
|------|--------------|--------|
| `None` | `TBPF_NOPROGRESS` | Hidden |
| `Normal` | `TBPF_NORMAL` | Green bar |
| `Indeterminate` | `TBPF_INDETERMINATE` | Marquee animation |
| `Error` | `TBPF_ERROR` | Red bar |
| `Paused` | `TBPF_PAUSED` | Yellow bar |

### macOS Dock Progress
Uses `NSDockTile` with `NSProgressIndicator`. See: `Desktop/Commands.mac.mm:setTaskbarIconProgression()`

- Progress bar overlays the dock icon
- Pass negative value to remove the progress indicator
- Mode parameter is ignored (macOS only supports normal progress)

### Linux Taskbar Progress
**Not supported** on standard Linux desktops (GNOME, KDE, etc.).
Would require `libunity` for Ubuntu Unity desktop only.
See: `Desktop/Commands.linux.cpp:setTaskbarIconProgression()` (stub)

### CustomMessage Dialog (Custom Buttons)

Dialogs with 1-6 custom button labels. See: `Desktop/Dialog/CustomMessage.*`

**Type**: `ButtonLabels` (`Libs::StaticVector<std::string, 6>`)

**Usage**:
```cpp
Dialog::CustomMessage dialog{"Title", "Message", {"Save", "Don't Save", "Cancel"}, MessageType::Warning};
dialog.execute(&window);
int clickedIndex = dialog.getClickedButtonIndex();  // 0-based, -1 if dismissed
```

**Platform Implementations**:
| Platform | API | Button Index Mapping |
|----------|-----|---------------------|
| macOS | `NSAlert` with `addButtonWithTitle:` | `NSAlertFirstButtonReturn (1000)` → 0 |
| Linux | `zenity --question --switch --extra-button=...` | Parses button text from stdout |
| Windows | `TaskDialogIndirect` with `TASKDIALOG_BUTTON[]` | Button IDs 100+ → 0-based |

**Critical**: First button in array is the default/primary button on all platforms.

### OpenFile Dialog Constraints

**CRITICAL**: File filters must NOT be applied when selecting folders (`m_selectFolder = true`).

See: `Desktop/Dialog/OpenFile.windows.cpp`, `Desktop/Dialog/OpenFile.mac.mm`

```cpp
// Correct pattern - skip filters for folder selection
if (!m_selectFolder && !m_extensionFilters.empty()) {
    // Apply filters only for file selection
}
```

This constraint was discovered as a bug fix - applying filters to folder selection breaks the dialog on Windows and macOS.

## Detailed Documentation

Related systems:
- CMakeLists.txt - Cross-platform build configuration
- README.md - Supported platforms and requirements
