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

- `src/Graphics/SharedUniformBuffer.hpp` (`MaxBankCount`), `.cpp` (the "cannot grow any further" error).
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 12 (the corpus run).
