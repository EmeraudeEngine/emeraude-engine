## A startup refused for the GPU or its driver: diagnosed, retried, told to the user (Oct 2026)

Found in user crash reports of a downstream application (2026-10-15): of 12 reports, 11 were SILENT startup refusals on
5 machines — the window never showed, the process exited with code 1, and only a log said "VK_INCOMPLETE",
"VK_ERROR_FEATURE_NOT_PRESENT" or "Incompatible driver". Owner decisions 2026-10-15: the user is told that the hardware
is not compatible, or to update an outdated driver; the engine shows a native dialog in English by default and the
application may replace it; the message links the GPU vendor's driver page.

### Enumerations: `VK_INCOMPLETE` is a success (`Vulkan::enumerate()`, `src/Vulkan/Utility.hpp`)

The two-call enumeration of the specification: count, size, fill; again while the answer is `VK_INCOMPLETE` (the list
grew between the calls — a hybrid laptop whose discrete GPU appears late), 8 attempts at most, the last answer kept;
only a negative `VkResult` fails. Every two-call enumeration of the engine uses it (physical devices, layers,
extensions, tool properties, surface formats, present modes, present rectangles, video session memory requirements).
⚠️ Measured: a Windows AMD laptop answered `VK_INCOMPLETE` to the COUNT query itself — its count is used.
The swap-chain image query keeps `VK_SUCCESS` only: its count is the swap-chain's own, a partial list would miss images.
`preparePhysicalDevices()` logs "N physical device(s) found" and warns on a retry.

### Which devices may be used (`Vulkan::Instance`)

- `minimumDeviceAPIVersion()`: the application's target (`23-the-application-targets-a-vulkan-version.md`; 1.3 by
  default, at most 1.2 on macOS, whose shaders target 1.2 / SPIR-V 1.5). A device below it, or without timeline
  semaphores at its effective version (`VK_KHR_timeline_semaphore` below 1.2), is refused at the selection (graphics and
  compute) with one line: device, decoded driver version, the reported and the required versions (or what is missing),
  "update the graphics driver". Measured: a GTX 1050 Ti on an outdated driver (Vulkan 1.2) used to reach
  `vkCreateDevice()` and die with `VK_ERROR_FEATURE_NOT_PRESENT`.
- The REQUIRED device features (`requireVK10/11/12/13` in `createGraphicsDevice()`) are checked BEFORE
  `vkCreateDevice()`: the missing ones are named and the device is not created (`vkCreateDevice()` names none).
- `getGraphicsDevice()` = the selection, then `createGraphicsDevice()` on the selected device, then on every other
  compatible device from the best: one device that cannot be created no longer ends the startup.
- Every refusal is recorded: `Instance::deviceRefusals()` (device, driver, API version, reason
  `BelowRequiredAPIVersion` / `MissingFeatures` / `CreationFailed`); `Instance::creationResult()` keeps the answer of
  `vkCreateInstance()`.
- One unconditional line names the device used: `Graphics device: '<name>' (<type>, vendor 0x…, device 0x…), driver
  '<driverName>' (<driverInfo>), driver version <decoded>, Vulkan x.y.z.`

### The GPUs without Vulkan (`SystemInfo::fetchGPUInformation()`)

The operating system's adapter list, independent of Vulkan, logged at startup ("Graphics adapters (operating system):
N | …") and exposed by `SystemInfo::getGPUDeviceInformation()` (`GPUDevice`: names, ids, bus, driver name and
version, `software` for a framebuffer / Basic Render adapter). Linux: `/sys/class/drm/cardN/device` (vendor, device,
`uevent` DRIVER / PCI_SLOT_NAME, `/sys/module/<driver>/version`), the names from the system's `pci.ids` when present;
integrated vs discrete is a heuristic (an iGPU sits on the root PCI bus). Windows: DXGI; macOS: IOKit / Metal.

### The refusal told to the user (`Core.startup.cpp`)

