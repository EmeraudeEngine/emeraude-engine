## A swap-chain given up for lack of video memory: told to the user, exit code 95 (Nov 2026)

Found in the crash reports of a downstream application (2026-11-10, 4 sessions, 3 Windows NVIDIA machines): after
hours of use, another process on the same GPU filled the video memory. The renderer could no longer recreate its
swap-chain, gave up after `MaxSwapChainRecreationFailures` attempts (`Renderer::retryDegradedSwapChain()`), and Core
stopped cleanly with `DeviceLostExitCode` (98) — without a word to the user, and with the exit code of a device that
stopped answering. Owner decisions 2026-11-10:

- **Only the swap-chain stops the engine.** Without it nothing can be shown and nothing works; a load (a model, a
  scene) whose allocation is refused stays a failed load and never stops the engine.
- The user is told, by the same contract as a startup refusal (`22-startup-refusals-told-to-the-user.md`): the
  application's hook first, else the engine's native dialog in English.
- A dedicated exit code, `Core::OutOfVideoMemoryExitCode` (95), so a crash reporter tells a full video memory from a
  device loss (98) without reading the journal.
- The dialog shows at the stop, BEFORE the application saves (`onBeforeCoreStop()`), while the window is still there.
- A device loss (98) and a lost display connection (96) are unchanged: a clean stop, no message.

### Naming the cause

- `Vulkan::Device::noteAllocationResult(VkResult)` records the time of every allocation refused with
  `VK_ERROR_OUT_OF_DEVICE_MEMORY`: `DeviceMemory::createOnHardware()` (manual path, external-memory imports included),
  `Buffer` and `Image` creation through VMA. It stops nothing; `Device::outOfDeviceMemoryWithin(delay)` reads it.
