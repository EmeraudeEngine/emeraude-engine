---
id: rtgi-bounce-hits-miss-flat-ambient
title: RTGI's bounce hits add no flat scene ambient — the only traced consumer without it
status: open
priority: unranked
scope: Graphics/Effects/Lighting/RTGI (hit shading)
opened: 2026-09-25
tags: [ray-tracing, gi, ambient, parity]
---

# RTGI's bounce hits add no flat scene ambient — the only traced consumer without it

## Why

Found by the adversarial review of the light-colour normalisation (2026-09-25), by reading: at each bounce hit RTGI
adds `albedo / PI × direct` plus the probe feedback, never `albedo × ambient / PI`. RTR adds that term at its hits
(`RTR.cpp` ~876) and so does the probe volume (`IrradianceProbeVolume.cpp` ~331, with its 1/PI since the same day).
In a hand-lit scene (the sky not driving the ambient: `collision` 15 000 lx, `light-and-shadow-debug` 120 lx) on the
traced lane, the RTGI bounce off a wall carries none of the flat ambient that RTR shows on the same wall in a mirror.
More visible since the probe ambient is no longer capped at 1 lx.

## What remains

- Add the flat ambient at RTGI's bounce hits (the effective ambient illuminance, as RTR reads it, ÷ PI, times the
  hit's diffuse albedo), check it is not counted twice with the probe feedback, and measure the intra-frame parity
  (a wall seen directly, through RTR and through a bounce) on a hand-lit bench.
