# PlatformSpecific System

Context for developing Emeraude Engine platform-specific code.

## Module Overview

Isolation of OS-specific code (Windows, Linux, macOS) with maximum abstractions to provide uniform cross-platform APIs.

---

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

## Directory Structure

```
PlatformSpecific/
├── AGENTS.md                    # This file
├── Helpers.hpp                  # Cross-platform helper declarations
├── Helpers.linux.cpp            # Linux helper implementations
├── Helpers.mac.cpp              # macOS helper implementations
├── Helpers.windows.cpp          # Windows helper implementations
├── SystemInfo.hpp/.cpp          # System information (+ platform files)
├── UserInfo.hpp/.cpp            # User information (+ platform files)
├── Types.hpp                    # Common type definitions
├── VideoCaptureDevice.hpp       # Cross-platform video capture interface
├── VideoCaptureDevice.cpp       # Shared code (width/height accessors, YUYV→RGBA)
├── VideoCaptureDevice.linux.cpp # V4L2 implementation
├── VideoCaptureDevice.mac.mm    # AVFoundation implementation
├── VideoCaptureDevice.windows.cpp # Media Foundation implementation
└── Desktop/
    ├── Commands.*               # System commands (taskbar, etc.)
    ├── Notification.*           # System notifications
    └── Dialog/
        ├── Abstract.hpp         # Base class for all dialogs
        ├── Types.hpp/.cpp       # Dialog types and aliases
        ├── Message.*            # Message dialogs (preset buttons)
        ├── CustomMessage.*      # Custom button dialogs
        ├── OpenFile.*           # File/folder open dialogs
        └── SaveFile.*           # File save dialogs
```

### File Naming Convention

Platform-specific implementations use suffixes:
| Suffix | Platform | Language |
|--------|----------|----------|
| `.linux.cpp` | Linux | C++ |
| `.mac.mm` | macOS | Objective-C++ |
| `.windows.cpp` | Windows | C++ |

CMake selects the appropriate file based on target platform.

---

## Helpers System

Platform-specific utility functions are centralized in `Helpers.hpp` with separate implementations per OS.

### Linux Helpers (`Helpers.linux.cpp`)

| Function | Purpose |
|----------|---------|
| `checkProgram(name)` | Checks if program exists via `which` |
| `hasZenity()` | Cached check for zenity availability |
| `hasKdialog()` | Cached check for kdialog availability |
| `isKdeDesktop()` | Checks `XDG_CURRENT_DESKTOP` for "KDE" |
| `escapeShellArg(arg)` | Escapes string for shell (single quotes) |
| `executeCommand(cmd, exitCode)` | Runs command via `popen`, returns stdout |
| `buildZenityFilters(filters)` | Builds `--file-filter=` arguments |
| `buildKdialogFilters(filters)` | Builds kdialog filter string |

**Tool Selection Logic** (used in all Linux dialogs):
```cpp
// Prefer kdialog on KDE, zenity otherwise
const bool useKdialog = hasKdialog() && (!hasZenity() || isKdeDesktop());
```

### Windows Helpers (`Helpers.windows.cpp`)

| Function | Purpose |
|----------|---------|
| `convertUTF8ToWide(str)` | UTF-8 `std::string` → `std::wstring` |
| `convertWideToUTF8(wstr)` | `std::wstring` → UTF-8 `std::string` |
| `convertANSIToWide(str)` | ANSI `std::string` → `std::wstring` |
| `convertWideToANSI(wstr)` | `std::wstring` → ANSI `std::string` |
| `createExtensionFilter(...)` | Builds `COMDLG_FILTERSPEC` array |
| `getStringValueFromHKLM(...)` | Reads Windows registry |
| `createConsole(title)` | Creates debug console window |
| `attachToParentConsole()` | Attaches to parent console |

### macOS Helpers (`Helpers.mac.cpp`)

macOS helpers are minimal - Objective-C provides native string handling. NSString conversion is done inline using `stringWithUTF8String:`.

