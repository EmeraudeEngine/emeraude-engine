---
id: download-image-leaks-a-transfer-operation
title: TransferManager::downloadImage() leaves its image transfer operation reserved forever
status: open
priority: unranked
scope: Vulkan/TransferManager (downloadImage), Vulkan/ImageTransferOperation
opened: 2026-10-04
tags: [vulkan, memory, transfer]
---

# TransferManager::downloadImage() leaves its image transfer operation reserved forever

## Why

`downloadImage()` takes a staging buffer with `getAndReserveImageTransferOperation()`, whose
`setRequestedForTransfer()` RESETS the operation's fence; an operation is available again only when that fence
signals (`isAvailable()`). The download submits with its OWN fence (`downloadFence`) and never signals the
operation's: the operation — and its staging buffer, sized to the image — stays reserved for the engine's life, and
the next transfer allocates a new one. Every call leaks one: `ImposterAtlas` bakes, `SceneRenderTarget` captures,
the console's texture download (`Scenes/Manager.console.cpp`). Found 2026-10-04 while writing
`TransferManager::downloadBuffer()` (which uses a temporary buffer of its own for exactly this reason).

## What remains

1. Give `downloadImage()` a temporary host-readable staging buffer like `downloadBuffer()` (CPU-cached reads too), or
   submit with the operation's fence. Before the image readback of `cpu-copies-retained-after-upload` (phase 3c),
   which will call it.
2. Prove it: the count of image transfer operations stays constant over N screenshots (today it grows by one each).

## References

- `src/Vulkan/TransferManager.cpp` (`downloadImage`, `downloadBuffer`), `src/Vulkan/ImageTransferOperation.hpp`
  (`setRequestedForTransfer`, `isAvailable`).
