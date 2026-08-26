---
id: stringstream-to-std-format
title: Replace std::stringstream by std::format for simple key/name building
status: open
priority: unranked
scope: cascade-wide
opened: unknown
tags: [cpp, c++20]
---

# Replace std::stringstream by std::format for simple key/name building

## What remains

- [ ] Replace `std::stringstream` with `std::format` (C++20) where it only builds a simple key,
  name or identifier.

## ⚠️ Traps

- **This does NOT work on macOS when targeting older SDKs** — `std::format` landed late in libc++.
  The cascade is strict cross-platform, so any conversion must keep the macOS floor buildable.

## Origin

Inherited from the historical root `TODO.md` (no date).