---

## UTF-8 Encoding Practices

All internal strings use **UTF-8 encoding**. Platform-specific conversions are required at boundaries.

### Windows
Always use `convertUTF8ToWide()` before passing strings to Windows APIs:
```cpp
const std::wstring wsTitle = convertUTF8ToWide(this->title());
const std::wstring wsMessage = convertUTF8ToWide(m_message);
MessageBoxW(parentWindow, wsMessage.data(), wsTitle.data(), flags);
```

### macOS
Use `stringWithUTF8String:` for NSString conversion:
```cpp
NSString* title = [NSString stringWithUTF8String:this->title().c_str()];
NSString* message = [NSString stringWithUTF8String:m_message.c_str()];
```

**IMPORTANT**: Do NOT use `stringWithCString:encoding:defaultCStringEncoding` - it may not handle UTF-8 correctly.

### Linux
Shell commands receive UTF-8 directly (modern Linux systems are UTF-8 native). Use `escapeShellArg()` to safely quote arguments.

---

## Dialog System

### Dialog Classes

| Class | Purpose | Buttons |
|-------|---------|---------|
| `Message` | Standard message dialogs | Preset layouts (`OK`, `OKCancel`, `YesNo`, `Quit`) |
| `CustomMessage` | Custom button dialogs | 1-6 custom labels |
| `OpenFile` | File/folder selection | N/A (system buttons) |
| `SaveFile` | Save location selection | N/A (system buttons) |

### File Path Handling

Dialogs use `std::filesystem::path` for file paths:
```cpp
// OpenFile returns vector of paths
std::vector<std::filesystem::path> m_filepaths;

// SaveFile returns single path
std::filesystem::path m_filepath;
```

### Default Path Support

Both `OpenFile` and `SaveFile` support setting a default directory. `SaveFile` also supports a default filename.

**OpenFile**:
```cpp
Dialog::OpenFile dialog{"Open", false, false};
dialog.setDefaultDirectory(std::filesystem::path("/some/directory"));
// Dialog opens in specified directory
```

**SaveFile**:
```cpp
Dialog::SaveFile dialog{"Save"};
dialog.setDefaultDirectory(std::filesystem::path("/some/directory"));
dialog.setDefaultFilename("myfile.txt");
// Dialog opens in /some/directory with "myfile.txt" pre-filled
```

See: `Dialog/SaveFile.hpp:setDefaultFilename()`, `Dialog/SaveFile.hpp:setDefaultDirectory()`, `Dialog/OpenFile.hpp:setDefaultDirectory()`

**Platform implementations**:
| Platform | OpenFile default dir | SaveFile default dir | SaveFile default filename | SaveFile auto-extension |
|----------|---------------------|---------------------|--------------------------|------------------------|
| Windows | `IFileOpenDialog::SetFolder()` | `IFileSaveDialog::SetFolder()` | `SetFileName()` | `SetDefaultExtension()` from first filter |
| Linux | kdialog positional arg / zenity `--filename=` | Same | Appended to path arg | Handled by desktop tool |
| macOS | `NSOpenPanel setDirectoryURL:` | `NSSavePanel setDirectoryURL:` | `setNameFieldStringValue:` | Handled by `allowedContentTypes` |

### Linux Dialog Implementation (zenity/kdialog)

Linux dialogs use native desktop tools via shell commands.

**Zenity Quirks** (as of 2025+):
| Issue | Solution |
|-------|----------|
| Icon parameter | Use `--icon=` not `--icon-name=` (deprecated) |
| Confirm overwrite | `--confirm-overwrite` deprecated (now default) |
| Multi-select separator | Use `--separator='\n'` for newline separation |
| Custom buttons | Use `--question --switch --extra-button=Label` |

**kdialog Limitations**:
- Maximum 3 buttons for question dialogs (`--yesnocancel`)
- For 4+ buttons, fall back to zenity even on KDE

