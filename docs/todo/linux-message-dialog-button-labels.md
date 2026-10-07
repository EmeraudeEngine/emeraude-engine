---
id: linux-message-dialog-button-labels
title: The Linux message dialog shows "Yes" / "No" in English whatever the language of its text
status: open
priority: unranked
scope: PlatformSpecific/Desktop/Dialog/Message.linux.cpp
opened: 2026-10-07
tags: [dialog, i18n, linux]
---

# The Linux message dialog shows "Yes" / "No" in English whatever the language of its text

## Why

`Message.linux.cpp` builds the zenity YesNo dialog with `--switch --extra-button=No --extra-button=Yes` and reads the
answer by comparing zenity's output with the literal `"Yes"`. The buttons are therefore always English: app_system's
crash report question, translated since 2026-10-07, shows a French text with "No" / "Yes" buttons (Windows `MessageBox`
and macOS `NSAlert` follow the OS language). The answer parsing is safe — anything but `"Yes"` (No, Escape, the close
button, a failing zenity) is No — and must stay so.

## What remains

1. Let the caller give the button labels (or localize them), keeping the answer mapped on the button identity, never
   on a translated text a user could not type.
2. kdialog (KDE) uses `--yesno`, whose buttons follow the desktop language: check it is unaffected.
3. Document it in `docs/subsystems/platformspecific/08-dialog-system.md`, then delete this item.

## References

- app_system `docs/crash-report.md` § The question; `src/CrashReportTexts.cpp`.
