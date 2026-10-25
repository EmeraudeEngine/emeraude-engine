---
id: swapchain-more-than-five-images-aborts
title: A driver returning more than 5 swap-chain images aborts the engine (StaticVector capacity)
status: open
priority: high
scope: Vulkan/SwapChain.cpp, Graphics/Renderer.cpp (createRenderingSystem)
opened: 2026-10-25
tags: [vulkan, swapchain, crash, defect]
---

# A driver returning more than 5 swap-chain images aborts the engine

## Why

`SwapChain` asks for `minImageCount` images (2 or 3, `selectImageCount()`), but Vulkan lets the implementation
create MORE: `vkGetSwapchainImagesKHR` returns the real count into `m_imageCount`, and `SwapChain.cpp` then calls
`m_frames.resize(m_imageCount)` on a `Base::StaticVector< Frame, 5 >`; `Renderer::createRenderingSystem(imageCount)`
does the same on `StaticVector< RendererFrameScope, MaxFramesInFlight >` (5). A count above 5 is a
`std::abort()` (no exceptions), a crash at start-up or at the first swap-chain recreation. Found by reading
(2026-10-25, while sizing the IBL ring on `MaxFramesInFlight`); never observed — the cascade's three machines return
the count they are asked for. A trust boundary (the driver's answer) that is not checked.

## What remains

- [ ] Decide the refusal (owner): fail the swap-chain creation cleanly with an error naming the count, or use only the
      first `MaxFramesInFlight` images (legal? every acquired image index must have its frame), or raise the cap.
- [ ] Check the count against the capacity BEFORE the resize, in both places; a test with a forced count.

## References

- `src/Vulkan/SwapChain.cpp` (`selectImageCount()`, the `vkGetSwapchainImagesKHR` count), `src/Graphics/Renderer.cpp`
  (`createRenderingSystem`), `src/Constants.hpp` (`MaxFramesInFlight`).
