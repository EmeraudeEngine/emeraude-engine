## The application targets a Vulkan API version; whatever needs more turns off (Oct 2026)

A downstream application that only shows the overlay was refused on older drivers: the engine required Vulkan 1.3 and
the 3D renderer's features on every device. Owner decisions 2026-10-15 (engine item
`core-targets-a-vulkan-api-version`): the application chooses the version it targets from `Core`, the engine uses
nothing above it on any device and turns off what needs more; a driver too old for the target is told to the user
(`22-startup-refusals-told-to-the-user.md`).

### The target

- `Core::targetVulkanAPIVersion()` (virtual, default `Vulkan::Instance::HighestTargetAPIVersion` = 1.3), read once at
  startup before the Vulkan instance (`Core::initializeCoreLevel()` → `Instance::setTargetAPIVersion()`). Compared on
  major.minor; below `LowestTargetAPIVersion` (1.1) it is raised, above 1.3 lowered, each with a log line.
- `--vulkan-api-target=X.Y` LOWERS it for one run (never above the application's target): a downstream path tested on
  any machine.
- The lowest target is 1.1 + `VK_KHR_timeline_semaphore`, not 1.0: a desktop driver stuck at 1.0 predates 2018 and the
  timeline extension (late 2019) never shipped on one, so 1.0 would accept no more machine, while 1.1 makes the
  extended queries, the external memory (CEF zero-copy) and multiview core.

### What follows the target

| | 1.1 | 1.2 | 1.3 |
|---|---|---|---|
| Instance `apiVersion`, device minimum (`minimumDeviceAPIVersion()`; at most 1.2 on macOS) | 1.1 | 1.2 | 1.3 |
| Timelines (`Queue`, `DeferredDestructor`) | `VK_KHR_timeline_semaphore` (`vkWaitSemaphoresKHR`…) | core | core |
| Shaders (`ShaderManager::setTargetAPIVersion()`, also in the binary cache identity) | SPIR-V 1.3 | SPIR-V 1.5 | SPIR-V 1.6 (macOS 1.5) |
| Scene (3D) renderer: bindless, IBL, scene default textures, MDI, RT, mesh shaders | off: **overlay only**, `Scenes::Manager::newScene()` refuses | on | on |
| Required device features (the 3D list in `createGraphicsDevice()`) | none beyond 1.1 | the 3D list | the 3D list + `shaderDemoteToHelperInvocation`, `maintenance4` when advertised |
| MSAA (depth resolve through `vkCreateRenderPass2`) | 1 sample (`Device::checkMultisampleCount()`) | yes | yes |
| Hardware video encode (synchronization2) | off | off | when advertised |
| Pipeline-creation feedback | off | off | on |

The device is driven at its EFFECTIVE version, `min(target, device)` (`Device::apiVersion()`,
`Instance::effectiveAPIVersion()`): the probe (`PhysicalDevice`, which receives the instance version), the feature
chain given to `vkCreateDevice()` (`DeviceRequirements::linkFeatureChain()`: the Vulkan 1.1/1.2 structures from 1.2,
1.3 from 1.3, each extension's structure only when it is enabled), VMA (`vulkanApiVersion`, the
BUFFER_DEVICE_ADDRESS / MEMORY_BUDGET flags only with their feature / extension), ImGui (`ApiVersion`).

### Verified (Linux, RTX 3070 Ti + Intel RPL-S + llvmpipe, validation layer, 2026-10-15)

- Target 1.3 and 1.2, the `default` demo: 0 VUID, 0 validation message, the same picture (mean luma 119.5 both).
- Target 1.1 (`--vulkan-api-target=1.1`): 0 VUID, 0 validation message, exit 0; the bindless manager not started; the
  CEF main menu drawn (`Stage.openMenu()`, 667 colours in the capture); `Stage.loadDemo(default)` refused: "The scene
  'default' cannot be created: the application targets Vulkan 1.1, which draws the overlay only".
- Test-only build (never committed) without the timeline extension, target 1.1: every device refused, the dialog
  "Graphics driver too old … it lacks what this application requires (no timeline semaphores)" with the NVIDIA page,
  exit 97.

### ⚠️ Traps

- The effective version also limits the PHYSICAL-device queries: a 1.3 query (`vkGetPhysicalDeviceToolProperties`) on a
  1.4 device of a 1.2 instance is refused by the validation layer (UNASSIGNED-API-Version-Violation, not a `VUID-`:
  count every `[VALIDATION][ERROR]` line, not only `VUID-`).
- `shaderDemoteToHelperInvocation` has no `demote` in any GLSL, yet compiling for SPIR-V 1.6 glslang declares the
  DemoteToHelperInvocation capability for discarding fragment shaders (VUID-VkShaderModuleCreateInfo-pCode-08740 when it
  was dropped): required from an effective 1.3 only.
- A scene must be refused at its CREATION, not at its enabling: the `Scene` constructor already builds the scene
  renderer's effects (a crash in the post-process construction at 1.1 when only `enable()` refused).
- A service that is never started is never terminated: the bindless manager is not even given the device below 1.2
  (else "the selected graphics device smart pointer still have 2 uses" at shutdown).
- Found on the way, true at every target: the buffer ownership-transfer barrier named the acceleration-structure stage
  unconditionally (`BufferTransferOperation`; invalid on any device without the RT extensions and with a dedicated
  transfer family); a sampler asked for anisotropy without the feature, or above the device limit (a settings value):
  `Renderer::getSampler()` now clamps both.
- Not verified yet: Windows (the downstream application's platform, CEF zero-copy through
  `VK_KHR_external_memory_win32` at 1.1), macOS (MoltenVK at 1.1), a real driver without timelines.
