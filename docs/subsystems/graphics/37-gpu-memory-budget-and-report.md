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

## To verify

- MoltenVK and the Windows drivers: does `VK_EXT_memory_budget` report a real budget, and does VMA spill the same way?
