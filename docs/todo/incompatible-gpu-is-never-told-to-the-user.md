---
id: incompatible-gpu-is-never-told-to-the-user
title: A startup refused for the graphics hardware or driver is never told to the user
status: in-progress
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

## Done (2026-10-15)

- Owner decisions 2026-10-15: the engine shows a native dialog in English by default, the application may replace it
  (`Core::onStartupRefused(const StartupRefusal &)` returns true), the message links the GPU vendor's driver page.
- `Core::initializeCoreLevel()` records the failing service; `Core::diagnoseStartupRefusal()` (`src/Core.startup.cpp`)
  classifies `NoVulkanDriver` / `OutdatedDriver` / `IncompatibleHardware` from `Vulkan::Instance::creationResult()`,
  `physicalDevices()`, `deviceRefusals()` and the OS GPU list; `tellStartupRefusal()` shows the dialog; the process exits
  with `Core::IncompatibleSystemExitCode` (97). Doc: `docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md`.
- Verified: Linux (zenity; the three causes, forced), Windows (an unparented MessageBox naming both GPUs and both
  driver pages, the software adapter left out, exit 97), macOS M6 (the NSAlert runs unparented and modal before the
  window exists).

## What remains

- [ ] macOS: the exit code 97 after the alert is dismissed (the owner's screen was locked during the run).
- Noted for the owner, not open work: the Windows MessageBox shows the URLs as text (a TaskDialog with hyperlinks
  would make them clickable); a SIGTERM is not served while the modal dialog is up (macOS, measured).

## Open questions (owner, when the item is started)

- Answered 2026-10-15: the engine's dialog by default with an application override; the engine owns the English
  wording, the application its translations (through the hook); vendor driver links included.

## References

- `device-below-vulkan-1-3-reaches-vkcreatedevice`, `physical-device-enumeration-refuses-vk-incomplete`,
  `gpu-inventory-without-vulkan`, `log-the-gpu-driver-version`.