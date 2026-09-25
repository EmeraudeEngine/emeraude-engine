---
id: vram-budget-observability
title: Log the VRAM budget (VK_EXT_memory_budget) — a paging frame is invisible today
status: open
priority: unranked
scope: Vulkan/Device (VMA budgets), Graphics/Renderer status
opened: 2026-09-25
tags: [vulkan, memory, diagnostics]
---

# Log the VRAM budget (VK_EXT_memory_budget) — a paging frame is invisible today

## Why

The terrain hang diagnosis (2026-09-25) could not rule out VRAM exhaustion on the 6 GB RTX 3060 Laptop: when the
working set passes the budget, WDDM pages allocations to system memory and a frame can take tens of seconds with no
device loss. Nothing in the engine reports the heap budgets, so such a run looks like a GPU hang.

## What remains

- Query the VMA heap budgets (backed by `VK_EXT_memory_budget` where available — check MoltenVK) at scene load and
  every N seconds; add them to `Core.RendererService.getStatus()`; warn once when usage exceeds the budget.
