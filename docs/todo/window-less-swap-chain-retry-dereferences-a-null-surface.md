---
id: window-less-swap-chain-retry-dereferences-a-null-surface
title: In window-less mode, the degraded swap-chain retry dereferences a null window surface
status: open
priority: high
scope: engine Graphics::Renderer, Vulkan::Surface (window-less mode)
opened: 2026-10-29
tags: [crash, window-less, swap-chain]
---

# In window-less mode, the degraded swap-chain retry dereferences a null window surface

## Why

Measured 2026-10-29 on Linux (debug build, engine `cc61b294`, Xvfb, NVIDIA RTX 3070 Ti selected): a downstream
application run with `--window-less` in its normal interactive mode (not a test mode) died with
`SIGSEGV / SEGV_MAPERR` at address `0x38` after 37 s, on the rendering thread:

```
Vulkan::Surface::update(physicalDevice)        src/Vulkan/Surface.cpp:62   (reads m_handle of a null this)
Graphics::Renderer::recreateRenderingSubSystem src/Graphics/Renderer.cpp:3600  m_window.surface()->update(...)
Graphics::Renderer::retryDegradedSwapChain     src/Graphics/Renderer.cpp:3541
Graphics::Renderer::renderFrame                src/Graphics/Renderer.cpp:1947
Core::renderingTask                            src/Core.cpp:434
```

A window-less run has no window surface, so `m_window.surface()` is null; the retry path of a degraded
swap-chain still reaches it. The same binary without `--window-less` (a real X11 window on Xvfb) ran the
whole session. Its window-less test runs never hit it — they presumably never degrade a swap-chain.

## What remains

- Decide what a degraded swap-chain means without a surface (probably: nothing to retry), and guard
  `recreateRenderingSubSystem()` / `retryDegradedSwapChain()` — every `m_window.surface()` dereference — for it.
- Find what degraded the swap-chain in that run (an overlay surface was being created with a 0.001 × 0.001
  geometry just before).
- A test that runs the renderer window-less through a swap-chain degradation.

## References

- The minidump and the full story are kept by the downstream application's maintainers.