`Core::initializeCoreLevel()` records which service failed (`CoreLevelFailure`). For the Vulkan instance or the
graphics renderer WITHOUT a graphics device, `diagnoseStartupRefusal()` classifies (`Core::StartupRefusal::Cause`):

| Cause | When | Message (English) |
|---|---|---|
| `NoVulkanDriver` | `vkCreateInstance()` → `VK_ERROR_INCOMPATIBLE_DRIVER`, or no physical device | "needs a graphics driver supporting Vulkan 1.3, and none was found" + the OS-listed GPUs + one driver page per vendor + "an old integrated graphics chip is not compatible" |
| `OutdatedDriver` | a device refused below the minimum version, or without timeline semaphores | names the GPU, its driver version and Vulkan version (or what it lacks), "update the graphics driver" + the vendor's page |
| `IncompatibleHardware` | a device lacking a required feature, or no compatible device | names the GPU and the missing features, "not compatible", "updating an old driver may help" |
| `Other` | not classified as above | (no dialog: `EXIT_FAILURE` as before) |

Then `Core::onStartupRefused(refusal)` — the application may show its own message (wording, translations) and return
`true`; otherwise a native `Dialog::Message` (error, unparented: the window may not exist yet). The process exits with
`Core::IncompatibleSystemExitCode` (97), so a crash reporter can tell "incompatible system" from a crash. Driver pages:
NVIDIA `https://www.nvidia.com/drivers`, AMD `https://www.amd.com/en/support/download/drivers.html`, Intel
`https://www.intel.com/content/www/us/en/support/detect.html`.

### Verified (Linux, RTX 3070 Ti + Intel RPL-S + llvmpipe, validation layer, 2026-10-15)

- Normal startup unchanged: "3 physical device(s) found", the device line, 0 VUID.
- No Vulkan driver (`VK_DRIVER_FILES` pointing at nothing): the zenity error "Graphics driver required" naming
  "GA102 [GeForce RTX 3070 Ti], Raptor Lake-S GT1 [UHD Graphics 770]" with the NVIDIA and Intel pages; exit 97.
- Test-only builds (never committed): llvmpipe reported as 1.2 and the NVIDIA GPU without a required feature → both
  refused, started on the Intel iGPU, 0 VUID; every device reported as 1.2 → "Graphics driver too old" naming the GPU
  and its driver, NVIDIA page, exit 97; every device without a required feature → "Graphics hardware not compatible",
  exit 97.
- Windows (Window-PA, RTX 3500 Ada + Intel UHD, 2026-10-15): the inventory names both GPUs and flags the Basic Render
  Driver software; with no ICD the unparented MessageBox "Graphics driver required" names both GPUs with both driver
  pages (the software adapter left out), exit 97. ⚠️ A MessageBox shows URLs as text, not links.
- macOS (macOS-PA, Apple M6): the inventory names "Apple M6" (IOKit, driver com.apple.AGXG18G); MoltenVK reports Vulkan
  1.3, above the macOS minimum 1.2; with no ICD `vkCreateInstance()` answers `VK_ERROR_INCOMPATIBLE_DRIVER` and the
  NSAlert runs unparented and modal before the window exists (GLFW already made NSApp); once dismissed, a clean teardown
  to "Core level terminated" and exit code 97 (2026-10-15). ⚠️ A SIGTERM is not served while the modal alert is up.
- The "Graphics device:" line decodes the vendor driver versions: Windows NVIDIA "596.58", Windows Intel "101.7088"
  (Intel packing), Linux NVIDIA; Apple M6 through MoltenVK (2026-10-15).
- Possible improvement, not open work (owner informed): a Windows TaskDialog with hyperlinks would make the driver URLs
  clickable.
- ⚠️ Not verified: a real `VK_INCOMPLETE` machine (Windows AMD hybrid laptops), a real outdated driver, an Intel Mac.
- ⚠️ DXGI / IOKit names differ from Vulkan's ("Intel(R) UHD Graphics" vs "Intel(R) RaptorLake-S Mobile Graphics
  Controller"): match the two lists by the LUID / the vendor and device ids, never by name.
