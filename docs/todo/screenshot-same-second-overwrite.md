---
id: screenshot-same-second-overwrite
title: Screenshots taken within the same second were reported overwritten on macOS and Windows (to verify)
status: open
priority: high
scope: Graphics (frame capture / screenshot)
opened: 2026-10-31
tags: [screenshot, capture, macos, windows]
---

# Screenshots taken within the same second were reported overwritten on macOS and Windows (to verify)

## Why

`docs/subsystems/graphics/10-9b-frame-capture-screenshot-and-temporal-capture.md` says the stem has been UNIQUE
since 2026-09-24 (a second already used moves to the next free one). Yet in late October 2026 the macOS peer
(a "noise series" of several captures read a standard deviation of 0) and the Windows peer (two shots less than 1 s
apart) both reported that the second capture replaced the first. A defect if confirmed, hence `high`.

## What remains

- Verify on each OS: two `screenshot()` 0.3 s apart, read the two paths the commands ANSWER, compare the files.
- Distinct paths and files → the engine is right; the peers' scripts rebuilt the name from the clock. Delete this
  item and say so in the measurement skills ("read the path the command answers, never rebuild it").
- Same path or same content → the uniqueness fix regressed on that OS: find why.

## References

- graphics `10-9b-frame-capture-screenshot-and-temporal-capture.md` § "The stem is UNIQUE"
