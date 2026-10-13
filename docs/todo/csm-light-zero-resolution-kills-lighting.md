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

Priority: medium by owner decision (2026-10-13), although a defect.

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
"no shadow maps" switch) became fatal. That consumer no longer has the switch (2026-10-13: its sun is always a
2048-px CSM, option 0 now toggles a colour projection, projet-alpha `src/Builtin/LightAndShadowDebug.cpp:108`), so
no demo exercises the configuration today — the engine-side gap is unchanged.

**A second route to the same silence (measured 2026-09-25, `terrain`, frozen noon sun, pinned exposure):**
with `Core/Graphics/ShadowMapping/Enabled = false`, the CSM sun lights NOTHING at all. The sand in direct light
fell from 112 to 8.6 / 255. So the non-shadow `DirectionalLightPass` of a CSM-built light contributes nothing,
with no warning and no VUID. That is very likely the same mechanism as resolution 0 (a CSM light with no usable
map), unverified. Found by the foliage diagnosis workflow of that day.
That second route is tracked on its own, with its pinned-exposure captures, in
[`sun-unlit-without-shadow-mapping.md`](sun-unlit-without-shadow-mapping.md). Fixing either one must re-test the other.

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

## References

- `src/Scenes/Toolkit.hpp:672` (the CSM `generateDirectionalLight` overload), `src/Scenes/Component/DirectionalLight.hpp:94-100`
  (the CSM constructor, no resolution check), `src/Scenes/Component/DirectionalLight.cpp:258` (the shadow map is
  created only when the resolution is above 0).
