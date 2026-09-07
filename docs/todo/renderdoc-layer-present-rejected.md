---
id: renderdoc-layer-present-rejected
title: RenderDoc layer + validation layer — every present is rejected, root cause unattributed
status: open
priority: medium
scope: Vulkan/Swapchain
opened: 2026-09-07
tags: [vulkan, renderdoc, validation, swapchain, measured]
---

# RenderDoc layer + validation layer — every present is rejected, root cause unattributed

## Why

A GPU capture and the validation layers cannot be used in the same run. That is a real loss: the
one moment we most want validation is while investigating what a capture shows. Today the
workaround is documented (`docs/caution-points.md` § Platform-Specific: force X11, turn
`Core/Video/VulkanInstance/EnableDebug` off for the capture run) but **nobody knows which side is
wrong**, and the wrong reading is cheap to reach: the VUIDs name our own draw call, so they read
like an engine layout bug.

## What is measured (Sep 2026, RTX 3070 Ti, X11/XWayland, RenderDoc 1.43 **and** 1.46, identical)

With both layers loaded, no frame is ever presented. First VUID of the burst — the only one worth
reading, the rest are consequences:

```
VUID-vkCmdDraw-None-09600
  command buffer expects VkImage 0x9d000000009d (aspect COLOR, mip 0, layer 0) to be in layout
  VK_IMAGE_LAYOUT_GENERAL -- instead, current layout is VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
VUID-vkQueuePresentKHR-pWaitSemaphores-03268   (semaphore with no way to be signalled)
VUID-VkPresentInfoKHR-pImageIndices-01430      (presented image in VK_IMAGE_LAYOUT_UNDEFINED)
[VulkanQueue] Unable to present an image : VK_ERROR_VALIDATION_FAILED_EXT
```

Term isolation, same binary, same X11 platform, one term changed at a time:

| Layers loaded | Result |
|---|---|
| validation only | 0 error, frame presented, screenshot correct |
| RenderDoc only | `.rdc` written (549 MB), replayed: 90 draw calls, 18 dispatches, 55 render passes |
| both | 6 validation errors, `VK_ERROR_VALIDATION_FAILED_EXT`, nothing presented, no capture |

## What remains

1. Identify the draw that binds a **swapchain image** to a descriptor expecting
   `VK_IMAGE_LAYOUT_GENERAL`. The capture itself names it: replay
   `capture_frame1177.rdc` (or a fresh one) and resolve the `VkImage` of the failing
   `vkCmdDraw` — the final composition and the overlay/notifier passes are the first suspects.
2. Decide which side is at fault, with evidence, not with intuition:
   - **RenderDoc**: it wraps the swapchain with its own usage flags and image layouts; validation
     tracks the layouts it saw. A layer that re-creates swapchain images can legitimately desync
     the tracker. If so, the finding belongs upstream (baldurk/renderdoc), not here.
   - **The engine**: if we transition a swapchain image to `GENERAL`, write a descriptor for it,
     and let something else move it back to `PRESENT_SRC_KHR` before the draw, the code is wrong
     and merely happens to work when no layer perturbs the timing. That is a real defect and must
     be fixed here.
3. Whichever it is, update `docs/caution-points.md` § Platform-Specific and the
   `/renderdoc-capture` recipe in projet-alpha, and delete this file.

## ⚠️ Traps

- ⚠️⚠️ **Do NOT read those VUIDs as proof of an engine bug.** The same code presents cleanly with
  the RenderDoc layer absent. Any conclusion must survive the isolation table above.
- ⚠️ **The failure is silent from the console side.** `triggerRenderDocCapture()` answers
  *"captured on the next present"* whether or not a present ever succeeds; the absence of a `.rdc`
  is the only signal. This is what left the problem unexplained from Jun to Sep 2026.
- ⚠️ **RenderDoc has no Wayland support at all** (`renderdoccmd version`: *"xlib, XCB, Vulkan
  KHR_display"*), so any experiment must force `Core/Video/Window/GLFW/UsePlatform` = `"X11"`
  first — otherwise the app dies on `VK_ERROR_EXTENSION_NOT_PRESENT` before frame one and tells
  you nothing about this defect.
- ⚠️ Restore the owner's `settings.json` after every experiment (`UsePlatform`, `EnableDebug`).

## References

- `docs/caution-points.md` § Platform-Specific — the workaround and the full measurement.
- `README.md` § GPU debugging with RenderDoc — the two mandatory conditions.
- projet-alpha `.claude/commands/renderdoc-capture.md` — the working capture procedure.
- `src/Graphics/RenderDocCapture.cpp` — the in-application API wrapper.
