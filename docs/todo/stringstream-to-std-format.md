---
id: stringstream-to-std-format
title: Replace std::stringstream by std::format for simple key/name building
status: open
priority: low
scope: cascade-wide
opened: unknown
tags: [cpp, c++20]
---

# Replace std::stringstream by std::format for simple key/name building

## What remains

- [ ] Replace `std::stringstream` with `std::format` (C++20) where it only builds a simple key,
  name or identifier.

## ⚠️ Traps

- The old trap "`std::format` does not work on macOS with older SDKs" is OBSOLETE. Shared engine code has used
  `std::format` since 2025-09 (`SwapChain.cpp` ~592, `RendererFrameScope.cpp` ~68) at the macOS 12.0 deployment
  target, and it builds on the three OS. Any conversion must still keep that floor buildable.

## Origin

Inherited from the historical root `TODO.md` (no date).
