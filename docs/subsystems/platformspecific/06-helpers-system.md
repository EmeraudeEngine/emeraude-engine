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
| `executeCommand(cmd, exitCode)` | Runs `/bin/sh -c` (through `cleanLoaderEnvCommand`) with `runProcessCapturingOutput()` (never `popen()`: the child gets its standard streams only, caution-points § A child process inherits NOTHING), returns stdout |
| `executeCommandPumpingEvents(cmd, exitCode, pump)` | Same, but runs the child on a worker thread and calls `pump` on the caller's thread until it exits. **Every interactive dialog must use this one** — see § Linux below |
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
| `getStringValueFromHKLM(...)` | Reads a `REG_SZ` value under HKLM. Returns `std::optional< std::wstring >`, `noexcept` — a failure is an empty optional plus the Windows error code on `std::cerr`, never a throw (the engine is built `/EHs- /EHc-`; it used to `throw std::runtime_error`, caught in `SystemInfo::fetchOSInformation()`, until 2026-09-08) |
| `createConsole(title)` | Creates debug console window |
| `attachToParentConsole()` | Attaches to parent console |

### macOS Helpers (`Helpers.mac.cpp`, `AppKitDefaults.mac.mm`, `ProcessActivity.mac.mm`)

No string helper here: Objective-C provides native string handling, and an NSString conversion is done inline with `stringWithUTF8String:`. An Objective-C++ helper (`.mm`) is compiled with ARC and typed into the explicit macOS list of `cmake/PrepareEngineSourceFiles.cmake`.

| Function | Purpose |
|----------|---------|
| `pinVulkanLoaderToBundledDriver()` | Sets `VK_DRIVER_FILES` to the bundle's `Contents/Resources/vulkan/icd.d/MoltenVK_icd.json` (unless already set, or absent), so the loader loads that driver only. Takes the resources directory (`FileSystem::resourcesDirectory()`). Called by `PlatformManager` before `glfwInit()`. See [18-macos-vulkan-driver-pinned-to-the-bundle.md](18-macos-vulkan-driver-pinned-to-the-bundle.md) |
| `disableWindowStateRestoration()` | Registers `ApplePersistenceIgnoreState = YES` in the volatile registration domain (`AppKitDefaults.mac.mm`), before `glfwInit()`: no "reopen the windows?" modal after a crash. `docs/caution-points.md` § *macOS: AppKit's window state restoration is off* |
| `wakeDisplays(timeout)` | Wakes the displays when EVERY online display sleeps, before `glfwInit()` (GLFW skips a sleeping display). `docs/caution-points.md` § *macOS: a sleeping display is woken to start* |
| `DisplaySleepInhibitor` (macOS, Windows) | RAII token: the display stays awake while held (`Window`, setting `Core/Video/Window/PreventDisplaySleep`). Same section |
| `ProcessActivity` | RAII token: the process is declared ACTIVE to macOS while held (`NSProcessInfo beginActivityWithOptions:reason:`, user initiated + latency critical, the system's idle sleep still allowed; `ProcessActivity.mac.mm`). Held by `PlatformManager` from `onInitialize()` (before `glfwInit()`) to `onTerminate()`, in EVERY run: without it App Nap demotes a process that shows no window 30 s after its launch. `docs/caution-points.md` § *macOS: App Nap demoted a window-less process after 30 s* |
| `setCurrentThreadTimeConstraint(period, computation)` | Puts the calling thread under the Mach time-constraint policy (the logic thread, on itself). `docs/caution-points.md` § *macOS coalesces every non-real-time timed wait* |

---
