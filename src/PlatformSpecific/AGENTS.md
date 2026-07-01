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
│                                #   — dedicated STA thread, dialog owned by the real main window)
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
> The Windows dedicated-STA-thread file-dialog helper (`runFileDialogOnDedicatedThread()`) lives in
> `PlatformSpecific/Helpers.{hpp,windows.cpp}`, **not** under `Desktop/Dialog/` — it is OS-machinery
> shared by `OpenFile`/`SaveFile`, so it sits with the other Windows helpers.

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
waits for it. The helper owns the thread, COM init, the cross-thread owner handshake
(`AttachThreadInput`) and the message-pumping wait; the per-dialog `showDialog()` only does the COM
dialog work and receives the owner `HWND`. On failure to spawn the worker (OS resource exhaustion),
it falls back to running the dialog inline on the caller thread.

**Why (perf):** the dialog's modal message loop pumps **every** message of the thread it runs on.
On the engine main thread that means the heavy main-thread traffic (rendering, input, and — under
app_system — CEF), and the Windows shell reacts by re-resolving its navigation pane on every burst:
thousands of redundant known-folder / cloud-sync-root (`SyncRootManager`) registry lookups. On a
machine whose known folders are redirected to a cloud provider (OneDrive, the Windows default), this
delays the dialog **3–5 s**. A dedicated thread has an empty message queue, so the pane resolves once
and the dialog appears in **~140 ms** (measured ≈×25 speed-up). Apps with an idle UI thread (e.g.
Notepad++) never hit this — which is why the same dialog is instant elsewhere on the same machine.

> [!WARNING]
> Do **not** "simplify" by calling `Show()` directly on the calling thread — it silently
> reintroduces the 3–5 s freeze on any machine with cloud-redirected known folders (invisible on
> a plain dev box, hence hard to diagnose).

### Owner-based modality, centering & Z-order

The dialog is shown **owned by the real main window** (its `HWND` is passed to the dialog body and
fed to `IFileDialog::Show(owner)` and the legacy `OPENFILENAMEW.hwndOwner` / `BROWSEINFOW.hwndOwner`).
Win32 owner relationships are the OS-native mechanism for all three behaviors the dialog needs, so
the OS handles them for free:

- **Modality** — the OS disables the owner while the dialog is up.
- **Centering** — the shell positions the dialog on its owner, correct monitor, no post-show jump.
- **Z-order** — an owned window is always above its owner and is **raised with it on activation**, so
  the dialog resurfaces after an alt-tab / taskbar click instead of being buried behind the main
  window.

The owner is **cross-thread**: the dialog runs on the worker (STA) thread, the owner lives on the
main thread. A cross-thread owner deadlocks if the owner thread stops pumping messages (e.g. a bare
`join()`), so it is made safe here by two things the helper does:

1. **The caller pumps messages while it waits** (see § Responsiveness) — so the owner's implicit
   `WM_ENABLE` / activation sends, dispatched from the worker, are serviced instead of deadlocking.
2. **`AttachThreadInput(workerThreadId, ownerThreadId, TRUE)`** for the dialog's lifetime — merges the
   two threads' input state so activation and **focus return on close** behave as if same-thread.
   It is detached after the dialog returns. (Attaching a thread to itself fails harmlessly on the
   inline-fallback path, where owner and dialog are already on the same thread.)

The worker still replicates the owner's DPI awareness context (`SetThreadDpiAwarenessContext`) so the
dialog renders at the right scale. Everything is **gated on `parentToWindow`**: when it is `false`
the dialog is shown with a null owner — no modality, no centering — for callers that deliberately
want an ownerless dialog.

### Responsiveness — message-pumping wait

`execute()` is synchronous by contract (the app_system IPC handler needs the result to send its
ZeroMQ reply), so the caller must wait for the worker — but a bare `join()` **hard-blocks** the main
thread, which breaks two things:

1. The dialog **owns the main window cross-thread**, so the OS dispatches `WM_ENABLE` / activation
   sends to the main thread from the worker; a non-pumping thread never services them, deadlocking
   the native modal handshake.
2. After ~5 s a non-pumping thread is marked **"Not Responding"** (DWM ghost: greyed snapshot,
   spinning cursor), even though the dialog is alive on the worker.

`runFileDialogOnDedicatedThread()` therefore replaces the bare `join()` with a **drain-then-wait**
loop on the worker's thread handle: each iteration first drains the caller thread's own queue
(`PeekMessage` / `Translate` / `Dispatch`), then blocks in `MsgWaitForMultipleObjectsEx` until the
worker terminates or new serviceable work arrives, then `join()`s (which returns immediately).
`WM_QUIT` is handled specially — it is re-posted (`PostQuitMessage`) and the dialog is waited out, so
the engine's main loop sees the quit only after the dialog closes.

