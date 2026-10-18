---
id: readers-through-io-open-input-file
title: The engine's own file readers open a FIFO path without checking it, and block
status: open
priority: high
scope: src/Audio/PlaylistResource.cpp, src/Audio/SoundfontResource.cpp, src/Graphics/TextureCache.cpp, src/Scenes/Loaders/WADLoader.cpp, src/Resources/SharingServer.cpp
opened: 2026-10-18
tags: [io, robustness, defect]
---

# The engine's own file readers open a FIFO path without checking it, and block

## Why
A FIFO without a writer blocks `std::ifstream::open()` forever: a path that comes from data (a store, a scene, a
dropped file, a peer) can freeze a loading thread. emeraude-base routes every one of its readers through
`IO::openInputFile()` since 2026-10-18 (base `caution-points.md` § the readers refuse what is not a regular file);
the engine opens some files with its own `std::ifstream`.

## What remains
- Route through `Base::IO::openInputFile()` (or `IO::fileGetContents()` / `IO::FileStream`): `PlaylistResource`,
  `SoundfontResource`, `TextureCache`, `WADLoader`, `SharingServer`.
- Exempt (fixed system paths, never data): `SystemInfo.linux.cpp`, `StorageInfo.linux.cpp`, `SerialPort.linux.cpp`
  (/proc, /sys) — say so at each site.
- Audit the third-party libraries that open a path themselves (a scene / mesh loader handed a path instead of a
  buffer): each gets `Base::IO::isReadableRegularFile()` before the call.
- A test per reader with base's FIFO harness (`test_IO.cpp`, `FifoForOneTest`).

## References
- emeraude-base `src/IO/IO.hpp` `isReadableRegularFile()`, `openInputFile()`.
