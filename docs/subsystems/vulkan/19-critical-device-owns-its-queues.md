## Critical: a device owns its queues, and every device states its own requirements (Oct 2026)

> [!CRITICAL]
> **A `Vulkan::Queue` holds a non-owning `Device &`, never a `std::shared_ptr< Device >`.** The device owns its
> queues (`Device::m_queues`) and destroys them before its `VkDevice` (`Device::destroy()`), so the reference is valid
> for the whole life of the queue.

### Why — the ownership cycle (fixed 2026-10-11)

Until 2026-10-11 each queue kept `this->shared_from_this()` of its device: `Device → unique_ptr< Queue > → shared_ptr<
Device >`, a cycle. In the normal path `Instance` broke it with an explicit `Device::destroy()`. In a FAILED
`Device::create()` nothing did: the caller dropped its `shared_ptr`, the queues kept the device alive, its destructor
never ran and the `VkDevice` was never destroyed. Measured on Linux (RTX 3070 Ti, 2026-10-10) with
`Core/Physics/EnableAcceleration = true`, where the compute device's first queue failed (see below):
`VUID-vkDestroyInstance-instance-00629` ("VkDevice … has not been destroyed"), then a SIGSEGV inside the NVIDIA driver
at exit, in every demo. With the reference (fault injection: the same failing compute device), the process exits 0
and the instance VUID is gone. Ave Robustus II: an owner never depends on its owned objects for its own lifetime.

### Every device states its own requirements

`Instance::getGraphicsDevice()` and `Instance::getComputeDevice()` build SEPARATE `DeviceRequirements`. A feature that
a device-level object relies on must be requested on both. Engine `3c4986549` (queue timelines) made every
`Queue::createTimeline()` create a timeline semaphore and requested `timelineSemaphore` on the graphics device only:
the compute device's first queue then failed (`VUID-VkSemaphoreTypeCreateInfo-timelineSemaphore-03252`),
"Unable to find a suitable compute device", physics acceleration off — and the cycle above turned it into a crash.
`getComputeDevice()` now requests it and `checkDevicesFeaturesForCompute()` refuses a device without it.

### Output structures carry their `sType` before the query

A two-call enumeration (`vkGet…(…, &count, nullptr)` then `resize(count)` then the second call) of a structure that
has an `sType` must resize WITH a prototype carrying `sType` (and `pNext = nullptr`), as
`m_queueFamilyProperties` does in `PhysicalDevice.cpp`. `m_toolProperties.resize(count)` left `sType = 0`:
`VUID-VkPhysicalDeviceToolProperties-sType-sType` and "Unable to get tool properties", once per physical device, on
every validation run on Windows, where the validation layer reports itself as a tool (`count ≥ 1`).
Found by Windows-PA (VVL 1.4.363, NVIDIA + Intel, 2026-10-10). Linux never reports it, with Debian's VVL 1.4.309
or LunarG's 1.4.363 (checked 2026-10-11 on the unfixed binary): the tool list is most likely empty there
(unverified) — a Linux-only validation run does not cover this path.

### A TransferManager takes the role of its device (2026-10-11)

The physics compute device is created with the compute family and the transfer-only family, never a graphics one. Its
`TransferManager` used to take its "specific" command pool on `getGraphicsFamilyIndex()` (family 0 on NVIDIA):
`VUID-vkCreateCommandPool-queueFamilyIndex-01937`, "No physics acceleration available!", then a leaked pool at
`vkDestroyDevice` (`VUID-vkDestroyDevice-device-05137`). Owner decision 2026-10-11: a transfer manager works in the
ROLE of its device. `Device` exposes the PRIMARY-role accessors `getPrimaryFamilyIndex()`, `getPrimaryQueue()`,
`getPrimaryTransferFamilyIndex()`, `getPrimaryTransferQueue()` (the graphics ones when the device has graphics
queues, the compute ones otherwise); `TransferManager`, `BufferTransferOperation` and `ImageTransferOperation` use
only those (20 call sites). Any new code in these three files must do the same: a graphics accessor there breaks the
compute device again.

Measured with `Core/Physics/EnableAcceleration = true`, validation ON, `balls-of-steel` and `sponza`: Linux RTX 3070 Ti
and Windows RTX 3500 Ada + Intel RaptorLake-S (forced) — the physics TransferManager comes up and terminates
gracefully, 0 VUID (no 01937 / 05137), exit 0. macOS (MoltenVK) stops earlier, at the compute device's missing
`VK_KHR_portability_subset`, and exits 0 — fixed 2026-10-12 (`Instance::requestPortabilitySubset()`, caution-points
§ Every logical device enables VK_KHR_portability_subset).
