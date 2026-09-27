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

> [!NOTE]
> **`Desktop/Commands.windows.cpp` opens files/folders/URLs via `ShellExecuteW()` — never route
> them through a shell (`cmd.exe /c start`).** Shell re-parsing breaks paths containing spaces
> (`start` consumes a quoted path as its window-title argument, silently, with exit code 0) and
> corrupts non-ANSI characters (ANSI code page). `ShellExecuteW(nullptr, L"open", ...)` takes the
> path verbatim as UTF-16 (`convertUTF8ToWide()` from `Helpers.hpp`) and returns a reliable
> status (`> 32` = success). Linux (`xdg-open`, direct argv exec) and macOS (`open`) are not
> concerned by the space issue.

### File Naming Convention

Platform-specific implementations use suffixes:
| Suffix | Platform | Language |
|--------|----------|----------|
| `.linux.cpp` | Linux | C++ |
| `.mac.mm` | macOS | Objective-C++ |
| `.windows.cpp` | Windows | C++ |

CMake selects the appropriate file based on target platform.

---
