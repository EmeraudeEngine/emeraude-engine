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
├── Helpers.hpp                  # Cross-platform helper declarations (incl. Windows-only file-dialog
│                                #   thread helper runFileDialogOnDedicatedThread())
├── Helpers.linux.cpp            # Linux helper implementations
├── Helpers.mac.cpp              # macOS helper implementations
├── Helpers.windows.cpp          # Windows helper implementations (incl. runFileDialogOnDedicatedThread()
│                                #   + file-local CenteringOwner — dedicated STA thread + centering proxy)
├── SystemInfo.hpp/.cpp          # System information (+ hybrid CPU detection via hwloc)
├── UserInfo.hpp/.cpp            # User information (+ platform files)
├── Types.hpp                    # Common type definitions (CPU struct with E/P-core counts)
├── StorageInfo.hpp              # Cross-platform storage enumeration interface
├── StorageInfo.linux.cpp        # /proc/mounts + statvfs + sysfs
├── StorageInfo.mac.mm           # getmntinfo + DiskArbitration
├── StorageInfo.windows.cpp      # GetLogicalDriveStrings + GetDiskFreeSpaceEx
├── VideoCaptureDevice.hpp       # Cross-platform video capture interface
├── VideoCaptureDevice.cpp       # Shared code (width/height accessors, YUYV→RGBA)
├── VideoCaptureDevice.linux.cpp # V4L2 implementation
├── VideoCaptureDevice.mac.mm    # AVFoundation implementation
├── VideoCaptureDevice.windows.cpp # Media Foundation implementation
└── Desktop/
    ├── Commands.*               # System commands (taskbar, etc.)
    ├── Notification.*           # System notifications
    └── Dialog/
        ├── Abstract.hpp           # Base class for all dialogs
        ├── Types.hpp/.cpp         # Dialog types and aliases
        ├── Message.*              # Message dialogs (preset buttons)
        ├── CustomMessage.*        # Custom button dialogs
        ├── OpenFile.*             # File/folder open dialogs
        └── SaveFile.*             # File save dialogs
