---
id: terrain-rt-triangle-list-spam
title: Terrain adaptive grid spams generateTriangleListIndicesForRT() with RT enabled
status: open
priority: low
scope: Graphics/Terrain
opened: 2026-08-12
tags: [ray-tracing, log-noise]
---

# Terrain adaptive grid spams generateTriangleListIndicesForRT() with RT enabled

## Why

With ray tracing enabled, the terrain adaptive grid logs
`generateTriangleListIndicesForRT() returned empty indices` **~18,000 times per minute**:
TriangleStrip geometry has no RT triangle-list path. Dates from `44cb3a85` (2026-03-10).

Pre-existing and unrelated to the material merge that surfaced it.

## What remains

- [ ] Either give TriangleStrip geometry a real triangle-list conversion for RT, or stop asking
  for one — and in both cases stop the per-frame log flood.
