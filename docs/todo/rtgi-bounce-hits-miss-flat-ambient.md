---
id: rtgi-bounce-hits-miss-flat-ambient
title: RTGI's bounce hits add no flat scene ambient — the only traced consumer without it
status: open
priority: high
scope: Graphics/Effects/Lighting/RTGI (hit shading)
opened: 2026-09-25
tags: [ray-tracing, gi, ambient, parity]
---

# RTGI's bounce hits add no flat scene ambient — the only traced consumer without it

## Why

Found by the adversarial review of the light-colour normalisation (2026-09-25), by reading: at each bounce hit RTGI
adds `albedo / PI × direct` plus the probe feedback, never `albedo × ambient / PI`. RTR adds that term at its hits
(`src/Graphics/Effects/Lighting/RTR.cpp:897`) and so does the probe volume (`src/Graphics/IrradianceProbeVolume.cpp:348`,
with its 1/PI since the same day). RTGI's hit radiance is `src/Graphics/Effects/Lighting/RTGI.cpp:701` (direct + probe
feedback only).
In a hand-lit scene (the sky not driving the ambient: `collision` 15 000 lx, `light-and-shadow-debug` 120 lx) on the
traced lane, the RTGI bounce off a wall carries none of the flat ambient that RTR shows on the same wall in a mirror.
More visible since the probe ambient is no longer capped at 1 lx.

## What remains

- Add the flat ambient at RTGI's bounce hits (the effective ambient illuminance, as RTR reads it, ÷ PI, times the
  hit's diffuse albedo), check it is not counted twice with the probe feedback, and measure the intra-frame parity
  (a wall seen directly, through RTR and through a bounce) on a hand-lit bench.
