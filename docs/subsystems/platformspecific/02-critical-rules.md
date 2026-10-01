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

## Triad 14 rules (2026-10-01)

- **Every engine `.mm` is compiled with ARC** (`-fobjc-arc`, set on each `.mm` by `cmake/PrepareEngineSourceFiles.cmake`).
  Each file refuses to build without it (`#if !__has_feature(objc_arc) #error`). Without ARC, the ARC-style code (no
  `release` anywhere) leaked every alert, notification and capture session.
- **NSString ↔ std::string through `PlatformSpecific/StringConversion.mac.hpp`**: `toNSString()` (never nil — nil
  inserted in a collection or given to `-fileURLWithPath:` raises an Objective-C exception, an abort) and
  `toStdString()` (never a NULL `UTF8String` into `std::string`).
- **No shell for a user string on macOS**: `runDefaultDesktopApplication()` runs `open` through an argv (`reproc`), as
  on Linux; it used `system("open \"" + argument + "\"")`. The Linux dialogs and notifications still build a shell
  command line: every user string goes through `escapeShellArg()` (one implementation, in `Helpers.linux.cpp`).
- **Windows strings**: a path for the shell goes through `Base::IO::toU8String()` (on MSVC, `path::string()` converts to
  the ANSI code page and throws outside it, an abort); the UTF-16 ↔ multibyte converters use the same explicit length
  for the size query and the conversion, refuse an input past INT_MAX and keep only what was written; one quoted
  argument for `ShellExecuteW` goes through `quoteArgument()` (backslashes before a quote doubled).
- **Camera permission (macOS)**: `VideoCaptureDevice::open()` checks `authorizationStatusForMediaType:` — denied or
  restricted is refused, undetermined asks the user and fails this attempt. The application's Info.plist must carry
  `NSCameraUsageDescription` (projet-alpha's does since 2026-10-01, with `NSMicrophoneUsageDescription`).
- **`openURL()` opens web links only** (`http://`, `https://`, owner ruling): the raw string goes to the system, which
  would launch any registered protocol handler (`file://host/share/x.exe`, the Windows `ms-*` handlers).
