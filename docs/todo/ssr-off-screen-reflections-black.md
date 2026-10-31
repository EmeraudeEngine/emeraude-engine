---
id: ssr-off-screen-reflections-black
title: "SSR: an off-screen reflection has no screen sample and falls back to black indoors — what should a screen-space miss show?"
status: open
priority: high
scope: Graphics/PostProcess (SSREffect ray-miss fallback), environment lighting of closed scenes
opened: 2026-10-11
tags: [rendering, ssr, design-question]
---

# SSR: an off-screen reflection has no screen sample and falls back to black indoors — what should a screen-space miss show?

## Why

**Owner reading (2026-10-13):** the black comes from the lack of screen samples — a ray that leaves the screen has
nothing to read. The code agrees, and explains why the result is black rather than the environment.

### What is measured

`post-processor-effect-debug`, `Reflections` bench: six reflective panels facing the camera, mirroring the room
BEHIND it (off-screen by construction).

| Run (2026-10-10/11) | Lane / reflection occupant | Frame luma | Panels |
|---|---|---|---|
| Linux RTX 3070 Ti, `LightingLane = Auto` | RayTracing / RTREffect | 131.4 | the roughness gradient |
| Linux RTX 3070 Ti, `LightingLane = ScreenSpace` | ScreenSpace / SSREffect | 74.7 | black |
| Windows Intel UHD (no RT) | ScreenSpace / SSREffect | 74.8 (panels 6.1) | black |

### What is read in the code (re-verified 2026-10-13)

- Every SSR ray of that bench misses (the reflected room is behind the camera) and takes the miss branch:
  `envColor = textureLod(prefiltered, R, roughness × PrefilteredMaxLod) × skyLuminance`
  (`src/Graphics/Effects/Lighting/SSR.cpp:740`), weighted by `envFallbackIntensity` (0.3, `SSR.hpp:91`) × fresnel
  × roughness fade.
- `skyLuminance` is `FrameContext::skyLuminance` (`SSR.cpp:1684-1686`, passed at `PostProcessor.cpp:1459`), the scene
  background's luminance — **0 when the scene has no background** (`Renderer.cpp:97-107`, `sceneSkyLuminance()`).
  The bindless environment slot always holds something (the engine default cubemap, `+DefaultTextureCubemap` in the
  log), but it is multiplied by 0.
- The `Reflections` bench enables no background (only the `LensFlare` bench does: projet-alpha
  `src/Builtin/PostProcessorEffectDebug.cpp:59-71`) and is a room closed on every side (floor, ceiling, walls:
  `PostProcessorEffectDebug.cpp:175-192`). SSR's miss fallback is therefore 0 there by construction. RTR shows the
  panels because its rays HIT the room's walls.
- The scale by the sky luminance is deliberate (the comment at `SSR.cpp:737-739`: a normalized cubemap injected
  into a chain that lives in nits was a constant, invisible under a bright sky); it mirrors the RTR miss path.

So the slot is not broken: an off-screen reflection has no screen sample, and the only fallback is the sky, which
does not exist indoors.

### Not measured yet

- The miss branch was not isolated at runtime (e.g. a forced `skyLuminance` or a debug output of the miss
  confidence): the reading above is code only.
- `labyrinth`'s black ceiling on the Intel iGPU (no RT, so no RTGI) may be the same mechanism; unchecked.

### A real case: a glossy shop window in daylight (2026-10-29, corrected 2026-10-31)

projet-alpha `city`, by day, the shopfront glass (dark, roughness 0.25): three owner captures a few degrees of camera
rotation apart (`1793277803/25/45.png`) flipped a window between a bright reflection and black; it was blamed on SSR.
⚠️ Corrected 2026-10-31: the owner's machine runs the RAY-TRACING lane there (`LightingLane = Auto`,
`PostProcess.getStatus()`: Reflections = RTREffect), so these captures were never SSR; on RTR the flip was not
reproduced (smooth glass 0.08, a 5-step yaw sweep and a 12-frame drive). On the SCREEN-SPACE lane the same smooth window
stays grey from the street: everything it mirrors is behind the camera, every ray misses, and the miss gives the sky at
`envFallbackIntensity` 0.3 — this item's question, met outdoors.

A candidate change, written and NOT adopted (2026-10-31, kept off the tree): the resolve BLENDS the hit and the miss —
the environment takes the part of the lobe a hit's confidence (distance, edge, facing fades) leaves, instead of a pixel
being either a hit or a miss (Stachowiak, "Stochastic Screen-Space Reflections", SIGGRAPH 2015 Advances: SSR confidence
over the reflection probes). It leaves a full-trust hit bit-identical (confidence <= roughness fade). Its A/B on the city
shop window (screen-space lane, 5 poses) measured no difference (±1 level): that pose's rays all miss. To adopt it, find a
bench where hits fade at the screen edge under a sky, and measure.

## What remains

1. **Owner decision — what should a screen-space miss show?** Options:
	- **A local probe / reflection probe** feeding the SSR miss (the engine already has the dynamic cubemap probe,
	  `docs/reflection-pipeline.md` § 2.3, but per material and suspended while SSR/RTR is active): physically the
	  right answer indoors; costs probe placement and renders, and a binding path into SSR.
	- **The RT fallback where available** (RTR on the miss rays only): exact, but absent precisely on the machines
	  that run the screen-space lane (Apple GPUs, Intel iGPU, low profile) — it does not solve the reported case.
	- **A fallback that does not depend on the sky** (e.g. the ambient term, or the prefiltered environment scaled by
	  something other than `skyLuminance`): cheap and never black, but not physical; risks the "plain constant" the
	  current scaling was introduced to remove.
	- **Accept black as the contract of a closed scene without a probe**, and document it (a scene author adds a probe
	  or a background): zero cost, but every no-RT machine keeps black reflections of off-screen content indoors.
2. Once decided: implement, then re-measure the three rows above (pinned exposure, per-pixel panel values).
3. Check `labyrinth` on the Intel iGPU against the same reading (background? `skyLuminance`? which lane?), whatever
   the decision.

## ⚠️ Traps

- The frame mean luminance barely moves with the panels (74.7 vs 131.4 includes the whole room): measure the panel
  pixels, not the frame.
- `+DefaultTextureCubemap` in the log does not mean the fallback contributes: it is scaled by `skyLuminance`.

## References

- `src/Graphics/Effects/Lighting/SSR.cpp:727-748` (miss branch), `SSR.hpp:91`, `SSR.cpp:1684-1686`
- `src/Graphics/PostProcessor.cpp:1459`, `src/Graphics/Renderer.cpp:97-107`
- `docs/reflection-pipeline.md` (the reflection paths, the cubemap probe and its suspension rule)
- projet-alpha `src/Builtin/PostProcessorEffectDebug.cpp` (the bench)
