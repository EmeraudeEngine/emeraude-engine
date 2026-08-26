---
id: mdi-wrong-texture-after-first-frame
title: MDI — opaque meshes sample the wrong texture after the first frame
status: open
priority: medium
scope: Graphics/MDI
opened: 2026-06-01
tags: [mdi, bindless, measured]
---

# MDI — opaque meshes sample the wrong texture after the first frame

## Why

MDI-rendered opaque meshes (palm trunks in `basic-scenery`) sample the WRONG texture after the
first frame: silhouette and depth are correct, but the surface shows the **skybox**.

**Confirmed MDI-specific** (MDI off → renders correctly). ⚠️ An earlier note blaming
"StandardResource without lighting, not MDI-related" was **WRONG**.

Lead: `VUID-vkCmdDraw-None-09600` (a sampled image is in the wrong image layout for MDI draws) ⇒
the bindless-texture integration for MDI objects is incomplete — MDI bypasses the per-renderable
`render()` where textures are normally resolved and transitioned.

## What remains

- [ ] RenderDoc or targeted instrumentation to pinpoint it. ⚠️ Code speculation already produced
  one wrong hypothesis here.

## Context

MDI is kept **OFF by default** (`Core/Graphics/MDI/Enabled`). The owner sees it as a niche tool
(large near-static heterogeneous scenes — foliage, crowds, rubble), at odds with the dynamic
per-object scenes the engine targets. It also needs `shaderInt64` and `drawParameters`. Four other
MDI bugs were fixed in `f61c91c7` (normal matrix from the world model matrix, std430 push-constant
alignment ×2, and binding the bindless set 2).

## References

`docs/todo/indirect-draw-batching.md`, `project` capture tooling in
`docs/renderdoc-*` / `/renderdoc-capture`.
