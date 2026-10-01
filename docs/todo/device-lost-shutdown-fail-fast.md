---
id: device-lost-shutdown-fail-fast
title: After a DEVICE_LOST, the Windows shutdown ends in a 0xc0000409 fail-fast instead of a clean exit
status: open
priority: unranked
scope: Vulkan device-loss handling, Core shutdown (Windows; the other OS not checked)
opened: 2026-10-01
tags: [vulkan, device-lost, shutdown, windows, robustness]
---

# After a DEVICE_LOST, the Windows shutdown ends in a 0xc0000409 fail-fast instead of a clean exit

## Why

Measured by the Windows peer (triad 15, 2026-10-01, AMD Radeon, beams, 1 launch out of 4). The TLAS instance address was
not 16-byte aligned (`VUID-vkCmdBuildAccelerationStructuresKHR-pInfos-03715`; fixed by triad 15). After that:

1. `[Error][VulkanQueue] Unable to submit work into the queue : VK_ERROR_DEVICE_LOST !` (`[device_fault] addr=0x0 READ_INVALID`).
2. 8× `VUID-vkResetFences-pFences-01123` and 1× `VUID-vkQueueSubmit-fence-00064`.
3. On a `resize(1600, 900)`: "Failed to resize the 'ToneMappingEffect' effect of the post-process stack !", then a fence
   `VK_TIMEOUT`.
4. `Core.shutdown()` reached "Engine is about to stop (User exit code: 0)" and the logics thread join. The process then
   ended with `0xc0000409` (fail-fast) in `Emeraude.dll` at offset `0x1bba1c9`, about 21 MB past the last export. That
   is probably the statically linked CRT's `abort()` / `__fastfail`: a `std::abort()` from a guard (a `StaticVector`
   overflow, an `assert`), or an invalid-parameter handler.

Once the device is lost, the engine keeps resetting and submitting fences, and the teardown aborts. Every resource
destruction must stay legal after a device loss, and the exit should be the clean one.

## What remains

- [ ] Reproduce on demand: force a device loss (a shader with an infinite loop, or a debug console command that triggers
  one), on the Windows AMD peer first, then on Linux and macOS.
- [ ] Capture the stack of the fail-fast (a minidump, WinDbg `!analyze -v`, or a Debug build), and name the guard that
  aborts.
- [ ] Once the loss is detected, stop every fence reset / submit (the `m_deviceLost` latch of
  `AccelerationStructureBuilder` is the model), skip the post-process resizes, and make the teardown legal.
- [ ] Re-test: after a forced loss, `Core.shutdown()` exits 0, with no new VUID after the first loss report.

## References

- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 15 (6), Windows (B).
- Vulkan spec, "Lost Device": which commands stay valid after `VK_ERROR_DEVICE_LOST`.
