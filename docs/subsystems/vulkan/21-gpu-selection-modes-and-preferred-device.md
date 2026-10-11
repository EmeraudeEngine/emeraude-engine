## GPU selection: modes, ranking, and a device preferred by the application (Oct 2026)

`Instance::getGraphicsDevice()` (and `getComputeDevice()`) keep only the physical devices that pass every compatibility
check (queues, presentation for the window, required features and extensions), then rank them. Order of authority:
**`Core/Video/VulkanDevice/ForceGPU`** (a device name) **> the device preferred by the application > the ranking**.

### Modes (`Core/Video/VulkanDevice/AutoSelectMode`, default `Performance`)

| Mode | Ranking (the best is the last after `rankDevices()`) |
|---|---|
| `Performance` | score = (1 + **every extension the device advertises** + 10 × queue count) × 5 discrete / 3 integrated / 2 virtual; then type, VRAM |
| `RawPerformance` (2026-10-11) | device type (discrete 4 > integrated 3 > virtual 2 > CPU 1), then the largest `DEVICE_LOCAL` heap (VRAM), then the score |
| `PowerSaving` | score with the inverse queue term and integrated × 5 |
| `DontCare` | the plain score |

⚠️ `Performance` is "performance + features": the advertised-extension count dominates, so a recent iGPU can beat a
modest discrete card. Measured by the owner on a downstream application (2026-10-11): an Intel iGPU won over a Quadro
P600, while Chromium's GPU process used the P600. `RawPerformance` ranks by power only (the owner's request:
"performance brute"); the REQUIRED features and extensions still filter in every mode.
The last tie-break is the enumeration order (lower first): the former `std::map< size_t, … >` keyed by score SILENTLY
dropped a device whose score equalled another's (graphics and compute selections both).

### A device preferred by the application: `Instance::preferGraphicsDevice(DeviceIdentity)`

For an application whose other components already sit on a GPU — the case that motivated it: a Chromium GPU process
pinned with `use-adapter-luid`, whose D3D11 shared textures can only be imported on the same adapter. Call it before
the first `getGraphicsDevice()` (the core level), on the main thread. `Vulkan::DeviceIdentity` carries the LUID
(Windows: the DXGI adapter LUID == `VkPhysicalDeviceIDProperties::deviceLUID`) and/or the device UUID; `matches()`
compares the LUIDs when both carry one, else the UUIDs. `PhysicalDevice::identity()` gives a device's identity, so
the application compares `getGraphicsDevice()->physicalDevice()->identity()` with what it asked for.
The preferred device is selected only among the compatible ones ("… (PREFERRED by the application)"); absent or not
compatible → "The physical device preferred by the application is absent or not compatible : the score decides."
Without a call, nothing changes.

⚠️ An integrated GPU reports SYSTEM memory as its `DEVICE_LOCAL` heap (measured: Intel RPL-S 64 GiB, llvmpipe 128 GiB,
RTX 3070 Ti 8 GiB): VRAM only compares devices of the same type, which is why `RawPerformance` ranks the type first.

### Listing the devices before the engine exists: `Vulkan::probePhysicalDevices(binaryDirectory)`

`Vulkan/PhysicalDeviceProbe.hpp`: a minimal temporary instance (no layer, no surface extension;
`VK_KHR_portability_enumeration` when the loader offers it) on the loader shipped with the application, destroyed before
returning; per device: name (the `ForceGPU` spelling), `DeviceIdentity`, vendor/device ID, type, largest `DEVICE_LOCAL`
heap. For a decision taken before the core starts — the Chromium GPU process is launched at `CefInitialize()`, BEFORE
the engine selects its device (measured: three Chromium subprocesses started before "Graphics capable physical device
selected"). Case that motivated it: `ForceGPU` holds a VULKAN name, which DXGI may spell differently (Windows peer:
Vulkan "Intel(R) RaptorLake-S Mobile Graphics Controller", DXGI "Intel(R) UHD Graphics"), so a DXGI name match could
never pin Chromium to the forced iGPU; the probe resolves the name to its LUID.
It opens the loader with `Loader::VolkBinding::None` and fetches its own entry points: volk's global pointers are never
touched, so it is safe whether or not the engine's loader is already open (measured: a probe after the engine started,
then the scene renders, 0 VUID, exit 0).

### Measured (Linux, RTX 3070 Ti + Intel RPL-S + llvmpipe, 2026-10-11)

- `Performance` and `RawPerformance`: the RTX 3070 Ti; `PowerSaving`: the Intel iGPU (which cannot present under
  NVIDIA Wayland: item igpu-presentation-under-nvidia-wayland).
- Preferred device (temporary test injection, reverted): `PowerSaving` + the RTX preferred → "NVIDIA GeForce RTX 3070
  Ti (PREFERRED by the application)"; a non-existent identity → the warning, then the ranking (RTX in
  `RawPerformance`).
