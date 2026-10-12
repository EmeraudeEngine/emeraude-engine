---
id: windows-path-string-ansi-pass
title: path::string() is ANSI on Windows, and throws outside the code page (about 120 sites)
status: in-progress
priority: high
scope: the whole engine (log lines, console / MCP outputs, paths handed to APIs); projet-alpha
opened: 2026-10-01
tags: [windows, unicode, paths, cascade]
---

# path::string() is ANSI on Windows, and throws outside the code page (about 120 sites)

## Why

On MSVC, `std::filesystem::path::string()` converts the UTF-16 path to the ANSI code page:

- a character outside the code page throws `std::system_error`, which is an abort with exceptions off;
- a character inside it (`é`) becomes an ANSI byte, which is invalid UTF-8 everywhere the engine expects UTF-8: JSON,
  the console / MCP protocol, CEF URLs, the logs.

Measured by the Windows peer (triad 14, 2026-10-01) with a copy of Release in a folder named `Jérôme`:
- the CEF menu did not load (its `file://` URL carried U+FFFD);
- `FileSystem.getJson()` emitted CP-1252 bytes, which the console client rejected.

Triad 14 fixed the shell paths (`PlatformSpecific/Desktop/Commands.cpp`, the dialogs). Triad 15 fixed both measured
sites and the other startup-critical ones:
- the binary name in `FileSystem.cpp`;
- `FileSystem.console.cpp`;
- the `--enable-log` argument in `Tracer.cpp`;
- the menu URL in `projet-alpha/src/UI/Manager.cpp`.

124 `.string()` calls in 30 engine files remain. The owner chose a dedicated pass over a change inside section 15.

## What remains

The owner chose **rule C** (2026-10-12): the code is correct without any manifest, AND the applications declare the
UTF-8 active code page (engine `docs/caution-points.md` § "On MSVC, `std::filesystem::path::string()` is ANSI").

- [x] Step 1, the hard aborts (Windows peer, 2026-10-12): `BaseInformation.cpp` and its `dataString()` consumers,
  `Vulkan/Loader.cpp`, `ResourceTrait::getResourceNameFromFilepath()`, `FileSystem`'s `operator<<`; projet-alpha
  `UI/WebView.cpp` and `Boot/windows/main.cpp`.
- [x] Step 2, the manifest (Windows peer, 2026-10-12): compatibility + `longPathAware` + `<activeCodePage>UTF-8</activeCodePage>`
  in projet-alpha.exe and projet-alpha-helper.exe (which had none), and the `PlatformManager` startup warning when
  `PlatformSpecific::getActiveCodePage() != CP_UTF8`.
- [ ] Step 3, the per-site pass, verified with an executable WITHOUT the manifest, in groups: inbound paths at trust
  boundaries (`loadGLTF`, glTF / FBX URIs); data keys and C APIs; logs and console (including the `<<` of a path, which
  streams `string()` quoted); extension compares. Every `path.string()` whose result leaves the process goes through
  `Base::IO::toU8String()` (or `toGenericU8String()`); a Windows API takes the wide string. Same in projet-alpha
  (`/usr/bin/grep -rn '\.string()' src`). Then a guard tool, proposed to the owner before it is written.
- [ ] Re-test on Windows from a non-ASCII install directory (as the triad 14 peer test did): the menu loads, every
  console JSON is valid UTF-8, the logs read correctly.

## ⚠️ Traps

- `IO::u8path()` / `toU8String()` no longer throw on Windows since triad 15: they go through the base's strict
  `String::utf8ToUTF16()` / `utf16ToUTF8()`, and an invalid sequence becomes U+FFFD.
- `/usr/bin/grep` sees the engine; the default `grep` (ugrep) does not.
- Fixing an abort uncovers the next one on the same path: under a non-ASCII install directory, `BaseInformation.cpp`,
  then `Vulkan/Loader.cpp`, then `ResourceTrait.cpp` aborted in turn. Re-run each repro until it exits 0, from the
  from-start debugger on a Debug copy (the stack names the site).
- The shipped Vulkan loader lists no layer under a non-ASCII install directory: a repro from such a directory runs
  without the validation layer unless the UTF-8 code page is active (check the `Insert instance layer` line).

## References

- Engine `docs/caution-points.md` § "On MSVC, std::filesystem::path::string() is ANSI".
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 14 (the peer measurement) and § 15.