```

> [!NOTE]
> The Windows dedicated-STA-thread file-dialog helper (`runFileDialogOnDedicatedThread()`
> + the file-local `CenteringOwner`) lives in `PlatformSpecific/Helpers.{hpp,windows.cpp}`,
> **not** under `Desktop/Dialog/` — it is OS-machinery shared by `OpenFile`/`SaveFile`, so it
> sits with the other Windows helpers.

### File Naming Convention

Platform-specific implementations use suffixes:
| Suffix | Platform | Language |
|--------|----------|----------|
| `.linux.cpp` | Linux | C++ |
| `.mac.mm` | macOS | Objective-C++ |
| `.windows.cpp` | Windows | C++ |

CMake selects the appropriate file based on target platform.

---

## Windows: native file dialogs run on a dedicated STA thread

`OpenFile.windows.cpp` and `SaveFile.windows.cpp` do **not** show the modern COM dialog
(`IFileOpenDialog` / `IFileSaveDialog`) on the engine's main thread. Their `execute()` is a thin
wrapper that delegates to the shared helper `runFileDialogOnDedicatedThread()`
(`Helpers.hpp` / `Helpers.windows.cpp`): it spawns a **dedicated STA thread** with an empty
message queue, runs the dialog body (`OpenFile::showDialog` / `SaveFile::showDialog`) there, and
`join()`s it. The helper owns the thread, COM init, and the centering proxy; the per-dialog
`showDialog()` only does the COM dialog work and receives the resolved parent `HWND`. On failure
to spawn the worker (OS resource exhaustion), it falls back to running the dialog inline on the
caller thread.

**Why (perf):** on the main thread the dialog's modal message loop pumps the heavy main-thread
traffic (rendering, input, CEF). The Windows shell reacts by re-resolving its navigation pane on
every burst — thousands of redundant known-folder / cloud-sync-root (`SyncRootManager`) registry
lookups — delaying the dialog **3–5 s** on machines whose known folders are redirected to a cloud
provider (OneDrive, the Windows default). On a quiet dedicated thread the pane resolves once and
the dialog appears in **~140 ms**.

> [!WARNING]
> Do **not** "simplify" by calling `Show()` directly on the calling thread — it silently
> reintroduces the 3–5 s freeze on any machine with cloud-redirected known folders (invisible on
> a plain dev box, hence hard to diagnose).

### Dialog centering — `CenteringOwner` (TECH-1838)

Passing the main window as the modal dialog's owner is **not** possible here: the main window
lives on the main thread, which is blocked on `join()` while the dialog is up, so a cross-thread
owner risks a message-pump deadlock (the modal dialog's implicit `WM_ENABLE` / activation sends to
the owner thread would never return). The first perf fix therefore used a **null owner** — which
also dropped native placement (dialog no longer centered on the app window, wrong monitor in
multi-screen). Note the lag and the owner are **independent**: the lag is purely a function of the
thread on which `Show()` pumps messages, not of the owner — so the centering fix below does not
reintroduce it.

`runFileDialogOnDedicatedThread()` restores placement **without** a cross-thread owner:

1. On the **caller thread**, before spawning the worker, it reads the main window's screen
   rectangle (`GetWindowRect`) and DPI awareness context (`GetWindowDpiAwarenessContext`), passed
   **by value** to the worker (no cross-thread HWND deref on the worker).
2. On the **worker (STA) thread**, `SetThreadDpiAwarenessContext(...)` matches the main window,
   then a `CenteringOwner` (file-local in `Helpers.windows.cpp`) is built — a hidden
   top-level window (built-in `STATIC` class, `WS_POPUP` without `WS_VISIBLE`) positioned over that
   rectangle. It is created **and destroyed on the worker thread** (RAII), so there is **no
   cross-thread owner** and no deadlock risk — the owner enable/disable sends stay same-thread.
3. The proxy `HWND` is passed to the dialog body, which feeds it to `Show(parentWindow)` and to the
   legacy `OPENFILENAMEW.hwndOwner` / `BROWSEINFOW.hwndOwner` paths.

The shell centers the dialog on the proxy natively, at creation time — **no post-show jump, no
flash**, correct monitor in multi-screen. The proxy is never shown; the shell only needs its
geometry.

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

### Windows: native file dialogs run on a dedicated STA thread (critical)

On Windows, `OpenFile::execute()` and `SaveFile::execute()` show the modern
`IFileOpenDialog` / `IFileSaveDialog` on a **dedicated STA thread**, not on the calling
thread. This is mandatory for performance — not stylistic.

**Why:** the dialog's modal message loop pumps **every** message of the thread it runs on.
Shown from a busy thread (the engine main loop pumping rendering, input, and — under
app_system — CEF), the Windows shell re-resolves its navigation pane on every message
burst: thousands of redundant known-folder / cloud-sync-root (`SyncRootManager`) registry
lookups. On a machine whose known folders are redirected to a cloud provider (OneDrive —
the Windows default), this delays the dialog by **3–5 s**. A dedicated thread has an empty
message queue, so the pane is resolved once and the dialog appears in ~140 ms (measured
×25 speed-up). Apps with an idle UI thread (e.g. Notepad++) never hit this — which is why
the same dialog is instant elsewhere on the same machine.

**Implementation:** `execute()` delegates to `runFileDialogOnDedicatedThread()`
(`Helpers.{hpp,windows.cpp}`), passing its COM dialog body (`showDialog()`) as a callback. The
helper spawns a `std::thread` that `CoInitializeEx(STA)`s, builds the hidden `CenteringOwner`
(see § Dialog centering above), runs the body, then `join()`s — the caller blocks on `join()`
because `execute()` is synchronous by contract. The centering owner is created **on the worker
thread** (never a cross-thread owner), which is what makes native centering possible without a
modal-pump deadlock. If the worker thread cannot be spawned, the helper runs the dialog inline on
the caller thread (slower on cloud-redirected machines, but functional). See the comment block in
`Dialog/OpenFile.windows.cpp` and `Helpers.windows.cpp`.

> [!WARNING]
> Do **not** "simplify" this back to a direct `Show()` on the calling thread — it silently
> reintroduces the 3–5 s freeze on any machine with cloud-redirected known folders (i.e.
> most Windows installs with OneDrive). The cost is invisible on a developer machine
> without OneDrive folder redirection.

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

#### Legacy Win32 file dialogs (accessibility compatibility fallback)

`OpenFile` / `SaveFile` default to the **modern COM** dialogs (`IFileOpenDialog` / `IFileSaveDialog`). The setting `Core/Compatibility/Windows/UseLegacyFileDialogs` (`CompatibilityWindowsUseLegacyFileDialogsKey`, default **`false`**) switches them to the **legacy Win32** API (`GetOpenFileNameW` / `GetSaveFileNameW`, plus a folder-picker variant).

- **Why it exists — not dead code:** the modern COM dialog can misbehave with some assistive technologies (screen readers / accessibility tools) on Windows 11. The Win32 path is a documented escape hatch for those users. Keep it.
- **Read-only at runtime:** read via `getOrSetDefault()` in `OpenFile.windows.cpp:execute()` and `SaveFile.windows.cpp:execute()`; never written by the engine. Toggle only by editing the settings file.
- **COM is primary, Win32 is the fallback:** the dedicated-STA-thread performance work above applies to the **COM** path only. The Win32 path is a thinner fallback (no dedicated-thread treatment, no `SyncRootManager` mitigation) kept for accessibility compatibility. Validate it on the target Windows version before relying on it.

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

## Storage Information (`PlatformSpecific::StorageInfo`)

**Files**: `StorageInfo.hpp` + `StorageInfo.{linux,mac,windows}.cpp`

Cross-platform mounted drive enumeration with space usage and removable detection.

**API**:
```cpp
namespace EmEn::PlatformSpecific::StorageInfo
{
    struct DriveInfo {
        std::string filesystem;      // Device path ("/dev/sda1") or description
        std::string mounted;         // Mount point ("/", "C:")
        std::string fsType;          // "ext4", "NTFS", "apfs", "exfat"
        uint64_t totalBytes{0};
        uint64_t usedBytes{0};
        uint64_t availableBytes{0};
        bool removable{false};       // USB, SD card, etc.
    };

