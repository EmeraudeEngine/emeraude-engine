---
id: screenshot-non-acquired-swapchain-image
title: screenshot() uses a non-acquired swap-chain image
status: open
priority: low
scope: Graphics/Renderer
opened: unknown
tags: [vulkan, validation, capture]
---

# screenshot() uses a non-acquired swap-chain image

## Why

`Core.RendererService.screenshot()` triggers `UNASSIGNED-non-acquired-swapchain-image-used`: the
capture path transitions a presentable image outside its acquire window. Pre-existing, fires only
on capture, cosmetic for the capture itself — but it is a validation error in the tool every
visual verification depends on.

## Measured on three platforms (2026-09-22)

- **macOS** (Apple M2, MoltenVK 1.4.1, validation really active): EVERY capture emits it, the log line right
  after `Core.RendererService.screenshot()` — `vkQueueSubmit(): pSubmits[0] performs a layout transition on
  presentable VkImage …, but the image has not been acquired from VkSwapchainKHR …`. It has no VUID, so a
  `grep -c VUID` reads 0.
- **Linux** (RTX 3070 Ti, same day, same `relief` captures): not a single line in the logs. The defect is
  the same code (`SwapChain::capture()`, the comment at the download already explains it: a presented image
  belongs to the presentation engine until re-acquired); only the layer's detection differs.
- ⚠️ So a Linux run proves nothing about this item: check it on macOS, and grep `UNASSIGNED` besides `VUID`.

## What remains

- [ ] Fix the `capture()` path so it works on an acquired image (or on its own copy).
