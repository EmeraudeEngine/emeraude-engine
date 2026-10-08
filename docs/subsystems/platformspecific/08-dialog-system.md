## Dialog System

### Dialog Classes

| Class | Purpose | Buttons |
|-------|---------|---------|
| `Message` | Standard message dialogs | Preset layouts (`OK`, `OKCancel`, `YesNo`, `Quit`); `setDefaultAnswer()` |
| `CustomMessage` | Custom button dialogs | 1-6 custom labels |
| `OpenFile` | File/folder selection | N/A (system buttons) |
| `SaveFile` | Save location selection | N/A (system buttons) |

### The default button of a `Message` (`setDefaultAnswer()`, since 2026-10-08)

The first button (Yes / OK) is the default one on Windows and macOS: Enter, Space (and on Windows the first button's
accelerator) answer it. On 2026-10-07 a downstream application's crash report question, unparented and unseen behind a terminal on
Windows, caught a stray key press and sent a report: every Windows box is now brought to the foreground (topmost when
not parented). A caller that wants the second button as the default calls `setDefaultAnswer(No)` (or `Cancel`):

| OS | How | Measured |
|---|---|---|
| Windows | `MB_DEFBUTTON2`; every box also gets `MB_SETFOREGROUND`, and `MB_TOPMOST` when it has no parent | 2026-10-08, Windows 11: the unparented crash report question appeared in front of the terminal (owner); `MB_DEFBUTTON2` not exercised |
| macOS | Return moved to the second button (`setKeyEquivalent:`); the alert is already at `CGShieldingWindowLevel` | not yet |
| Linux zenity | `--question --ok-label=Yes --cancel-label=No --default-cancel`; the answer is the exit code (0 = Yes only) | 2026-10-08, Enter → No |
| Linux kdialog | unchanged (kdialog's own default) | — |

⚠️ Owner decision (2026-10-08): Enter keeps meaning **Yes** by default, a downstream application's crash report question included —
it does not call `setDefaultAnswer()`. What protects it from an unseen box is the Windows foreground / topmost flags.

The answer stays safe whatever the default: only the Yes / OK button (exit code 0, `IDYES`, `NSAlertFirstButtonReturn`)
is a Yes; Escape, the close button and a failing tool are No / Cancel.

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
| Custom buttons | Use `--question --switch --extra-button=Label` (`CustomMessage`; `Message` YesNo uses `--ok-label` / `--cancel-label` since 2026-10-08) |

**kdialog Limitations**:
- Maximum 3 buttons for question dialogs (`--yesnocancel`)
- For 4+ buttons, fall back to zenity even on KDE

**Button Index Mapping**:
| Tool | Method | Index Extraction |
|------|--------|------------------|
| zenity (switch mode, `CustomMessage`) | Outputs clicked button text | Match against button labels |
| zenity (`Message`) | Exit code: 0 = OK / Yes, anything else = Cancel / No | Direct mapping |
| kdialog | Exit codes: 0=Yes, 1=No, 2=Cancel | Direct mapping |

> [!IMPORTANT]
> **Spawn desktop tools with a clean dynamic-loader environment.** zenity/kdialog are
> *system* programs and must load *system* libraries. When the host application is
> launched with its bundled library directory pushed into `LD_LIBRARY_PATH` (AppImage
> wrappers, dev launchers, Steam-like parents), that value is inherited by the spawned
> child and a bundled library can shadow its system counterpart. The observed failure:
> a CEF-based consumer bundles CEF's stripped `libvulkan.so.1`, which does **not**
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

**NSAlert has no window title**: `Message` shows its title as the bold message text and the message as the informative
text below it (a message alone when the title is empty). Until 2026-10-08 the title was dropped and the whole message was
bold. `CustomMessage` still shows the message in bold and the title below it as informative text.

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
