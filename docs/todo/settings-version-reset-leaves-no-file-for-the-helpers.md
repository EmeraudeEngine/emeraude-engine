---
id: settings-version-reset-leaves-no-file-for-the-helpers
title: The settings version guard renames the file and writes the fresh one only at exit; helpers and --disable-settings-autosave break on it
status: open
priority: high
scope: engine Core (settings version guard), Settings (a file named on the command line)
opened: 2026-10-15
tags: [settings, startup, cef-helpers, robustness]
---

# The settings version guard renames the file and writes the fresh one only at exit; helpers and --disable-settings-autosave break on it

## Why

Found by the Windows peer on 2026-10-15, testing a downstream application whose settings file was stamped by an older
engine (WrittenByEngineVersion 1.0.0, engine 1.0.1). It cost two failed runs.

- The version guard (`Core.cpp`, the settings reset, ~line 3530-3600) RENAMES the settings file to
  `<file>.<time>-bck` and clears the store. The fresh file is written only at save-at-exit. Until then no file exists
  at that path.
- With `--disable-settings-autosave`, the fresh file is never written at all. Yet the rename happens anyway: the run
  modifies the settings on disk, although that flag means it must not.
- Every CEF helper process receives the same `--settings-filepath`. It finds no file and stops
  (`Settings.cpp:107`: "The settings file … named on the command line doesn't exist: the engine stops."), then
  "Shared data manager initialization error!".
- Measured result: every render process crashed, the JavaScript application never started ('timeout'), and the
  application showed its startup-error overlay.
- In that state `Core.shutdown()` did not finish within 180 s (the run was killed). In a second run, a shutdown sent
  during startup was "Executed" but the application kept running. Whether that is the engine's or the downstream
  application's is open (see What remains).
- Production (no `--settings-filepath`): the helpers read the default path. A missing default file means defaults, not
  a stop, so this exact cascade looks harness-only. The gap remains, though: during the first run after an engine
  update, the helpers run on defaults, not on the restored entries. To confirm.

## What remains

- [ ] Owner decision on the fix, among (or combining):
  - (a) write the fresh settings file at once after a reset (the restored entries included), so the path never stays
    empty;
  - (b) under `--disable-settings-autosave`, reset IN MEMORY only: no rename, the file on disk untouched (the flag's
    promise);
  - (c) a helper process never stops on a missing named file: it falls back to the defaults, or to the `-bck` backup.
- [ ] The shutdown that does not finish in the startup-error state: reproduce and attribute (engine `Core` vs the
  downstream application's startup-error path).
- [ ] Test: an old-stamped settings copy + `--settings-filepath` + `--disable-settings-autosave`, then the same
  without the flags. Check the helpers, the file on disk, and the exit.

## ⚠️ Traps

- A test harness that copies the user's settings must stamp the current WrittenByEngineVersion, or the guard resets the
  copy (the Windows peer's workaround).

## References

- `src/Core.cpp` (settings version guard), `src/Settings.cpp` (named file missing → fatal, owner decision 2026-10-14).
