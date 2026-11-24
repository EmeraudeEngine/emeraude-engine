---
id: temporal-capture-does-not-name-who-raised-a-mark
title: A temporal capture says a frame is marked, never WHO raised the lighting mark
status: open
priority: unranked
scope: src/Graphics/FrameCapture.cpp, src/Graphics/Renderer (latchTemporalDiscontinuity), src/Scenes/Scene, src/Scenes/LightSet (the epoch's producers)
opened: 2026-11-23
tags: [tooling, temporal-discontinuity, console]
---

# A temporal capture says a frame is marked, never WHO raised the lighting mark

## Why

`Renderer.temporalCapture` writes `"temporalDiscontinuity": true | false` per frame (`src/Graphics/FrameCapture.cpp:614`)
— the only readout of the temporal-discontinuity contract (graphics doc 13-12 § 43). The mark has nine producers (a
light added, an emission step, the ambient, a background, two IBL paths, the sky's immediate cycle, another scene, the
public `Scene::raiseTemporalDiscontinuity()`); the JSON does not say which one raised. Three investigations ended on
"likely, not proven" for that reason alone:

- 2026-11-03, an AMD peer: a second raise 0.1 to 1.3 s after a time-of-day jump in 6 of 26 — "the street lamps'
  switch is the likely source; not proven";
- 2026-11-22, a Windows peer: every frame of a capture marked around a parked vehicle, none at speed — reported as
  "worth a look";
- 2026-11-22, two peers: marks before the command of the capture and a run of 25 three seconds later — "most likely
  the traffic signals switching colour".

Each time the reader had to guess from what the scene contains.

## What remains

- An owner decision on the transport. The epoch is ONE monotonic sum on purpose (owner, 2026-11-03: no per-light table
  on the render thread). A proposal that keeps it: a small bit set of producer KINDS published with each render state
  slot beside the epoch (cleared at the publication), latched with it, and written in the capture's JSON as a list
  (`"temporalDiscontinuityRaisedBy": ["emission-step", "public-raise"]`) on the frame the raise lands.
- For an emission step, whether the capture also names the LIGHT (a name published once per raise costs a string per
  slot: the kind may be enough).
- `docs/ai-runtime-control.md` § Temporal capture describes the JSON: it follows.

## ⚠️ Traps

- A raise lasts 25 frames: only the frame where it LANDS has a producer; the 24 that follow are its tail.
- The view discontinuity (`"viewDiscontinuity"`) has one producer and needs nothing.

## References

- `docs/subsystems/graphics/13-12-post-processing-effects/43-the-temporal-discontinuity-contract.md` (the producers'
  table, the transport, the three observations above).
