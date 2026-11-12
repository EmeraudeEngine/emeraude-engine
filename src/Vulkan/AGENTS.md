# Vulkan System


Context for developing the Emeraude Engine Vulkan abstraction layer.


## Where the content is (router)

> ⚠️ This file is a ROUTER since 2026-09-27: its sections were moved VERBATIM to
> `docs/subsystems/vulkan/`, one file per topic (a very large section is a folder, one file per
> sub-topic). **Read only the file(s) matching the task** — never the whole folder. A reference
> elsewhere to "`src/Vulkan/AGENTS.md` § <Section>" means the row of that title below.

| Section | File | Size |
|---|---|---|
| Module Overview | [`docs/subsystems/vulkan/01-module-overview.md`](../../docs/subsystems/vulkan/01-module-overview.md) | 1 KB |
| Vulkan-Specific Rules | [`docs/subsystems/vulkan/02-vulkan-specific-rules.md`](../../docs/subsystems/vulkan/02-vulkan-specific-rules.md) | 19 KB |
| Important Files | [`docs/subsystems/vulkan/03-important-files.md`](../../docs/subsystems/vulkan/03-important-files.md) | 1 KB |
| Critical: LayoutManager Thread Safety | [`docs/subsystems/vulkan/04-critical-layoutmanager-thread-safety.md`](../../docs/subsystems/vulkan/04-critical-layoutmanager-thread-safety.md) | 2 KB |
| Critical: Descriptor Pool FREE_DESCRIPTOR_SET_BIT | [`docs/subsystems/vulkan/05-critical-descriptor-pool-free-descriptor-set-bit.md`](../../docs/subsystems/vulkan/05-critical-descriptor-pool-free-descriptor-set-bit.md) | 1 KB |
| Critical: Descriptor pools are GROWABLE, and a working NVIDIA run proves nothing about their sizes (Sep 2026) | [`docs/subsystems/vulkan/06-critical-descriptor-pools-are-growable-and-a-working-nvidia.md`](../../docs/subsystems/vulkan/06-critical-descriptor-pools-are-growable-and-a-working-nvidia.md) | 1 KB |
| Critical: Dedicated Device Memory on MoltenVK (Sep 2026) | [`docs/subsystems/vulkan/07-critical-dedicated-device-memory-on-moltenvk.md`](../../docs/subsystems/vulkan/07-critical-dedicated-device-memory-on-moltenvk.md) | 1 KB |
| Critical: Buffer Descriptor Offset | [`docs/subsystems/vulkan/08-critical-buffer-descriptor-offset.md`](../../docs/subsystems/vulkan/08-critical-buffer-descriptor-offset.md) | 1 KB |
| Development Patterns | [`docs/subsystems/vulkan/09-development-patterns.md`](../../docs/subsystems/vulkan/09-development-patterns.md) | 8 KB |
| Queue Family Ownership Transfer (buffer uploads) | [`docs/subsystems/vulkan/10-queue-family-ownership-transfer.md`](../../docs/subsystems/vulkan/10-queue-family-ownership-transfer.md) | 3 KB |
| TLAS Async Build (Inline Command Buffer Recording) | [`docs/subsystems/vulkan/11-tlas-async-build.md`](../../docs/subsystems/vulkan/11-tlas-async-build.md) | 2 KB |
| Critical: Deferred destruction contract (`DeferredDestructor`) + uploads still running (queue timelines, `PendingSubmissions`, `Device::destroyAfter()`) | [`docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md`](../../docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md) | 6 KB |
| Critical: Ray Query vs RT Pipeline Stage Flags | [`docs/subsystems/vulkan/13-critical-ray-query-vs-rt-pipeline-stage-flags.md`](../../docs/subsystems/vulkan/13-critical-ray-query-vs-rt-pipeline-stage-flags.md) | 1 KB |
| External-Memory Image Import (zero-copy CEF accelerated paint) | [`docs/subsystems/vulkan/14-external-memory-image-import.md`](../../docs/subsystems/vulkan/14-external-memory-image-import.md) | 3 KB |
| Multi-Draw Indirect Support | [`docs/subsystems/vulkan/15-multi-draw-indirect-support.md`](../../docs/subsystems/vulkan/15-multi-draw-indirect-support.md) | 2 KB |
| Critical Points | [`docs/subsystems/vulkan/16-critical-points.md`](../../docs/subsystems/vulkan/16-critical-points.md) | 2 KB |
| Detailed Documentation | [`docs/subsystems/vulkan/17-detailed-documentation.md`](../../docs/subsystems/vulkan/17-detailed-documentation.md) | 1 KB |
| VkPipelineCache — the driver cache the engine now owns (Aug 2026) | [`docs/subsystems/vulkan/18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md`](../../docs/subsystems/vulkan/18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md) | 6 KB |
| Critical: a device owns its queues (`Queue` holds `Device &`), every device states its own requirements, output structures carry their `sType` (Oct 2026) | [`docs/subsystems/vulkan/19-critical-device-owns-its-queues.md`](../../docs/subsystems/vulkan/19-critical-device-owns-its-queues.md) | 3 KB |
| Validation layers for one run: `--set-vk-layers` (Oct 2026) | [`docs/subsystems/vulkan/20-validation-layers-for-one-run.md`](../../docs/subsystems/vulkan/20-validation-layers-for-one-run.md) | 2 KB |
| GPU selection: modes (`RawPerformance`), ranking without lost ties, a device preferred by the application (`preferGraphicsDevice`), translation layers (Microsoft D3D12) ranked last (Oct-Nov 2026) | [`docs/subsystems/vulkan/21-gpu-selection-modes-and-preferred-device.md`](../../docs/subsystems/vulkan/21-gpu-selection-modes-and-preferred-device.md) | 3 KB |
| A startup refused for the GPU or its driver: `VK_INCOMPLETE` accepted (`Vulkan::enumerate()`), the target's minimum (`23`), required features checked, fallback to the next device, the OS GPU inventory, the user told (`Core::onStartupRefused`, exit 97) (Oct 2026) | [`docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md`](../../docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md) | 6 KB |
| The application targets a Vulkan version (`Core::targetVulkanAPIVersion()`, `--vulkan-api-target`): 1.1 + timeline extension = overlay only, 1.2 = the scene renderer, 1.3 = everything; the effective version `min(target, device)` drives the probe, the feature chain, the shaders' SPIR-V, VMA (Oct 2026) | [`docs/subsystems/vulkan/23-the-application-targets-a-vulkan-version.md`](../../docs/subsystems/vulkan/23-the-application-targets-a-vulkan-version.md) | 6 KB |
| A swap-chain given up for lack of video memory: the refused allocations recorded (`Device::noteAllocationResult`), the episode named (`Renderer::isSwapChainGivenUpForVideoMemory`), the user told (`Core::onGraphicsFailure`, exit 95); a failed load never stops the engine (Nov 2026) | [`docs/subsystems/vulkan/24-swap-chain-given-up-for-lack-of-video-memory.md`](../../docs/subsystems/vulkan/24-swap-chain-given-up-for-lack-of-video-memory.md) | 5 KB |
| Critical: an upload is ordered on the GPU before its readers (`Device::uploadFrontier()`, waited by every renderer batch and the AS builds) and after the previous writes of the same object (`pendingWrites()`, `SynchInfo::waitsFor()`); a host fence orders the host only (Nov 2026) | [`docs/subsystems/vulkan/25-uploads-ordered-before-their-readers.md`](../../docs/subsystems/vulkan/25-uploads-ordered-before-their-readers.md) | 7 KB |
| Swap-chain failure paths: a refused frame submission drops the frame and degrades the swap-chain; a degraded HEADLESS swap-chain (window-less) is recreated without a surface (the null-surface SIGSEGV); a driver's image count beyond 5 is refused, never an abort; a degraded swap-chain keeps an on-demand application rendering until it is recreated or given up (Nov 2026); `--inject-frame-submit-failure`, `--inject-swap-chain-image-count` | [`docs/subsystems/vulkan/26-swap-chain-failure-paths.md`](../../docs/subsystems/vulkan/26-swap-chain-failure-paths.md) | 4 KB |
