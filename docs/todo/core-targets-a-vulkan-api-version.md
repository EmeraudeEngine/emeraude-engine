---
id: core-targets-a-vulkan-api-version
title: The application chooses the Vulkan API version it targets from Core; whatever needs more turns off by itself
status: in-progress
priority: high
scope: engine Core, Vulkan (Instance, Device, Queue, RenderPass), Saphir (shader target), Graphics (Renderer sub-services)
opened: 2026-10-15
tags: [vulkan, startup, compatibility]
---

# The application chooses the Vulkan API version it targets from Core; whatever needs more turns off by itself

## Why

The engine refuses every device below Vulkan 1.3 (1.2 on macOS) and requires the 3D renderer's features (bindless
descriptor indexing, buffer device address, timeline semaphores, multiview, demote to helper invocation). A downstream
application that only shows the overlay needs none of them, yet its users on older drivers or GPUs are refused at
startup (the refusals measured in its crash reports, 2026-10-15, `docs/subsystems/vulkan/22-startup-refusals-told-to-
the-user.md`).

## Owner decisions (2026-10-15)

- The application targets a Vulkan API version FROM `Core` (its subclass); the engine's default stays 1.3.
- Everything that needs more than the target is turned off automatically — never a silent degradation of a feature the
  application asked for: a use of a disabled subsystem is refused explicitly.
- Below 1.2 the timeline semaphores come from `VK_KHR_timeline_semaphore` (required): the GPU-lifetime infrastructure
  (`Queue` tracked submissions, `DeferredDestructor`) is not rewritten on fences.
- The lowest target is **1.1** (+ the timeline extension), not 1.0: a desktop driver stuck at 1.0 predates 2018, and the
  timeline extension (late 2019) never shipped on one, so "1.0 + timeline" accepts the same machines as "1.1 +
  timeline" while 1.1 makes the extended queries, the external memory (CEF zero-copy) and multiview core. A target
  asked below 1.1 is raised to 1.1 with a log line.
- Approach A (a declared requirement) over an automatic degradation for everyone, and over lowering the threshold only.
- The hook is a virtual method of `Core` (`targetVulkanAPIVersion()`, default 1.3), read at startup before the Vulkan
  instance exists.
- Below 1.2 (no bindless) the engine is OVERLAY-ONLY for now: enabling a scene is refused with an explicit error. A
  basic 3D without bindless may come later.
- The accelerated CEF import (external memory) is core at the 1.1 floor: available at every target.
- A command-line argument (`--vulkan-api-target=X.Y`) LOWERS the target for one launch (tests on any machine), never
  above the application's target.

## What remains

- [x] Inventory (2026-10-15): the overlay itself is Vulkan 1.0 (one combined sampler, a push constant, SPIR-V-1.0-clean
      shaders). What blocks a 1.0 target is around it: the graphics selection gate requires descriptor indexing
      (`Instance::checkDevicesFeaturesForGraphics`); the device probe chains Vulkan11/12/13 structures and calls
      `vkGetPhysicalDeviceToolProperties` (1.3) on every device; the device creation chains Vulkan11/12/13 structures;
      VMA's BUFFER_DEVICE_ADDRESS (and MEMORY_BUDGET, whose extension is never enabled) flags are unconditional; the
      timelines (core 1.2) carry every tracked upload and the deferred destructor; the renderer stops if the bindless
      manager fails; the default resources and the post-processor start without a scene; the shader target and its
      cache identity are hard-coded (vk1.3 / SPIR-V 1.6); pipeline-creation feedback is gated on the DEVICE version;
      MSAA uses `vkCreateRenderPass2` (1.2).
- [x] Design: decided (above).
- [x] Implementation and the Linux runs (2026-10-15): targets 1.3, 1.2, 1.1 with the validation layer, 0 validation
      message each; the knowledge is in `docs/subsystems/vulkan/23-the-application-targets-a-vulkan-version.md`.
- [ ] Windows peer: `--vulkan-api-target=1.1`, the CEF main menu through the accelerated (zero-copy,
      `VK_KHR_external_memory_win32`) import, a scene refused, 0 validation message; 1.2 and 1.3 unchanged.
- [ ] macOS peer: the same at 1.1 on MoltenVK.
- [ ] The downstream application overrides `targetVulkanAPIVersion()` (its own repository, at the owner's go).

## Gaps found by the inventory (to fix in the same pass or to item)

- `fragmentStoresAndAtomics`, `multiDrawIndirect`, `drawIndirectFirstInstance` are required, only RT / the opt-in MDI
  use them; the MDI refusal (`Renderer.cpp` ~879) checks `shaderDrawParameters` but not `shaderInt64` / BDA;
  `shaderImageGatherExtended` is optional and no consumer checks it.
- `VK_KHR_multiview` is always requested (core since 1.1; only the cubemap targets use multiview).

## ⚠️ Traps

- `shaderDemoteToHelperInvocation` has no `demote` in any GLSL, yet it IS needed at a 1.3 target: compiling for
  SPIR-V 1.6, glslang declares the DemoteToHelperInvocation capability for discarding fragment shaders (measured:
  VUID-VkShaderModuleCreateInfo-pCode-08740 on RenderableInstancePointLightPassFragmentShader when it was dropped). It is
  required from an effective 1.3 only.

- The target also fixes the SPIR-V version the shaders are compiled to (1.1 → SPIR-V 1.3, 1.2 → 1.5, 1.3 → 1.6): a
  device rejects a module above its version.
- A device ABOVE the target is driven at the target (`VkApplicationInfo::apiVersion`, no feature above it enabled), so
  the downstream path is testable on any modern GPU.
- No profile helps a machine without any Vulkan driver.

## References

- `src/Vulkan/Instance.hpp` (`RequestedAPIVersion`, `MinimumDeviceAPIVersion`), `src/Vulkan/Instance.cpp`
  (`createGraphicsDevice()`: the required and optional feature lists), `src/Saphir/ShaderManager.cpp` (the glslang
  target), `src/Vulkan/Queue.cpp` (timeline), `src/Graphics/BindlessTextureManager.*`, `src/Vulkan/RenderPass.cpp`.
