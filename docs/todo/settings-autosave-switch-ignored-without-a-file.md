---
id: settings-autosave-switch-ignored-without-a-file
title: --disable-settings-autosave is ignored when the settings file does not exist
status: open
priority: high
scope: Settings (onInitialize)
opened: 2026-10-14
tags: [settings, command-line, defect]
---

# --disable-settings-autosave is ignored when the settings file does not exist

## Why

`Settings::onInitialize()` (`src/Settings.cpp`): when the file is missing it logs "doesn't exist … will be written at
the application successful exit", calls `saveAtExit(true)` and RETURNS — before the `--disable-settings-autosave`
check further down. The switch's help text promises "Disable the generation or the saving of settings files": a run
with the switch on a fresh profile (no `settings.json` yet), or with a `--settings-filepath` that does not resolve,
WRITES a full settings file at exit.

Seen twice on 2026-10-14: the Windows peer (a mangled `--settings-filepath`: 3 runs wrote 3 settings files of 6 026
bytes into the Release folder) and Linux (a relative `--settings-filepath` resolved from the binary's folder: 3 files
of 6 945 bytes written there). Every AI test run relies on that switch; on a machine without a settings file yet, it
would create the owner's.

## What remains

- Test the switch before the missing-file branch (no `saveAtExit(true)` under it); keep the warning, saying the file
  will NOT be written. Decide with the owner whether a missing `--settings-filepath` target should rather be refused
  (an explicit path that does not exist is likely a mistake).
- Proof: a run with the switch and a non-existent path writes nothing; without the switch the file is still created.

## References

- Windows peer report 2026-10-14; `Core.cpp` argument help (`--disable-settings-autosave`).
