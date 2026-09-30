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

### macOS Helpers (`Helpers.mac.cpp`)

macOS helpers are minimal - Objective-C provides native string handling. NSString conversion is done inline using `stringWithUTF8String:`.

| Function | Purpose |
|----------|---------|
| `pinVulkanLoaderToBundledDriver()` | Sets `VK_DRIVER_FILES` to the bundle's `Contents/Resources/vulkan/icd.d/MoltenVK_icd.json` (unless already set, or absent), so the loader loads that driver only. Called by `PlatformManager` before `glfwInit()`. See [18-macos-vulkan-driver-pinned-to-the-bundle.md](18-macos-vulkan-driver-pinned-to-the-bundle.md) |

---