- `Renderer::retryDegradedSwapChain()`: a failed recreation within `SwapChainVideoMemoryDelay` (30 s) of a refused
  allocation marks the EPISODE as a video-memory one, until a recreation succeeds. A time window, not "since the last
  presented frame": in production the recreation also fails inside `vkCreateSwapchainKHR` with
  `VK_ERROR_INITIALIZATION_FAILED` (no allocation of the engine's refused), right after refused imports — and a frame
  could be presented between them (measured: 0 to 6 s between the last refused allocation and the first failure, on
  the 4 sessions; the give-up itself came up to 160 s later).
- At the give-up the Fatal says "the video memory is exhausted" and `Renderer::isSwapChainGivenUpForVideoMemory()` is
  raised; the main loop calls `Core::stopOnVideoMemoryExhaustion()` instead of `stopOnDeviceLoss()` (unless the device
  is also lost). One-shot, shared with the two other runtime stops.

### What the user and the application get (`Core::GraphicsFailure`)

`Core::diagnoseGraphicsFailure(cause)` fills `title` ("Not enough graphics memory"), `message` (the GPU and the size of
its largest device-local heap, "the application must close. Restart it to continue working. If this happens again,
close other programs that use the graphics card, or contact support." — the wording of the downstream application,
validated by the owner on 2026-11-11, without its name) and the parts for a translated message: `deviceName`,
`videoMemoryBytes`, and this process's `usageBytes` and `budgetBytes` on that heap (VK_EXT_memory_budget through VMA),
`sharedMemory` for an integrated GPU. `Core::tellGraphicsFailure()` traces it, calls `onGraphicsFailure()` (true = the
application told the user itself), else shows an unparented native dialog. ⚠️ Only a native dialog can show: anything
the engine draws (an overlay, a web view) needs the swap-chain that just failed.

### The application's last chance on every runtime stop (owner decision 2026-11-11)

`GraphicsFailure::Cause` has three values: `OutOfVideoMemory` (95), `DeviceLost` (98, `stopOnDeviceLoss()`) and
`DisplayLost` (96, `stopOnDisplayLoss()`). Each stop calls `tellGraphicsFailure()` → `onGraphicsFailure()` AFTER
`setAppReadyToQuit()` and BEFORE `onBeforeCoreStop()`: the application saves its data there — its logic and its web
content still run, only the drawing or the display is gone (the downstream application's "emergency save": a bus request
to its UI, at most 10 s, while the engine's loop is stopped). The engine's own dialog stays for `OutOfVideoMemory` only;
`DeviceLost` and `DisplayLost` show nothing unless the application does (their `title` / `message` are filled for it:
"Graphics card not responding", "Display connection lost"; no memory query on a lost device). Before, the two other
stops went straight to `onBeforeCoreStop()`: an application that skips its quit handshake once the stop is decided
(`setAppReadyToQuit()`) lost the unsaved work.

### Proof (`EmeraudeStartupFaultInjection`, a build with `EMERAUDE_FAULT_INJECTION`)

- `--inject-swap-chain-out-of-video-memory` (`FaultInjection::swapChainOutOfVideoMemoryInjected()`): Core degrades the
  swap-chain once, `--inject-runtime-failure-delay` seconds after the main loop started (default 5), every recreation
  then fails as a full video memory makes it fail; expected: the "video memory is exhausted" Fatal after 30 attempts
  (≈ 3 s), the message printed by the runner's hook, exit code 95. `--inject-device-loss` and `--inject-display-loss`
  stop the same way through the other two causes (98, 96). Measured 2026-11-11 (Linux, RTX 3070 Ti, branch
  `cef/emergency_save`): 95, 98, 96 with their texts, the reference 0.
- `--eat-video-memory`: the real test. The runner fills the video memory with device-local allocations through
  `Vulkan::DeviceMemory` (256 MiB, then four times smaller after each refusal, down to 16 MiB; their refusal is
  recorded like a refused import), then grows the window by a quarter every 500 ms (at most 8192 px a side) until a
  recreation fails; the engine's native dialog shows, the eaten memory is given back first. ⚠️ It fills the GPU of the
  machine it runs on (the desktop may stutter until the stop); the runner stops by itself after 60 s.

**Measured** (2026-11-10, Linux main, RTX 3070 Ti 8 GB, NVIDIA 620.70, KDE Wayland):

- `--inject-swap-chain-out-of-video-memory`: 30 attempts, the video-memory Fatal, the message, exit code **95**, 0 VUID;
  the reference run unchanged (exit 0).
- `--eat-video-memory` at a CONSTANT window size: 3232 MiB eaten until the driver refused 16 MiB, then 119
  recreations in 60 s, all successful. `recreate()` frees the attachments first and takes the same room back: the
  engine's own memory cannot make a same-size recreation fail. In production another process takes the freed room.
- `--eat-video-memory` with the window growing (the GPU applications closed, 5600 MiB eaten): every recreation
  succeeded up to 8192x8192. The heaps show where the attachments went: VMA's allocations on the device-local heap
  stayed at 60-110 MiB while those on the SYSTEM heap grew from 0 to 3072 MiB; the budget warning fired ("GPU memory
  AT ITS BUDGET", 5776 MiB used for 5854). On this driver VMA spills to system memory instead of failing: nothing
  fails, frames read the attachments over PCIe. The real test cannot end on 95 here; the production failures were on
  Windows (WDDM): to be measured there.
- Windows (Windows-PA, RTX 3500 Ada Laptop 12 GB, driver 32.0.15.9658, one 1920x1080 screen at 125 %, engine
  d4bf3bab): the injection exits **95** and the reference 0, validation layer loaded, 0 VUID. The real test: 11680 MiB
  eaten until the driver refused 16 MiB (WDDM does refuse a raw device-local allocation once the heap is full), then
  119 recreations in 60 s, all successful, no give-up: the engine's VMA allocations spilled to the system heap (its
  depth/stencil images there), and the driver recreated the presentable images while the process used about
  12070 MiB of a 11247 MiB budget (WDDM overcommits them). ⚠️ Windows clamps the window to the desktop: it stayed at
  1924x1055, the swap-chain never grew.
- **Conclusion (2026-11-10):** a full video memory alone does not make the swap-chain fail on these two drivers — the
  engine spills, the driver overcommits. The production failures needed more: the CEF imports, which cannot spill (a
  dedicated import of an external texture), the depth buffer refused by VMA on BOTH heaps, `VK_ERROR_OUT_OF_HOST_MEMORY`
  in one session (system memory exhausted as well). The stop told to the user covers those cases; the injection is
  what proves it on the three OSes (Linux, Windows, macOS: exit 95, 0 VUID).
- macOS (macOS-PA, Apple M6, engine d4bf3bab): the injection exits **95**, the reference 0, validation layer loaded,
  0 VUID, no crash report. Unified memory: the message named 16384 MiB of "video memory", the whole system memory;
  since then an integrated GPU (`VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU`) sets `GraphicsFailure::sharedMemory` and the
  message says "The graphics memory, shared with the system, is full". The real test is not run on a Mac (it would
  fill the system memory).
- ⚠️ The runner crashed Steam (another Chromium-based process on the same GPU) while the memory was full: whoever
  allocates at the wrong moment falls. Close the GPU applications of the machine before the real test.
- ⚠️ Side finding: at 11441x6416 the process died inside the libdecor GTK decoration plugin ("creating a buffer file
  for -1527818496 B failed", then SIGSEGV): `Window::resize()` accepts any size. The runner caps its growth at 8192.

### Related

- `docs/subsystems/graphics/37-gpu-memory-budget-and-report.md` — the budget warning (an allocation refused while the
  engine is under it means another process holds the memory), and why the swap-chain attachments are not reserved.
- `docs/subsystems/source-tree/08-startup-fault-injection.md` — the fault-injection build and runner.
