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

## What remains

- [ ] Fix the `capture()` path so it works on an acquired image (or on its own copy).
