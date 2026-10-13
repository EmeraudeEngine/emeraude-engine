---
id: intel-hang-render-thread-frees-memory-during-resize
title: Intel iGPU hard hang — a render-thread vkFreeMemory deadlocks the driver during a resize storm
status: open
priority: high
scope: Vulkan/DeferredDestructor, Graphics/Renderer (GrabPass recreation), Vulkan memory (VMA block release), Overlay resize path
opened: 2026-10-14
tags: [vulkan, intel, windows, resize, deadlock, defect]
---

# Intel iGPU hard hang — a render-thread vkFreeMemory deadlocks the driver during a resize storm

## Why

Windows peer, 2026-10-14, Intel RaptorLake-S iGPU (driver 101.7088, forced through a settings copy): the application
freezes for good (Windows: "Not Responding", never recovers). Pre-existing: reproduced at d58de797, before the
2026-10-13 overlay work, and at 900846fe.

Repro: launch with no demo, `Stage.openMenu()`, then bursts of 20 back-to-back `Core.WindowService.resize` between
1280x720 / 1440x810 / 1600x900 (/1366x768), ending at 1280x720, 3 s apart. Hangs within the first 1-3 bursts in
**11 of 11** launches — with the validation layer, without it, and with every implicit layer disabled. NVIDIA (same
machine): 0 hang in 4 x 15 bursts.

Symbolized stacks (Release + PDB, the same in every capture):
- render thread: `Core::renderingTask` → `Renderer::renderFrame` → `DeferredDestructor::tick` → `GrabPass::destroy`
  (once `~LTCTexture`) → `Image::~Image` → `vmaDestroyImage` → `VmaDeviceMemoryBlock::Destroy` → `vkFreeMemory` →
  igvk64, waiting on a condition variable inside the driver;
- the driver's own present thread: `igxelpicd64!DrvSwapBuffers` → `DrvPresentBuffers` → igvk64,
  `RtlAcquireSRWLockShared`;
- main thread: blocked in whatever driver entry it reached — `glfwSetWindowSizeWin32` → `NtUserSetWindowPos` → a
  window-message hook into igxelpicd64 (`RtlEnterCriticalSection`); or `Overlay::Manager::onWindowResized` →
  `Surface::createFramebufferResources` → `TransferManager::transitionImageLayout` → `vkQueueSubmit`
  (`RtlAcquireSRWLockExclusive`); at d58de797 `Surface::updatePhysicalRepresentation` → `Device::waitIdle`.

Reading (peer, not proven): an Intel driver deadlock between `vkFreeMemory` (freeing the emptied VMA block of the
old-size GrabPass / LTC images retired after the resize-triggered recreation) and its asynchronous present while the
window is being resized. The engine is the trigger: render-thread frees during a resize storm, continuous present,
and the bare menu rendered unpaced (~4 800 FPS before 6bdef322 — re-measure on it).

## What remains

- Re-measure on 6bdef322 + (the no-scene pacing may change the odds, not the cause).
- Confirm the vkFreeMemory link: e.g. keep VMA's empty blocks (no `VmaDeviceMemoryBlock::Destroy` right after a
  recreation) for one run — if the hang vanishes, the trigger is proven.
- Options for the owner, after the proof: VMA keeps empty blocks / a deferred block release outside the resize window;
  idle the present queue before such frees; check the Intel driver's known issues and a newer driver. Owner decision.

## ⚠️ Traps

- A single 20-resize burst per phase does NOT reproduce it (the peer's earlier runs): several bursts, menu open, no demo.
- Force the Intel GPU through a settings COPY, `--disable-settings-autosave` always.

## References

- Windows peer report 2026-10-14 (stacks out15-19 `*-stacks.txt` on that machine).
- VMA documentation, "Allocation algorithm / empty blocks" (`VMA_ALLOCATION_CREATE_*`, `vmaSetCurrentFrameIndex`).
