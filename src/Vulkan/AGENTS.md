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
