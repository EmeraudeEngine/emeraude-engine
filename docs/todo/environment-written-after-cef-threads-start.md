---
id: environment-written-after-cef-threads-start
title: The Vulkan loader environment is written after CEF's threads exist (setenv race)
status: open
priority: high
scope: PlatformManager::onInitialize(), PlatformSpecific::prependVulkanLayerDirectory() / pinVulkanLoaderToBundledDriver()
opened: 2026-10-11
tags: [concurrency, vulkan, cef, defect]
---

# The Vulkan loader environment is written after CEF's threads exist (setenv race)

## Why

The loader reads its configuration from the environment only, so the engine writes it before the first loader call:
`VK_ADD_LAYER_PATH` (the layers shipped with the application, 2026-10-11, every OS) and `VK_DRIVER_FILES` (macOS,
the bundled MoltenVK). Both happen in `PlatformManager::onInitialize()`, inside `Core::run()` — and in projet-alpha
`CefInitialize()` has already run by then (`src/Boot/*/main.*`), so Chromium's threads exist. POSIX `setenv()` may
reallocate `environ` while another thread's `getenv()` walks it: undefined behaviour (clang-tidy
`concurrency-mt-unsafe`, 3 findings in `Helpers.linux.cpp`, 2026-10-11). Windows' `SetEnvironmentVariableW` is
serialised by the system, the POSIX path is the exposed one.

## What remains

- Write the loader environment before any thread exists: an engine entry point called by the consumer's `main()` before
  CEF (and before the ThreadPool), or the Core constructor if it provably runs first — a design choice for the owner.
- Then the three findings leave the ledger.
