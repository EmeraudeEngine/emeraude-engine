---
id: usdz-cannot-be-mapped-on-macos
title: No .usdz opens on macOS — tinyusdz disables mmap because TARGET_OS_IPHONE is defined (as 0)
status: in-progress
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

Owner decision 2026-10-04: **(c) both** — done on Linux the same day:
- ext-deps-generator `patches/tinyusdz.patch` gains a hunk for `src/io-util.cc` testing the VALUE
  (`(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)`, the same for the simulator); `git apply --check` passes on
  the target commit. Takes effect with the next macOS dependency build (and is worth upstreaming).
- `USDZArchive::open()` reads the archive into memory when `MMapFile()` fails, logging why ("no reason given: mmap
  not available in tinyusdz" when empty). Proven on Linux by forcing the failure: texture-cat-plane.usdz imported,
  its image released and reloaded from its archive byte range.

Left: a macOS run opening a .usdz (works with the engine fallback alone; mapped again after the dependency rebuild),
then this file is deleted.

## References

- `src/Scenes/Loaders/USDLoader.cpp` (`USDZArchive::open`), tinyusdz `src/io-util.cc` (the platform guard).
