# GPU memory — the budget warning and the allocation report (2026-10-04)

Before this, a scene that outgrew the GPU looked like a GPU hang with nothing in the log (item
`vram-budget-observability`, the 2026-09-25 terrain hang; then JungleRuins' Xid 109 on 2026-10-04).

## What exists

| Piece | Where | What it gives |
|---|---|---|
| `Vulkan::Device::memoryBudgets()` | VMA `vmaGetHeapBudgets` (VK_EXT_memory_budget; without it VMA estimates 80 % of the heap) | Per heap: budget, usage, VMA's allocated / reserved bytes, size, device-local |
| `Vulkan::Device::memoryDetailedStatisticsJSON()` | VMA `vmaBuildStatsString(detailed)` | Every block and allocation: type, size, usage flags (megabytes on a large scene: on demand only) |
| `Renderer::checkMemoryBudget()` | every 2 s after a frame fence | A WARNING when a device-local heap reaches **98 %** of its budget, with the 12 largest allocation groups; an info when it is back under 90 % |
| `Core.RendererService.getGPUMemory(top)` | console / MCP | Heaps (budget, usage, `atBudget`) and the allocations grouped by heap, kind (BUFFER / IMAGE) and usage flags, largest first |
| `Core.RendererService.writeGPUMemoryReport()` | console / MCP | VMA's full JSON into the captures directory; answers its path |

## ⚠️ "At the budget", not "over it"

VMA stops a device-local heap AT its budget and places the next allocations in a system-memory heap. JungleRuins:
heap 0 usage 7154 MiB = budget 7154 MiB, 22 GB in heap 1. A test `usage > budget` never fires in exactly the case that
matters; the warning fires at 98 %. Those system-memory allocations stay there: the GPU reads them over PCIe, a frame
can take seconds, an NVIDIA GPU loses the device (Xid 109, CTX SWITCH TIMEOUT).

## Reading a report

The usage flags name the resource: `VERTEX|…|DEVICE_ADDRESS|AS_INPUT` is mesh geometry (or a per-instance buffer, small
and numerous), `INDEX|…` its indices, `IMAGE SAMPLED|TRANSFER_DST` a texture, `IMAGE SAMPLED|COLOR_ATTACHMENT` a render
target. In Release the Vulkan identifiers are compiled out, so allocations carry no name: kind, usage and SIZE are the
evidence. Many allocations of the SAME size are copies — that is how JungleRuins' 6 trees × 65 were found
(`docs/scene-loaders-usd.md`, item `geometry-content-dedup`).

The console answers after the frame: a scene that loses its device right after loading cannot be asked. A demo can
write the report itself at the end of its load (`device()->memoryDetailedStatisticsJSON()`).

## Staging buffers — released after the upload (2026-10-12)

Item `staging-buffers-retained-after-load`. Every upload goes through a `Vulkan::TransferManager` transfer operation:
a host-visible staging buffer (`BUFFER TRANSFER_SRC` for buffers, `BUFFER TRANSFER_SRC|TRANSFER_DST` for images in
the report), a command buffer and a fence. An operation is reusable once its fence has signalled. Uploads are
asynchronous, so a load with many uploads in flight creates many operations, and an operation is enlarged to the
largest upload it serves. Before this change nothing released them until the service terminated.

**The policy (owner decision 2026-10-12)** — `TransferManager::releaseIdleStagingBuffers()`, called by
`Renderer::beginFrame()` after the frame fence, next to `checkMemoryBudget()`:

| Constant | Value | Rule |
|---|---|---|
| `StagingLargeBytes` | 16 MiB | an idle operation larger than this is released at the first frame after its fence signalled |
| `StagingIdleDelay` | 5 s | an operation no upload reserved for this long is released, whatever its size |
| `StagingReleaseInterval` | 1 s | the idle rule runs at most this often; one log line per interval at most |

The pass never waits (`try_to_lock`: skipped while an upload holds the lock) and releases only an operation whose
fence has signalled. Log: `N staging buffers released (X MiB), B buffer and I image transfer operations kept.`

**Measured** (Apple M6, MoltenVK, sponza, 416-417 staging reservations per load, 3 runs each; a temporary probe traced
the pools at each reservation; "in flight" = bytes of operations whose fence had not signalled, the floor of any
policy; "after" = `getGPUMemory()` 15 s after the load):

| Policy | Peak during the load | In flight (floor) | Staging after the load | Upload span |
|---|---|---|---|---|
| none (before) | 418 / 457 / 453 MiB | 269-317 MiB | 418 / 457 / 453 MiB | 4.2 s |
| (a) idle 5 s only | 419-479 MiB | 269-317 MiB | 0 | 4.1-4.3 s |
| (b) 64 MiB cap, largest idle first | 303-322 MiB | 290 MiB | 21.3 MiB | 4.2 s |
| (c) > 16 MiB at once only | 279-281 MiB | 270 MiB | 9.8-11.7 MiB | 4.2 s |
| **(c) + (a), adopted** | **280 / 281 / 290 MiB** | 270 MiB | **0** | 4.2 s |

(b) was rejected: an arbitrary cap held for ever. (a) alone leaves the peak as it was; (c) alone keeps every small
operation for ever. Proof: citadel → sponza → citadel → sponza in one run, validation on, 0 `VUID-`, 0 `UNASSIGNED`,
the pools empty at the end (citadel alone uploads through one 480 MiB staging buffer).

## To verify

- MoltenVK and the Windows drivers: does `VK_EXT_memory_budget` report a real budget, and does VMA spill the same way?