**Button Index Mapping**:
| Tool | Method | Index Extraction |
|------|--------|------------------|
| zenity (switch mode) | Outputs clicked button text | Match against button labels |
| kdialog | Exit codes: 0=Yes, 1=No, 2=Cancel | Direct mapping |

### Windows Dialog Implementation

| Dialog | API |
|--------|-----|
| `Message` | `MessageBoxW()` |
| `CustomMessage` | `TaskDialogIndirect()` with `TASKDIALOG_BUTTON[]` |
| `OpenFile` | `IFileOpenDialog` (COM) |
| `SaveFile` | `IFileSaveDialog` (COM) |

**CustomMessage Button IDs**: Start at 100 to avoid conflicts with common button IDs.

**Required for TaskDialog**: Common Controls v6 manifest dependency (automatically injected via `#pragma comment`).

### macOS Dialog Implementation

| Dialog | API |
|--------|-----|
| `Message` / `CustomMessage` | `NSAlert` with `addButtonWithTitle:` |
| `OpenFile` | `NSOpenPanel` |
| `SaveFile` | `NSSavePanel` |

**NSAlert Button Mapping**: Returns 1000, 1001, 1002... for buttons in order added. Convert to 0-based: `index = button - 1000`.

**File Type Filtering**: Use `UTType` with `allowedContentTypes` (macOS 12.0+).

### OpenFile Constraints

**CRITICAL**: File filters must NOT be applied when selecting folders (`m_selectFolder = true`):
```cpp
// Correct pattern
if (!m_selectFolder && !m_extensionFilters.empty()) {
    // Apply filters only for file selection
}
```

Applying filters to folder selection breaks the dialog on Windows and macOS.

---

## Taskbar Progress

### Windows (`ITaskbarList3`)

| `ProgressMode` | Windows Flag | Visual |
|----------------|--------------|--------|
| `None` | `TBPF_NOPROGRESS` | Hidden |
| `Normal` | `TBPF_NORMAL` | Green bar |
| `Indeterminate` | `TBPF_INDETERMINATE` | Marquee animation |
| `Error` | `TBPF_ERROR` | Red bar |
| `Paused` | `TBPF_PAUSED` | Yellow bar |

### macOS (`NSDockTile`)

- Uses `NSProgressIndicator` overlay on dock icon
- Pass negative value to remove progress indicator
- Mode parameter is ignored (only normal progress supported)

### Linux

**Not supported** on standard Linux desktops. Would require `libunity` for Ubuntu Unity only.

---

## Development Patterns

### Adding a New Platform-Specific Feature

**1. Define the common interface** (`.hpp`):
```cpp
// MyFeature.hpp
class MyFeature {
public:
    static bool doSomething(const std::string& param);
};
```

**2. Create platform implementations**:
```
MyFeature.linux.cpp
MyFeature.mac.mm
MyFeature.windows.cpp
```

**3. Update CMakeLists.txt**:
```cmake
if(WIN32)
    list(APPEND PLATFORM_SOURCES MyFeature.windows.cpp)
elseif(UNIX AND NOT APPLE)
    list(APPEND PLATFORM_SOURCES MyFeature.linux.cpp)
elseif(APPLE)
    list(APPEND PLATFORM_SOURCES MyFeature.mac.mm)
endif()
```

### Handling Unsupported Features

```cpp
bool MyFeature::doSomething(const std::string& param) {
    // Implementation not available on this platform
    Tracer::warning(ClassId, "MyFeature::doSomething not implemented on this platform");
    return false;  // Graceful failure
}
```

---

## Video Capture System

Cross-platform webcam/video capture via `VideoCaptureDevice`.

### Architecture

| File | Purpose |
|------|---------|
| `VideoCaptureDevice.hpp` | Common interface: `enumerateDevices()`, `open()`, `captureFrame()`, `close()` |
| `VideoCaptureDevice.cpp` | Shared code: `width()`, `height()`, `convertYUYVtoRGBA()` |
| `VideoCaptureDevice.linux.cpp` | V4L2 implementation (synchronous mmap) |
| `VideoCaptureDevice.mac.mm` | AVFoundation implementation (async delegate → sync copy) |
| `VideoCaptureDevice.windows.cpp` | Media Foundation implementation (synchronous IMFSourceReader) |

