---
id: physical-device-enumeration-refuses-vk-incomplete
title: The physical device enumeration treats VK_INCOMPLETE as a failure and stops the engine
status: in-progress
priority: high
scope: Vulkan/Instance
opened: 2026-10-15
tags: [vulkan, startup, windows, defect]
---

# The physical device enumeration treats VK_INCOMPLETE as a failure and stops the engine

## Why

`Instance::preparePhysicalDevices()` (`src/Vulkan/Instance.cpp:345`) calls `vkEnumeratePhysicalDevices()` twice
(count, then fill) and accepts only `VK_SUCCESS` from each. `VK_INCOMPLETE` is a **success code** (positive
`VkResult`): the returned data is valid, the list simply changed or did not fit, and the caller is expected to query
again. The engine logs it as an error, reports "No physical device available !" and the Vulkan instance service fails:
the application exits with code 1 before a window is shown.

Measured in user crash reports of a downstream application, 2026-10-15 (Windows 10, engine 1.0.0, bundled loader
1.4.363), two machines, three sessions, both AMD Ryzen "H" laptop CPUs (Radeon iGPU, very likely a discrete GPU too —
the log cannot tell, see `gpu-inventory-without-vulkan`):

- on the FIRST call (count query): `Unable to get physical device count : VK_INCOMPLETE !`;
- on the SECOND call (fill): `Unable to get physical devices : VK_INCOMPLETE !` — the device count grew between the
  two calls (a hybrid laptop whose discrete GPU appears late, or an ICD answering differently).

These machines very likely have a perfectly capable GPU: the engine refuses them for nothing.

## Done (2026-10-15, engine 7dfbe4c0)

- `Vulkan::enumerate()` (`src/Vulkan/Utility.hpp`): the standard loop (count, size, fill; again while `VK_INCOMPLETE`,
  8 attempts at most, the last answer kept); only a negative `VkResult` fails. `preparePhysicalDevices()` uses it, logs
  the number of devices found and warns on a retry.
- Every other two-call enumeration goes through it: instance / device layers and extensions, tool properties, surface
  formats, present modes, present rectangles, video session memory requirements; `PhysicalDeviceProbe` accepts any
  non-negative answer. The swap-chain image query keeps `VK_SUCCESS` only, on purpose (its count is the swap-chain's
  own: a partial list would be missing images — commented there).

## Production reports (2026-10-21, crash-report tool)

56 reports (≈17 machines, Windows, 8.0.2 / engine 1.0.0) end on `No physical device available !`; 34 of them carry `VK_INCOMPLETE` — this item's defect, at scale (the others: a real absence of device, to read). Engine 1.0.1 (shipped with 8.0.3) holds the fix; **the 8.0.3 production reports are the Windows run of "What remains"**.

Plan: `CrashReport Solucius` (a plan of the downstream application).

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 24 — per version: 8.0.2 ×23, 8.0.3 ×1 |
| Reports | 67 (first 2026-10-15, last 2026-11-09) |
| Versions | 8.0.2 ×66, 8.0.3 ×1 |
| Platforms | windows ×67 |
| OS | Windows 10 Pro ×25, Windows 11 Home ×21, Windows 10 Home ×17, Windows 11 Pro ×3, Windows 10 Education ×1 |
| GPU (selected) | unknown ×67 |
| ANGLE backend | d3d11 ×67 |
| Died at milestone | SessionOpened ×66, CEFInitializationFailed ×1 |
| Single-instance lock | not recorded ×67 |

- **Cause (normalized):** `VulkanInstanceService: No physical device available !`
- **Where:** `vendors/emeraude-engine/src/Vulkan/Instance.cpp` — `EmEn::Vulkan::Instance::onInitialize`
- **Signature:** `065c7bb001149082`

Last error before the death (else the last trace), by number of reports:

- ×66 `died at SessionOpened`
- ×1 `died at CEFInitializationFailed`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[a line of the downstream application, redacted] (×4)
[Core][Success] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) PlatformManagerService service up!
[VulkanInstanceService][Error] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::preparePhysicalDevices`) Unable to get physical device count : VK_INCOMPLETE !
[VulkanInstanceService][Fatal] (vendors/emeraude-engine/src/Vulkan/Instance.cpp `EmEn::Vulkan::Instance::onInitialize`) No physical device available !
[Core][Fatal] (vendors/emeraude-engine/src/Core.cpp `EmEn::Core::initializeCoreLevel`) VulkanInstanceService service failed to execute!
```

Archives:

- 67 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## 8.0.3 status (checked 2026-10-21)

**Fixed.** 34 of the 56 reports carry `VK_INCOMPLETE`, accepted as a success since `7dfbe4c0` (engine 1.0.1): those machines should start. The 22 others had no device at all: 8.0.3 tells them (`NoVulkanDriver`, exit 97) — Changed, not fixed.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## Why it stays open (2026-10-22)

The four sibling causes fixed by 1.0.1 were closed on 2026-10-22 on the owner's word
(`docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md` § What the production reports showed). Not this one:
its signature is the Fatal "No physical device available !" (`src/Vulkan/Instance.cpp:320`), which 1.0.1 still traces
for a machine with no device at all before the `NoVulkanDriver` dialog and exit 97 — and the downstream tool classifies
by the first Fatal, before the exit code. Closed now, the 22 no-device reports would come back on 8.0.3 as false
regressions. It closes once the tool tells exit 97 apart (a downstream application item)
and the 8.0.3 reports show no `VK_INCOMPLETE`.

## What remains

- [ ] A run on a machine that answers `VK_INCOMPLETE` (a Windows AMD hybrid laptop, as in the reports): no reproduction
      machine here; the Linux path is verified unchanged (3 devices found).

## References

- Vulkan specification, § "Return Codes" (`VK_INCOMPLETE` is a success code) and § "Physical Devices"
  (`vkEnumeratePhysicalDevices`: fewer handles written than available → `VK_INCOMPLETE`).