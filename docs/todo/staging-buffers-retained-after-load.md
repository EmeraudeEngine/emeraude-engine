---
id: staging-buffers-retained-after-load
title: Staging buffers stay allocated after a load — 385 MiB on WorldLobby (macOS)
status: open
priority: unranked
scope: Vulkan/TransferManager
opened: 2026-10-04
tags: [memory, vulkan, transfer]
---

# Staging buffers stay allocated after a load

## Why

First use of `Core.RendererService.getGPUMemory()` on macOS (M2, 2026-10-04): WorldLobby, 17 s after its load,
still holds **62 TRANSFER_SRC-only buffers, 384.8 MiB, up to 64 MiB each** — the shape of upload staging buffers
kept alive. On unified memory that is RAM taken from the 12 GB budget; on a discrete GPU it is host-visible memory.
Whether they are a pool kept on purpose or forgotten is not known.

Windows (RTX 3060 Laptop, 2026-10-04): the same shape, **6 HOST TRANSFER_SRC buffers, 156.8 MiB, the largest 64 MiB**
(= the largest vertex buffer), ~15 s after WorldLobby's load. Also noted there: the BLAS storage (`DEVICE_ADDRESS|
AS_STORAGE`, 670.8 MiB) is as large as the vertex buffers it is built from (667.7 MiB) — expected for uncompacted
BLAS; compaction (VK_KHR_acceleration_structure compaction queries) would be its own item.

## What remains

- Find who owns them (`Vulkan/TransferManager`, the staging pool): intended reuse, or a leak after the uploads.
- If a pool: a bound, and a shrink when idle (after the load, after N seconds without uploads).
- Measure on Linux too (`getGPUMemory`, group `BUFFER TRANSFER_SRC`), before and after.

## References

- `docs/subsystems/graphics/37-gpu-memory-budget-and-report.md` — how the report reads.
