---
id: labyrinth-lane-parity-indirect-diffuse
title: Emissive surfaces in the indirect diffuse — SSGI counts a lamp's panel on top of the lamp, RTGI ignores every emitter
status: in-progress
priority: high
scope: Graphics/Effects/Lighting (SSGI, RTGI), lane parity
opened: 2026-10-25
tags: [gi, parity, defect]
---

# Emissive surfaces in the indirect diffuse — SSGI counts a lamp's panel on top of the lamp, RTGI ignores every emitter

## Why

Met while measuring the depth-reconstruction port (graphics doc 13-12 § 41), 2026-10-25, Linux, RTX
3070 Ti, 1920×1080, validation ON, 0 VUID. `labyrinth`, pose `Act.setPosition(-6, 0, 67.3)` +
`Act.lookAt(-6, 3.9, 66)`, exposure pinned f/2.8 · 1/30 s · ISO 400 (`Camera.setExposure(Head, Eyes, 2.8, 0.0333,
400)`), settings copies differing by `LightingLane` only. The contribution of the `IndirectDiffuse` slot = the mean
luminance (linearised: sRGB, then the inverse of the Narkowicz ACES fit) with the slot on minus with it disabled:

| Binary | ScreenSpace lane (SSGI) | RayTracing lane (RTGI) |
|---|---|---|
| before the depth port (`522e9d47`) | 0.106 | 0.0079 |
| after it | 0.127 | 0.0065 |

The frame without GI, reflections and AO is the same on both lanes (0.026–0.027): the direct light agrees, the
indirect does not. SSGI turns the lamp-lit walls bright; RTGI adds almost nothing. It predates the depth port (which
moved SSGI by +20 %, its rays now covering their true metres).

The two lanes are meant to be two costs of ONE lighting (graphics doc 13-12 § 18, § 20). Sponza's documented gap is
3.7× at its worst pose, explained by the off-screen occluder blind spot; 15–20× is beyond it.

## The cause (measured 2026-10-25)

**The lamp panels.** Same position, two framings, one launch per lane:

| Framing | SS lane: GI / direct | RT lane: GI / direct |
|---|---|---|
| looking at a lamp panel (`lookAt(-6, 3.9, 66)`) | **4.74** | 0.26 |
| pitched down, no panel on screen (`lookAt(-6, 0, 65)`) | −0.05 (noise) | 0.08 |

Without a panel on screen the lanes agree. With one, SSGI's bounce returns ~5 times the direct light, which no
single bounce of the lamps' light can do (albedo < 1): the energy is the panels' own emission. The two lanes do not
read the same thing at a bounce hit:
- **SSGI** samples the final scene colour at the hit pixel — a panel there is 7 800 nits of emission;
- **RTGI** shades the hit as `albedo / π × direct light + albedo × probe feedback`: an emitter's emission is never
  added.

In labyrinth each panel is the VISIBLE FACE of its lamp (`AbstractLightEmitter::linkEmissiveMaterial()`, L = Φ / (π A)
of that very lamp), so SSGI counts every lamp TWICE — once as the spot, once as the panel — and the second copy is not
held by the spot's cone: it floods the upper walls. It is also view-dependent: the walls brighten when a panel enters
the frame. RTGI is right for a linked panel, and would be wrong for a free-standing emitter (labyrinth's green exit
sign, a neon), which it never lets light anything.

## Decision and fix (2026-10-25)

Owner: **option A**, no patch — a light's face never re-emits, a free emitter always does, in every estimator. Carriers
S1: a runtime material UBO slot (the layout grew to 144 floats). S2: SSGI drops a light-face hit whole, documented with
its bound and its exact alternative. S3: the direct estimator for free emitters now, emissive next-event estimation under
`rt-many-lights-sampling`. Implemented and measured on Linux: graphics doc 13-12 § 42 (panel pose 7.6 / 0.42 → 0.043 /
0.041; the exit sign lights its alcove in both lanes; sponza unchanged).

## What remains

- The peers (macOS-PA, Windows-PA): the three labyrinth framings on their lanes, sponza's four poses, 0 VUID; then
  re-compare the platforms at the panel pose (before the fix: SSGI +0.129 Linux 1080p, +0.165 macOS 1440p, +0.178 /
  +0.168 Windows 720p, clipped pixels masked).
- Then close this item (its knowledge is in § 42).

## References

- The protocol: graphics doc 13-12 § 41 (two binaries, one launch per lane, each slot disabled in turn).
- `src/Graphics/Effects/Lighting/SSGI.cpp`, `RTGI.cpp`; graphics doc 13-12 § 18, § 20.
