## User preferred language (`UserInfo::preferredLanguage()`)

**Files**: `UserInfo.hpp`, `UserInfo.{linux,windows,mac}.cpp` (since 2026-10-07)

The language of the user's interface, as a **BCP 47 tag** (`fr-BE`, `zh-Hant-TW`, `en`), read once when the
`UserInfoService` initializes — a primary service, so it is known before any window, web view or CEF. Empty when
unknown. First consumer: a downstream application's crash report question, asked before the JavaScript i18n exists.

| OS | Source | Example |
|---|---|---|
| Linux | `LANGUAGE` (first entry of the `:` list), else `LC_ALL`, `LC_MESSAGES`, `LANG` — gettext's order; encoding and modifier dropped, `_` turned into `-`; `C` / `POSIX` say nothing | `LANGUAGE=fr_BE:fr` → `fr-BE` |
| Windows | `GetUserPreferredUILanguages(MUI_LANGUAGE_NAME)` (first entry), else `GetUserDefaultLocaleName()` | `fr-BE` |
| macOS | `CFLocaleCopyPreferredLanguages()` (first entry, System Settings order) | `fr-BE` |

- It is the **interface** language, not the formatting locale: never feed it to `setlocale()` (the engine keeps the C
  numeric locale, `Base::Locale::enforceNumericC()`).
- Measured on 2026-10-07 (Linux, `LANG=fr_BE.UTF-8`, `LANGUAGE=fr_BE:fr`): `fr-BE`. Windows and macOS: compiled and run
  by their peers before trusting it.
- ⚠️ The Linux `Dialog::Message` YesNo buttons are the literal `Yes` / `No` (zenity `--extra-button`), whatever the
  language of the text: engine item `linux-message-dialog-button-labels`.
