---
id: engine-throwing-at-calls
title: The engine still calls the throwing .at() (17 sites)
status: open
priority: high
scope: src/Audio, src/Graphics (SharedUniformBuffer, CubemapResource, …)
opened: 2026-10-11
tags: [ave-robustus, defect]
---

# The engine still calls the throwing .at() (17 sites)

## Why

`.at()` throws `std::out_of_range`, which aborts under `-fno-exceptions` (`terminate called after throwing an instance
of 'std::out_of_range'`). Proven on 2026-10-10: `SwapChain::chooseSurfaceFormat()` called `formats.at(0)` on an empty
format list (Intel iGPU, `VK_ERROR_SURFACE_LOST_KHR`) and the process aborted (exit -6) — fixed 2026-10-11
(`std::optional` return, swap-chain creation refused). 17 other sites remain:
`/usr/bin/grep -rn -E "\.at\(" src --include=*.cpp --include=*.hpp` (MusicResource, Audio/Manager,
SharedUniformBuffer, CubemapResource, …). projet-alpha has the same item (`throwing-at-calls`).

## What remains

- Per site: a proven index with `[]` (+ `assert`), or an explicit bounds check and a refusal.
- A tool to keep it out: a clang-tidy / grep gate on `.at(` in the cascade's own sources.
