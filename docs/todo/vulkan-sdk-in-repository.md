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

## Owner decision (2026-10-11): in the ext-deps archive, v018 upgraded in place

The Vulkan SDK minimum goes into the ext-deps-generator archives — not as binaries committed to this repository —
and the published **v018 is upgraded in place** (nobody consumes it yet apart from the campaign tests). Reasons: one
source of truth for every native dependency (glslang already comes from ext-deps; a `-I` to the SDK once mixed
glslang 16.4 headers with the 16.5 library, `cmake/SetupVulkan.cmake`), no binaries in the public history, the
per-OS / per-configuration download already proven on the three OS (2026-10-10).

⚠️ Re-publishing the same tag: `InstallDependencies.cmake` does not download when `dependencies/<dir>` or the
`<dir>.v018.zip` already exists. Every machine that extracted v018 (none today: the developers use symlinks, the
campaign clones were deleted) must delete both, or it keeps the old content silently.

## State (2026-10-11)

- ext-deps-generator `extract_vulkan_sdk.py` (headers, link library, stripped validation layer + relative manifest,
  macOS loader / MoltenVK / ICD, licences, `VERSION`) — run on Linux: 38 MiB per configuration.
- Engine: `SetupVulkan.cmake` requires `vulkan-sdk/` (no version, no system path) and links no Vulkan library: the
  engine opens the subtree's loader by explicit path (`Vulkan::Loader` + volk, owner option A), CEF keeps its own. The
  layer is copied next to the binary and prepended to `VK_ADD_LAYER_PATH`. projet-alpha's macOS bundle takes MoltenVK /
  loader / ICD from it. Linux proved (caution-points § The Vulkan SDK comes from the external dependencies).
- Licences: `LICENSES/` holds only the SDK's pointer file; the per-component Apache-2.0 texts (Vulkan-Headers,
  Vulkan-Loader, Vulkan-ValidationLayers, MoltenVK) are still to add.

## What remains

- Run the extraction on macOS and Windows (SDK 1.4.363), check the layouts the script assumes, then the three-OS proof
  with `VK_LOADER_DEBUG=layer,driver`, then the owner removes the system SDKs, then re-publish v018 (owner decision:
  upgraded in place) — every machine that extracted v018 deletes its folder and zip first.

- A proposal to the owner: per OS, the strict minimum (headers, loader, MoltenVK on macOS, the Khronos validation
  layer + its manifest, glslang tools if needed), licences (Apache-2.0 / MIT: LGPLv3-compatible, to confirm per
  component), sizes, Git LFS or not, how `SetupVulkan.cmake` and the runtime (`VK_ADD_LAYER_PATH`, the macOS bundle)
  find it, and how a version bump is done.
