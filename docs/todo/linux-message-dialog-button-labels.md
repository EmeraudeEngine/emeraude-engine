---
id: linux-message-dialog-button-labels
title: The Linux and macOS message dialogs show English button labels whatever the language of their text
status: open
priority: unranked
scope: PlatformSpecific/Desktop/Dialog/Message.linux.cpp, Message.mac.mm
opened: 2026-10-07
tags: [dialog, i18n, linux, macos]
---

# The Linux and macOS message dialogs show English button labels whatever the language of their text

## Why

Every preset button label is a hard-coded English string outside Windows:

- zenity: OKCancel `--question --cancel-label=Cancel --ok-label=OK` (`src/PlatformSpecific/Desktop/Dialog/Message.linux.cpp:156`),
  YesNo `--question --ok-label=Yes --cancel-label=No` (`Message.linux.cpp:167`, since 2026-10-08; before,
  `--switch --extra-button=No --extra-button=Yes`).
- kdialog: OKCancel forces `--yes-label OK --no-label Cancel` on its `--yesno` / `--warningyesno` dialog
  (`Message.linux.cpp:122-126`); only kdialog's YesNo (`Message.linux.cpp:101` / `:112`) keeps the desktop language.
- macOS NSAlert: `@"OK"`, `@"Cancel"`, `@"Yes"`, `@"No"`, `@"Quit"` (`src/PlatformSpecific/Desktop/Dialog/Message.mac.mm:86-100`).

A downstream application's crash report question, translated since 2026-10-07, therefore shows a French text with
"No" / "Yes" buttons; only Windows `MessageBox` follows the OS language ("Oui" / "Non"). The answer is read from the
button identity (zenity's / kdialog's exit code, NSAlert's return code) — it must stay so: a localized label must never
be what the answer is compared with.

## What remains

1. Let the caller give the button labels (or localize them) for every preset layout (OK, OKCancel, YesNo, Quit),
   keeping the answer mapped on the button identity, never on a translated text a user could not type.
2. kdialog (KDE): YesNo follows the desktop language, but OKCancel forces the English `OK` / `Cancel` labels — it
   is in the same fix.
3. Document it in `docs/subsystems/platformspecific/08-dialog-system.md`, then delete this item.

## References

- The crash-report question of a downstream application.
