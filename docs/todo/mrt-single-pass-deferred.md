---
id: mrt-single-pass-deferred
title: MRT single-pass deferred — 4 draws per object down to 1
status: open
priority: high
scope: Graphics/Renderer
opened: unknown
tags: [performance, deferred, gpu]
---

# MRT single-pass deferred — 4 draws per object down to 1

## Why

P1 of the Graphics optimization roadmap (`src/Graphics/AGENTS.md` § Optimization Roadmap): the
G-buffer is filled through multiple subpasses, **4 draws per object**, where a single-pass MRT
G-buffer needs **1**. Expected impact: −75 % geometry draws. UE5 does it in one pass.

## What remains

- [ ] Write the G-buffer in one MRT pass.

## ⚠️ Measurement protocol (mandatory, `src/Graphics/AGENTS.md`)

1. Before: `/renderdoc-capture` on the test scene, record metrics.
2. Implement.
3. After: `/renderdoc-capture` again, compare.
4. Visual output identical or better (read the thumbnail).
5. Report the delta in draw calls, render passes, vertex throughput.

No blind optimization.
