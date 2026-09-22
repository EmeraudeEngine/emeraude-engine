---
id: window-less-frame-usable-and-capturable
title: A window-less run must render the real frame and let it be captured
status: open
priority: unranked
scope: Graphics/Renderer (renderOffscreenFrame), Vulkan/SwapChain capture, Overlay
opened: 2026-09-22
tags: [window-less, capture, benching, cross-platform]
---

# A window-less run must render the real frame and let it be captured

## Why

Owner (2026-09-22): "Il faudra possiblement corriger le path du renderer sans swap-chain, mais il devrait
être capable de créer des images utilisables et capturables." The window-less mode is how a device that
cannot present is benched. The case that exposed it: this workstation's Intel iGPU (`Intel(R) Graphics
(RPL-S)`, Mesa 25.0.7). Mutter runs on the NVIDIA card and refuses the iGPU's dmabufs (`failed to import
supplied dmabufs: Could not bind the given EGLImage to a CoglTexture2D`). Even `vkcube --gpu_number 1`
aborts at swapchain creation, and the engine hangs 60 s per acquire.

## Measured, 2026-09-22

- `--window-less` crashed at startup, fixed in `e189549b`. `Overlay::Manager::initImGUI()` handed a null
  GLFWwindow to `ImGui_ImplGlfw_InitForVulkan()` (SIGSEGV, no log line). ImGUI is now skipped there.
- After that fix, the run reaches "scene successfully loaded" on the Intel device (forced through
  `Core/Video/VulkanDevice/ForceGPU`). `Core.RendererService.screenshot()` answers
  `Framebuffer capture failed !`, because `Renderer::captureFramebuffer()` only reads the swapchain.
- `Renderer::renderOffscreenFrame()` is NOT the windowed frame. It is one forward render pass into an
  8-bit `WindowLessView` (`RenderTarget::View< ViewMatrices2DUBO >`, single-sample): no HDR scene target,
  no post-process chain (tone mapping, exposure, TAA), no lighting lanes. An image from it validates
  "it starts and draws", not a rendering change.

## What remains

- [ ] Owner decision: the design. The option presented but not answered (2026-09-22) was the SAME frame as
      the windowed one, rendered into an engine-owned final image instead of the swapchain image, with
      the capture reading that image.
- [ ] `screenshot()` works window-less.
- [ ] Re-bench the Intel iGPU window-less on `relief` modes 0 and 1: 0 VUID, 0 UNASSIGNED.

## ⚠️ Traps

- Capturing INSIDE the frame, before the present, is also the fix of `screenshot-non-acquired-swapchain-image`
  (the windowed capture reads an image already handed to the presentation engine). The two items share a
  mechanism, so settle them together.
- ImGUI screens are not drawn window-less (no platform backend): a capture there shows no ImGUI.
