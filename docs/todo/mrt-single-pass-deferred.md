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

## Verified state (2026-09-08)

- **The MRT attachments already exist.** `Renderer.cpp:1143-1176` creates normals, material
  properties, albedo and velocity, and the shader generator detects the layout from
  `colorAttachmentCount` against a **FIXED** MRT layout. The groundwork is not missing.
- **What is still multi-pass is the LIGHTING, and that is where the draws go.** `RenderPassType`
  (`src/Graphics/Types.hpp:90-111`) enumerates 16 values — `AmbientPass` plus one family per light
  type and shadow/colour-map combination — and an object is re-rasterised **once per pass**. So
  "4 draws per object" is ambient + 3 lights, not four G-buffer subpasses. Going to 1 means
  resolving the lighting in screen space from the G-buffer, not merely merging attachment writes.

## What remains

- [ ] Write the G-buffer in one MRT pass **and resolve the lighting deferred**, so geometry is
      rasterised once regardless of the light count.

## ⚠️ Measurement protocol (mandatory, `src/Graphics/AGENTS.md`)

1. Before: `/renderdoc-capture` on the test scene, record metrics.
2. Implement.
3. After: `/renderdoc-capture` again, compare.
4. Visual output identical or better (read the thumbnail).
5. Report the delta in draw calls, render passes, vertex throughput.

No blind optimization.