> [!WARNING]
> The wake mask **excludes `QS_INPUT`** (it is `QS_PAINT | QS_TIMER | QS_POSTMESSAGE |
> QS_SENDMESSAGE`), and `MWMO_INPUTAVAILABLE` is **not** used (hence drain-*first*). This is not
> cosmetic: `AttachThreadInput` shares the worker's input queue with the main thread, so mouse-moves
> / keystrokes aimed at the **dialog** flag input as "available" on the main thread. With `QS_INPUT`
> in the mask (or with `MWMO_INPUTAVAILABLE`), the wait would return immediately and repeatedly while
> `PeekMessage` — which only retrieves messages for the main thread's *own* windows — removes nothing:
> a **100 % CPU busy-spin whenever the user moves the mouse over the dialog**. The main window is
> disabled by the modal and has no input to process anyway; it only needs paint / posted (CEF) / timer
> messages. Sent messages (the owner's `WM_ENABLE` / activation handshake) are serviced by the wait
> regardless of the mask.

This does **not** reintroduce the 3–5 s perf storm: that was the *dialog's* modal loop pumping heavy
traffic, and the dialog's loop runs on the worker. This pump only services the main thread's own
messages (paint, CEF, DWM responsiveness pings). The main window is disabled by the native modal (it
is the owner), so the pumped input is ignored — **responsive but non-interactive**, the correct
modal behavior.

### Reentrancy — a reentrant open is refused, not nested

The pump above dispatches messages, which re-enters the main window's `WndProc`. A dispatched
message can call back into `runFileDialogOnDedicatedThread()` to open a **second** native file dialog
on the same thread. This is **refused at the top of the function** (a `thread_local` "active" flag,
RAII-scoped over the whole operation): a reentrant call logs a warning and returns `false`, which the
caller treats as a cancellation.

Refusing — rather than nesting — is the only robust fix, because the damage is done by the nested
dialog *opening*, not by any nested pumping:

- A nested dialog owned by the same main window calls `EnableWindow(owner, FALSE)` on `Show()` and
  `EnableWindow(owner, TRUE)` on close. Win32 owner enable/disable is **not ref-counted**, so the
  inner dialog's close would **re-enable the main window while the outer dialog is still modal** —
  the app becomes interactive under an open modal. Preventing the nested `Show()` is what closes this
  hole; merely avoiding a nested pump would not.
- Two stacked native file choosers are not a meaningful UX anyway.

> [!WARNING]
> Do **not** "fix" a reentrant dialog request by opening it on another worker/owner or by pumping
> differently — a second dialog owned by the same window reopens the non-ref-counted owner-enable
> hole above. The reentrant request must be refused (or deferred by the caller until the first
> dialog closes).

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
| `cleanLoaderEnvCommand(cmd)` | Prefixes a command with `env -u LD_LIBRARY_PATH -u LD_PRELOAD` so a spawned system tool (zenity/kdialog) runs with a pristine dynamic-loader environment |
| `executeCommand(cmd, exitCode)` | Runs command via `popen` (through `cleanLoaderEnvCommand`), returns stdout |
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

### Windows dialog mechanism (dedicated STA thread, main-window owner)

On Windows, `OpenFile::execute()` / `SaveFile::execute()` delegate to
`runFileDialogOnDedicatedThread()`, which shows the modern `IFileOpenDialog` / `IFileSaveDialog` on a
**dedicated STA thread owned by the main window**. The complete rationale and mechanism — perf
(why a dedicated thread), owner-based modality / centering / Z-order, and the message-pumping wait —
are documented once in the top-level section **§ Windows: native file dialogs run on a dedicated STA
thread** (above). It is not repeated here to avoid drift.

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

> [!IMPORTANT]
> **Spawn desktop tools with a clean dynamic-loader environment.** zenity/kdialog are
> *system* programs and must load *system* libraries. When the host application is
> launched with its bundled library directory pushed into `LD_LIBRARY_PATH` (AppImage
> wrappers, dev launchers, Steam-like parents), that value is inherited by the spawned
> child and a bundled library can shadow its system counterpart. The observed failure:
> a consumer (app_system) bundles CEF's stripped `libvulkan.so.1`, which does **not**
> export `vkCreateXlibSurfaceKHR`; a GTK-4 zenity built with the Vulkan renderer
> (Fedora 43+) then aborts with `undefined symbol: vkCreateXlibSurfaceKHR`. Both spawn
> paths therefore route through `cleanLoaderEnvCommand()` (prefix
> `env -u LD_LIBRARY_PATH -u LD_PRELOAD`): `executeCommand()` (all `popen`-based dialogs)
> and `Notification::show()` (`system()`-based). **Any new desktop-tool spawn must wrap
> its command the same way.** Prefer fixing the pollution at the source too (link with
> RPATH `$ORIGIN` instead of exporting `LD_LIBRARY_PATH` in launchers) — this scrub is
> the defense-in-depth net for launches the engine does not control.

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
