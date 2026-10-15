---
id: incompatible-gpu-is-never-told-to-the-user
title: A startup refused for the graphics hardware or driver is never told to the user
status: open
priority: high
scope: Core, Vulkan/Instance, Graphics/Renderer
opened: 2026-10-15
tags: [startup, ux, drivers, defect]
---

# A startup refused for the graphics hardware or driver is never told to the user

## Why

When the Vulkan instance or the graphics device cannot be created, `Core::initializeCoreLevel()`
(`src/Core.cpp:1146`, failures at `:1167` and `:1232`) logs a fatal line and the process exits with code 1. No dialog
is shown: the user sees the application vanish, and only a log (or a crash report) holds "Incompatible driver !",
"VK_ERROR_FEATURE_NOT_PRESENT" or "VK_INCOMPLETE" — words a user cannot act on.

Owner decision, 2026-10-15: **the user must be told that the hardware is not compatible, and, when the driver is
outdated, to update it.**

Measured in user crash reports of a downstream application, 2026-10-15: of 12 reports, 11 are such silent startup
refusals (5 machines); one user relaunched five times, another twice — nothing told them why.

## What remains

- [ ] Classify the failure into causes a user can act on, each with its own message:
  - **no Vulkan driver at all** (`vkCreateInstance` → `VK_ERROR_INCOMPATIBLE_DRIVER`, or no physical device):
    "no graphics driver supporting Vulkan was found — install / update the graphics driver; if the computer only has
    an old integrated GPU, it is not compatible";
  - **driver too old** (a device below the required Vulkan version, `device-below-vulkan-1-3-reaches-vkcreatedevice`):
    name the GPU, its driver version, and ask to update the driver;
  - **hardware not compatible** (a required feature the device cannot have whatever the driver): name the GPU and say
    it is not supported;
  - anything else: a generic message pointing to the log.
- [ ] Show it in a native dialog (`PlatformSpecific/Desktop/Dialog/`) before the exit, then exit with a dedicated
      non-zero code so a downstream crash reporter can tell "incompatible system" from a crash.
- [ ] Expose the classification to the application (a hook or a queryable result) so a downstream application can
      word, localise or replace the message, and record it in its own reports.
- [ ] Runtime verification: each message triggered once (forced minimum version, no ICD via an empty
      `VK_DRIVER_FILES`, …) and recorded in the topic doc.

## Open questions (owner, when the item is started)

- Engine dialog by default with an application override, or the application only? Who owns the wording and its
  translations?
- A link to the GPU vendors' driver pages in the message, or not?

## References

- `device-below-vulkan-1-3-reaches-vkcreatedevice`, `physical-device-enumeration-refuses-vk-incomplete`,
  `gpu-inventory-without-vulkan`, `log-the-gpu-driver-version`.