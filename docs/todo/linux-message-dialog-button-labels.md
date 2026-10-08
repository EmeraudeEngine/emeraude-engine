---
id: linux-message-dialog-button-labels
title: The Linux and macOS message dialogs show "Yes" / "No" in English whatever the language of their text
status: open
priority: unranked
scope: PlatformSpecific/Desktop/Dialog/Message.linux.cpp, Message.mac.mm
opened: 2026-10-07
tags: [dialog, i18n, linux]
---

# The Linux and macOS message dialogs show "Yes" / "No" in English whatever the language of their text

## Why

`Message.linux.cpp` builds the zenity YesNo dialog with `--ok-label=Yes --cancel-label=No` (since 2026-10-08; before,
`--switch --extra-button=No --extra-button=Yes`), and `Message.mac.mm` adds `@"Yes"` / `@"No"` NSAlert buttons. The
buttons are therefore always English on Linux AND macOS: app_system's crash report question, translated since
2026-10-07, shows a French text with "No" / "Yes" buttons; only Windows `MessageBox` follows the OS language ("Oui" /
"Non"). The answer is read from the button identity (zenity's exit code, NSAlert's return code) — it must stay so: a
localized label must never be what the answer is compared with.

## What remains

1. Let the caller give the button labels (or localize them), keeping the answer mapped on the button identity, never
   on a translated text a user could not type.
2. kdialog (KDE) uses `--yesno`, whose buttons follow the desktop language: check it is unaffected.
3. Document it in `docs/subsystems/platformspecific/08-dialog-system.md`, then delete this item.

## References

- app_system `docs/crash-report.md` § The question; `src/CrashReportTexts.cpp`.