    [[nodiscard]]
    std::vector< DriveInfo > listDrives () noexcept;
}
```

**Platform details**:
| Platform | Source | Space API | Removable detection |
|----------|--------|-----------|---------------------|
| Linux | `/proc/mounts` | `statvfs()` | `/sys/block/{dev}/removable` |
| macOS | `getmntinfo()` | `statfs` struct | DiskArbitration framework |
| Windows | `GetLogicalDriveStringsW()` | `GetDiskFreeSpaceExW()` | `GetDriveTypeW()` (DRIVE_REMOVABLE/DRIVE_CDROM) |

**Filtering**: Virtual/pseudo filesystems excluded. Linux skips `/dev/loop*` (snaps). Windows skips network/unknown drives.

---

## CPU Hybrid Core Detection (SystemInfo Enhancement)

**Files**: `SystemInfo.cpp`, `Types.hpp`

Enhanced CPU information with efficiency/performance core counts for hybrid architectures (Intel 12th gen+, Apple M1/M2/M3).

**New fields** in `PlatformSpecific::CPU` struct (`Types.hpp`):
```cpp
uint32_t efficiencyCores{0};   // E-cores (via hwloc cpukinds API)
uint32_t performanceCores{0};  // P-cores (via hwloc cpukinds API)
```

**Detection**: Uses hwloc >= 2.4 `cpukinds` API. Kind 0 = lowest performance (E-cores), Kind N-1 = highest (P-cores). On non-hybrid CPUs (`numKinds < 2`), both fields remain 0.

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
- `Base/StaticVector.hpp` - Fixed-capacity vector used for `ButtonLabels`
- CMakeLists.txt - Cross-platform build configuration
