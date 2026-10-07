---
id: rt-light-ssbo-rewritten-while-in-flight
title: The RT light SSBO is one host-mapped buffer rewritten while frames in flight read it, from live coordinates
status: open
priority: high
scope: Scenes/LightSet, Graphics/Effects/Lighting (RTGI, RTR, probes)
opened: 2026-10-04
tags: [ray-tracing, lighting, synchronisation]
---

# The RT light SSBO is rewritten while frames in flight read it

## Why

`LightSet::updateVideoMemory()` maps and rewrites `m_RTLightSSBO` — ONE host-visible buffer — every frame, while the
previous frames in flight may still be reading it in RTGI / RTR / the irradiance probes. It also reads the LIVE
`getWorldCoordinates()` of each light (logic thread state), where the raster light UBOs read the PUBLISHED block of the
frame's render state (`AbstractLightEmitter::publishedBlock()`), so a moving light is placed differently by the traced
lanes and by the raster in the same frame.

Found by the G-buffer audit of the deferred resolve (2026-10-04). The resolve does NOT use this buffer: it owns one
buffer per frame in flight filled from the published blocks (`Graphics::DeferredLightResolve::prepare()`), which is
the pattern to follow here.

## What remains

- One buffer (or one region) per frame in flight, written for the frame being recorded.
- Fill from the published blocks of the frame's render state, like the raster and the deferred resolve.
- Prove it: a fast-moving light, RT lane, compare the reflection / GI light position with the raster's (and the
  synchronisation validation layer on the old code, if it reports the hazard).