### Platform-Specific Details

**Linux (V4L2):**
- Direct `open()` → `ioctl(VIDIOC_*)` → `mmap` buffer → `select()` for frame readiness
- Format: YUYV (converted via shared `convertYUYVtoRGBA()`)

**macOS (AVFoundation):**
- `FrameCaptureDelegate` (ObjC class in .mm) receives frames asynchronously via `AVCaptureVideoDataOutputSampleBufferDelegate`
- Latest frame stored in `std::mutex`-protected buffer with `std::condition_variable` for first-frame synchronization
- `MacCaptureContext` struct (stored via `void* m_platformHandle`) holds `AVCaptureSession`, input, output, delegate, dispatch queue
- `open()` waits up to 3s for first frame arrival (AVFoundation's `startRunning` is async)
- Format: BGRA (converted to RGBA by swapping B↔R per pixel)
- Uses `AVCaptureDeviceDiscoverySession` for device enumeration (handles deprecation of `devicesWithMediaType:`)
- All ObjC code wrapped in `@autoreleasepool`

**Windows (Media Foundation):**
- `MFCaptureContext` struct (stored via `void* m_platformHandle`) holds `IMFSourceReader`, `IMFMediaSource`, format/COM flags
- `open()` tries RGB32 first, falls back to YUY2
- RGB32 (BGRA) → RGBA via B↔R swap; YUY2 via shared `convertYUYVtoRGBA()`
- COM lifecycle: `CoInitializeEx`/`CoUninitialize` with `RPC_E_CHANGED_MODE` handling
- MF lifecycle: `MFStartup`/`MFShutdown` tracked per context
- String conversions use `convertWideToUTF8()`/`convertUTF8ToWide()` from `Helpers.hpp` (**no local duplicates**)

### CMake Dependencies (`SetupVideoCapture.cmake`)

| Platform | Linked Libraries |
|----------|-----------------|
| macOS | `AVFoundation`, `CoreMedia`, `CoreVideo` frameworks |
| Windows | `Mfplat.lib`, `Mfreadwrite.lib`, `Mf.lib`, `Mfuuid.lib` |
| Linux | None (V4L2 uses kernel headers) |

---

## Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| Platform macros in implementation files | Remove them - CMake handles file selection |
| Wrong UTF-8 conversion on Windows | Always use `convertUTF8ToWide()` |
| `defaultCStringEncoding` on macOS | Use `stringWithUTF8String:` instead |
| File filters with folder selection | Skip filters when `m_selectFolder = true` |
| More than 3 buttons with kdialog | Fall back to zenity |
| Blocking shell commands on Linux | Use `executeCommand()` which handles `popen`/`pclose` |
| Duplicating string conversions on Windows | Use `Helpers.hpp` functions (`convertUTF8ToWide`, `convertWideToUTF8`). **Never** write local `MultiByteToWideChar`/`WideCharToMultiByte` wrappers |

---

## Attention Points

- **STRICT isolation**: Windows code NEVER touches Linux/macOS and vice-versa
- **No platform macros in .cpp/.mm files**: CMake selects files per OS
- **Cross-platform testing**: Test on all 3 OS before commit
- **Abstract API**: Common `.hpp` interface, separate implementations
- **CMake correctly configured**: Verify file selection per OS
- **Explicit warnings**: If feature unsupported, log clear warning
- **UTF-8 everywhere**: Convert at platform boundaries only

---

## Related Documentation

- `Desktop/Dialog/Types.hpp` - `MessageType`, `ButtonLayout`, `ButtonLabels` definitions
- `Libs/StaticVector.hpp` - Fixed-capacity vector used for `ButtonLabels`
- CMakeLists.txt - Cross-platform build configuration
