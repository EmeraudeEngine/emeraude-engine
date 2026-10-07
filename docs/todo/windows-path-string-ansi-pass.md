---
id: windows-path-string-ansi-pass
title: path::string() is ANSI on Windows, and throws outside the code page (about 120 sites)
status: open
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

- [ ] Replace every `path.string()` whose result leaves the process (a log line, a console / MCP output, JSON, a URL,
  a C API taking UTF-8) with `Base::IO::toU8String()` (or `toGenericU8String()`). Where a Windows API is called, use the
  wide API with `path.c_str()` instead. Also check the `<<` of a path (it streams `string()` quoted).
- [ ] Same in projet-alpha (`/usr/bin/grep -rn '\.string()' src`).
- [ ] Re-test on Windows from a non-ASCII install directory (as the triad 14 peer test did): the menu loads, every
  console JSON is valid UTF-8, the logs read correctly.

## ⚠️ Traps

- `IO::u8path()` / `toU8String()` no longer throw on Windows since triad 15: they go through the base's strict
  `String::utf8ToUTF16()` / `utf16ToUTF8()`, and an invalid sequence becomes U+FFFD.
- `/usr/bin/grep` sees the engine; the default `grep` (ugrep) does not.

## References

- Engine `docs/caution-points.md` § "On MSVC, std::filesystem::path::string() is ANSI".
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 14 (the peer measurement) and § 15.
