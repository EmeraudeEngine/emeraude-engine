---
id: csm-light-zero-resolution-kills-lighting
title: A CSM directional light with shadowMapResolution 0 stops lighting entirely
status: open
priority: medium
scope: Scenes/Component/DirectionalLight, Scenes/Toolkit
opened: 2026-09-08
tags: [shadow-map, robustness, measured]
---

# A CSM directional light with shadowMapResolution 0 stops lighting entirely

## Why

`Toolkit::generateDirectionalLight` has two shadowed overloads and they behave differently at
resolution 0:

- **Classic** `(name, colour, lux, shadowMapResolution, coverageSize)` — resolution 0 means "no
  shadow map", the light still lights. Demos use it as their "shadows off" switch.
- **CSM** `(name, colour, lux, shadowMapResolution, cascadeCount, lambda, csmScale)` — resolution 0
  builds a cascaded light with no map, and the light contributes **nothing at all**.

Measured 2026-09-08 on `light-and-shadow-debug`, pinned pose `setPosition(-4, 2, 8)` /
`lookAt(-4, 2, 1)`, pinned exposure, same binary: frame mean luminance **56.31 → 4.04 / 255** when
the sun's resolution went to 0 under the CSM overload. Not "shadows dropped" — the scene goes dark,
leaving roughly the ambient term alone. Zero VUID, no warning, no log line: silent.

⚠️ Found because `LightAndShadowDebug` was switched to a CSM sun and its option 0 (the scene's
"no shadow maps" switch) became fatal. projet-alpha now keeps the CLASSIC overload for that option
so the scene cannot build the configuration — that is a workaround in the consumer, and the
engine-side gap is this item.

## What remains

- [ ] Decide the contract and enforce it in ONE place: either a CSM light with resolution 0 lights
      without shadows exactly like the classic one, or the constructor refuses the configuration
      loudly instead of building a light that silently contributes nothing.
- [ ] Whichever is chosen, `Toolkit::generateDirectionalLight` must not accept a configuration that
      produces a silent no-op.

## ⚠️ Traps

- ⚠️ `Toolkit::generateDirectionalLight` selects CSM purely by **argument count** — `(…, 2048, 4,
  0.5F)` is CSM, `(…, 2048, 140.0F)` is the classic map. The two are one keystroke apart and the
  failure is silent, which is what let this reach a demo. Recorded in
  [`../shadow-mapping.md`](../shadow-mapping.md).
- ⚠️ Do not use "resolution 0" as an isolation instrument for a shadow measurement on a CSM light —
  it changes the lighting, not just the shadowing, so any A/B built on it is confounded.
