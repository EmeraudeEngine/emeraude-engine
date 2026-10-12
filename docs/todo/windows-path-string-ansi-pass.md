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
- [x] Step 3, the per-site pass (Windows peer, 2026-10-12), verified on executables WITHOUT the UTF-8 code page: every
  site of base, engine and projet-alpha (inbound paths at trust boundaries, data keys and C APIs, logs and console,
  extension compares), the glTF uri pre-check (fastgltf aborted on `%FF.bin`), the console / MCP U+FFFD reply guard,
  emeraude-base `BlobTrait`'s path overload and `String::isValidUTF8()`. The convention: `docs/cpp-conventions.md`
  § "Paths and Text Encoding".
- [x] The guard: `tools/check-path-encoding.py` (owner decision 2026-10-12: clang-query authority, grep quick net,
  allowlist with reasons, a documented pre-push step).
- [x] Linux (2026-10-12): GCC builds the cascade, `clang -fsyntax-only` passes the 60 touched translation units, the
  base suite passes in Release and under ASan/UBSan (2508 tests, the 12 new ones included), a 37-demo sweep with the
  validation layer gives 0 VUID. The guard on the Linux Release + Debug databases (625 translation units) first gave 27
  findings that the Windows databases could not see: 21 in POSIX-only files (the clang layer now skips them, like the
  grep gate) and 6 legitimate sites (a POSIX branch of `TrustStore.cpp`, wide strings and path copies in Windows files
  the grep gate cannot type), now in the allowlist with their reasons: 0 finding, 10 allowlisted.
- [x] macOS (2026-10-12, Mac mini M6): Apple clang builds the pass with 0 warning; the guard (Homebrew llvm's
  clang-query) on the Release + Debug databases gives 0 finding, 7 allowlisted (the 2 `TrustStore.cpp` POSIX-branch
  entries are Linux-only, and macOS compiles `test_BlobTrait.cpp`, so its clang layer sees the wide string).
- [ ] Offer the fastgltf patch upstream (the owner's act; the patch and the issue text exist).
- [x] Re-test on Windows from a non-ASCII install directory (lot 3 binaries, 2026-10-12, `activeCodePage` stripped from
  both executables, `Ωmega-日本` and `Jérôme` installs): exit 0 with 0 error; the Ω glTF and BoxAccents load, the
  hostile glTFs are refused (2 errors); the TCP and MCP replies are valid UTF-8 (U+FFFD plus a warning naming the
  command). The CEF main menu (after `Stage.openMenu()`) is pixel-identical to an ASCII-install control run, in the
  engine screenshot and in the window grab (0 pixel differs). The logs, written under non-ASCII file names, decode as
  strict UTF-8 (0 U+FFFD) and print the install path literally; the CEF URL carries it percent-encoded as UTF-8. The
  guard on the MSVC Release + Debug databases gives 0 finding, 4 allowlisted. Known: the validation layer does not load
  from a non-ASCII install without the code page (the shipped loader, see Traps).

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
