---
id: usdz-cannot-be-mapped-on-macos
title: No .usdz opens on macOS — tinyusdz disables mmap because TARGET_OS_IPHONE is defined (as 0)
status: open
priority: unranked
scope: Scenes/Loaders/USDLoader (USDZArchive::open), tinyusdz (ext-deps-generator)
opened: 2026-10-04
tags: [usd, macos, dependencies]
---

# No .usdz opens on macOS — tinyusdz disables mmap because TARGET_OS_IPHONE is defined (as 0)

## Why

Found by the macOS session 2026-10-04 (base c8073d5, engine 923d552f), opening tinyusdz's
`models/texture-cat-plane.usdz` with `Core.openFiles()`: "[Error][USDLoader] Unable to map the archive '…' : " (an
EMPTY reason), then "[Error][ModelViewer] Unable to import the file". `USDZArchive::open()`
(`src/Scenes/Loaders/USDLoader.cpp`, `tinyusdz::io::MMapFile`) fails for EVERY .usdz on macOS.

Root cause, tinyusdz `src/io-util.cc`: `#if defined(TINYUSDZ_BUILD_IOS) || defined(TARGET_OS_IPHONE) ||
defined(TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__) || …` selects the "non posix" branch. Apple's
`TargetConditionals.h` DEFINES `TARGET_OS_IPHONE` on macOS with the VALUE 0 (verified: `clang++ -E` after `<cstdlib>`
/ `<string>`), so `TINYUSDZ_MMAP_SUPPORTED` is 0 and `MMapFile()` returns false without a message. Pre-existing,
unrelated to the CPU-copy work, but it blocks its USDZ path on macOS (the entry byte range, `model-embedded-image-sources`).

## What remains

Owner decision, then the fix, then a macOS run opening a .usdz:
- (a) patch tinyusdz in ext-deps-generator to test the VALUE (`defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE`, the
  same for `TARGET_IPHONE_SIMULATOR`); worth upstreaming;
- (b) in `USDZArchive::open()`, read the archive into memory when `MMapFile()` fails;
- (c) both.

## References

- `src/Scenes/Loaders/USDLoader.cpp` (`USDZArchive::open`), tinyusdz `src/io-util.cc` (the platform guard).
