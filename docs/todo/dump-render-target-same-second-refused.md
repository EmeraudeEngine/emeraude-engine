---
id: dump-render-target-same-second-refused
title: A second dumpRenderTarget of the same layer within one second is refused
status: open
priority: high
scope: Scenes/Manager.console (dumpRenderTarget: the file name)
opened: 2026-11-15
tags: [console, render-target, capture]
---

# A second dumpRenderTarget of the same layer within one second is refused

## Why

The dump's files are named `rt-dump-<seconds since the epoch>-layer<N>` (`Manager.console.cpp`). Two dumps of the
same layer in the same second get the same name, and the second one fails: `PixelFactory::FileIO::write(), the file
'…/rt-dump-1794700000-layer5.png' already exists !` (an `[Error]`; nothing is overwritten). Measured 2026-11-15 on
Linux: 4 of 10 dumps sent about 0.7 s apart were refused.

Nothing is lost or corrupted, but a series of reads — the way a target is measured over time — loses samples in
silence for a script which only reads the path out of the reply.

## What remains

- [ ] Give the dump a unique stem, the way a screenshot has one since 2026-09-24 (a second already used moves to the
  next free one: graphics `10-9b-frame-capture-screenshot-and-temporal-capture.md` § "The stem is UNIQUE") — reuse
  that mechanism, do not write a second one.
- [ ] Re-test: ten dumps of one layer 0.3 s apart give ten files and ten distinct paths in the replies.

## References

- `src/Scenes/Manager.console.cpp` (`dumpRenderTarget`, the base name of its files).
- `docs/render-targets.md` (the trap is noted there).
