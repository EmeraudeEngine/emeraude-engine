---
id: texture-destroyed-while-being-created
title: An image is destroyed by another thread while it is still being created or recorded
status: open
priority: unranked
scope: Vulkan / Graphics resources (texture creation, loader refusal, teardown)
opened: 2026-09-30
tags: [vulkan, lifetime, textures, validation, race]
---

# An image is destroyed by another thread while it is still being created or recorded

## Why

Left over from `texture-destroyed-while-upload-in-flight` (2026-09-30). That item's main variant — a texture
released while its upload ran (`VUID-vkDestroyImage-image-01000`, then `vkDestroyDevice-05137`) — is fixed by the
queue timelines (2026-10-07, engine `docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md`). These
variants are NOT: the object is used AFTER its destruction, which no deferred destruction can cover.

- 2026-10-07, Linux RTX 3070 Ti, validation on, the cyclic-BSP WAD below, deferral disabled for the A/B: 1 run in 3
  gave 2 × `VUID-vkBindImageMemory-image-parameter` ("Invalid VkImage Object") — the image was destroyed between
  `vkCreateImage` and `vkBindImageMemory`. With the deferral on: 0 VUID in 4 runs (too few to call it gone).
- 2026-09-30: 2 × `VUID-vkCmdCopyBufferToImage-dstImage-parameter` + 4 × `VUID-VkImageMemoryBarrier-image-parameter`
  — an upload RECORDED after its image was destroyed.
- Teardown variants to re-check on the timeline build: citadel's 20 s shutdown on Windows RTX 3060 (2026-10-02,
  4 × image-01000, 4 × buffer-00922, 2 × vkFreeMemory-00677) and 20 heavy glTF loads in a row then shutdown (Linux,
  2026-09-30) — both may be the upload variant, now fixed.

## Repro

1. Copy `/usr/share/games/doom/doom1.wad`; in the copy, set the right child of E1M1's ROOT BSP node to the root
   itself (NODES lump: node `count - 1`, the uint16 at byte 24 = `count - 1`; E1M1 has 236 nodes).
2. Launch projet-alpha without a demo, validation on, `Core.openFiles("<copy>")`: "Corrupted WAD … the BSP node
   #235 is reached …".

## What remains

- [ ] Find who destroys an `Image` another thread is creating / recording (the loader's refusal path releasing a
      texture whose creation task still runs?) — gdb on `DebugMessenger::debugCallback` gives the creator's stack;
      the destroyer needs a breakpoint on `Image::destroyFromHardware` for that handle.
- [ ] Re-run the two teardown repros on the timeline build.
