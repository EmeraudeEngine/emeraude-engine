---
id: memory-outside-the-census
title: Host RAM — most of a scene's private memory is outside the resource census (terrain 8.1 GB for 0.5 GB)
status: open
priority: unranked
scope: Resources census, Graphics (CDLOD terrain, LOD levels, imposters), physics, driver host allocations
opened: 2026-10-04
tags: [memory, diagnostics]
---

# Host RAM — most of a scene's private memory is outside the resource census

## Why

With the CPU-copy release ON (`docs/subsystems/resources/11-cpu-copy-release.md`), the resource census
(`Core.ResourcesManagerService.memoryCensus()`) is ~0.5 GiB, but the process keeps far more: Windows (2026-10-04)
citadel 4.75 GB private bytes for a 467 MiB census, **terrain 8.1 GB for 497 MiB**; Linux citadel RSS 2.5 GB, terrain
4.1 GB; macOS phys_footprint ~9 GB on terrain (which also counts ~4.7 GB of graphics memory on unified memory).

## What remains

1. Split it before deciding anything: heaptrack (Linux) by call site, VMA statistics (the driver-visible part), and
   a per-subsystem count of what lives outside every container: the CDLOD terrain grid and its RT proxy, LOD levels
   (`make_shared` geometries), imposter atlases, the physics' triangle meshes and octrees, loading intermediates kept
   at the allocator's high-water mark (the tree generator's).
2. Then decide per contributor (owner): release, stream, shrink, or accept.

## References

- `docs/subsystems/resources/11-cpu-copy-release.md` (the measurements), `05-development-commands.md`
  (`memoryCensus()`).
