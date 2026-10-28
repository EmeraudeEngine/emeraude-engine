---
id: shared-material-buffer-capacity
title: An asset with more materials than the shared uniform buffer holds loses its meshes
status: open
priority: high
scope: Graphics/SharedUniformBuffer, Graphics/Material
opened: 2026-10-01
tags: [graphics, materials, capacity, measured]
---

# An asset with more materials than the shared uniform buffer holds loses its meshes

## Why

This came out of the triad 12 corpus run (2026-10-01): 349 glTF files opened one after another with
`Core.openFiles()`. The Khronos sample `NodePerformanceTest` has 10 000 meshes, each with its own material. The
shared material uniform buffer is full at **7 232 elements** (64 banks of 113, `SharedUniformBuffer::MaxBankCount`,
4 MiB) and "cannot grow any further". In that run, 5 538 of its materials failed to load, so their meshes left the
scene. The materials of the assets opened earlier in the session were still in the resource cache and held part of the
slots.

The error is explicit and handled: the scene keeps running. But one large asset, or a long session that loads many
assets, quietly loses geometry.

**2026-10-28:** `MaxBankCount` was raised to 256 (owner, for `city` at side 128's 7508 spot lights): the shared
buffers now hold four times as many elements (16 MiB, 256 banks of 113: 28 928 materials). The
`NodePerformanceTest` measurement below has not been run again: its 10 000 materials should now fit, the cached
materials of a long session still hold their seats.

## What remains

- [ ] Measure `NodePerformanceTest` alone in a fresh instance: how many of its 10 000 materials fail.
- [ ] Decide with the owner:
  - a larger or unbounded buffer (more banks, or a buffer per bank count; `MaxBankCount` is load-bearing: see its
    note);
  - sharing identical materials (10 000 copies of the same PBR values could be one slot);
  - releasing the slots of cached materials that no scene uses any more;
  - a storage buffer instead of a uniform buffer for the material table.
- [ ] Re-test: `NodePerformanceTest` complete, 0 "shared uniform buffer is full".

## References

- `src/Graphics/SharedUniformBuffer.hpp:283` (`MaxBankCount{256}`, 64 until 2026-10-28), `src/Graphics/SharedUniformBuffer.cpp:192` (the
  bank-count check) and `:240` (the "cannot grow any further" error).
- Engine commit `050081a3` (2026-10-01), which opened this item with the corpus-run measurement above; the triad
  report holds no record of that run (its § 12 is `src/Animations`).
