---
id: mdi-draws-instanced-batches-wrong
title: With MDI enabled, an instanced batch would be drawn as ONE instance at LOD 0 — the MDI path ignores instancing and LODs
status: open
priority: unranked
scope: Graphics/MDI (BatchBuilder), Scenes/Scene.rendering (Opaque list)
opened: 2026-09-25
tags: [mdi, instancing, latent-bug]
---

# With MDI enabled, an instanced batch would be drawn as ONE instance at LOD 0 — the MDI path ignores instancing and LODs

## Why

By reading (2026-09-25, the terrain hang diagnosis): `MDI::BatchBuilder` hard-codes `instanceCount = 1`, always
takes `geometry(0)` (LOD 0), reads its model matrix from a per-draw SSBO instead of the instance VBO, and only builds
programs for unlit instances. Its skip list does not exclude instanced renderables. `Core/Graphics/MDI/Enabled` is
false by default (and on every known machine), so it is latent: switching it on would draw an unlit forest as one
tree per cell, at LOD 0.

## What remains

- Exclude instanced renderables (and LOD-bearing ones) from the MDI builder until it supports them, or teach it
  `instanceCount`, the instance VBO and the selected LOD. Verify on `terrain` / `forest` with MDI on.
