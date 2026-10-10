---
id: vulkan-sdk-in-repository
title: Ship the strict minimum of the LunarG Vulkan SDK in dependencies/VulkanSDK/<OS>/<version>/
status: open
priority: unranked
scope: dependencies/, cmake/SetupVulkan.cmake, the three OS
opened: 2026-10-11
tags: [vulkan, build, tooling]
---

# Ship the strict minimum of the LunarG Vulkan SDK in dependencies/VulkanSDK/<OS>/<version>/

## Why

Owner request (2026-10-11): no Vulkan SDK installation step after a new clone, the same SDK version on the three OS.
The campaign of 2026-10-10 showed three different validation layers (Linux: Debian 1.4.309, macOS: 1.4.357 in
`/usr/local`, Windows: 1.4.363) and macOS bundling the MoltenVK of whatever SDK is installed. Since 2026-10-11 the
Linux machine also has LunarG 1.4.363 in `~/VulkanSDK/1.4.363.0` (user space, loaded with `VK_ADD_LAYER_PATH`).

## Owner's plan (2026-10-11)

1. The layout is `dependencies/VulkanSDK/<OS>/<1.4.x>/` in the emeraude-engine repository.
2. Its content is EXTRACTED from the SDK installed on each OS (the strict minimum).
3. The engine is started SHADOWING the system installation (the repository copy wins), and it must be proved on the
   three OS that the repository copy is the one in use (loader / layer / MoltenVK paths in the logs).
4. Only then the owner removes the system Vulkan SDK from each machine — the final proof that nothing reaches it.

## What remains

- A proposal to the owner: per OS, the strict minimum (headers, loader, MoltenVK on macOS, the Khronos validation
  layer + its manifest, glslang tools if needed), licences (Apache-2.0 / MIT: LGPLv3-compatible, to confirm per
  component), sizes, Git LFS or not, how `SetupVulkan.cmake` and the runtime (`VK_ADD_LAYER_PATH`, the macOS bundle)
  find it, and how a version bump is done.
