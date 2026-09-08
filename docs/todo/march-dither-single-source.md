---
id: march-dither-single-source
title: Four effects open-code the march-origin dither — migrate them onto the shared rule
status: open
priority: low
scope: Graphics/Effects/Framebuffer (MotionBlur, VolumetricLight, RTR, SSR)
opened: 2026-09-08
tags: [shaders, reuse, post-processing]
---

# Four effects open-code the march-origin dither

## Why

`EMEN_MARCH_DITHER_GLSL`
([`../../src/Graphics/Effects/Framebuffer/MarchDitherGLSL.hpp`](../../src/Graphics/Effects/Framebuffer/MarchDitherGLSL.hpp))
now holds the engine's one march-origin dither, and `VolumetricScattering` consumes it. Four older
effects still carry their own copy of the same expression:

| Site | Shape |
|---|---|
| `MotionBlur.cpp:283` | already a named `interleavedGradientNoise(vec2)` function |
| `VolumetricLight.cpp:165` | inline, on `gl_FragCoord.xy` |
| `RTR.cpp:1701` | inline, wrapped in `6.2831853 * …` for an angle |
| `SSR.cpp:264` | already a named function, on a `pixel` parameter |

All four are **bit-identical** to the shared helper: same constants (`52.9829189`, `0.06711056`,
`0.00583715`), same `fract(a * fract(dot(p, b)))` shape. So the migration is a behaviour-preserving
no-op, and the point of doing it is that the rule stops being able to drift — the same reasoning
that made the light-space transform's four copies a defect
([`light-space-transform-single-source.md`](light-space-transform-single-source.md)).

## What remains

- [ ] Splice `EMEN_MARCH_DITHER_GLSL` into each of the four shaders and call
      `emInterleavedGradientNoise()`.
- [ ] Drop the four local definitions.

## ⚠️ Traps

- ⚠️⚠️ **These shaders are compiled at RUNTIME, not at build time.** A typo in one of the four GLSL
  literals produces a shader that fails to compile *while the demo runs* — the C++ build stays
  green and says nothing. Each migrated effect needs its own launch, and the log read for a
  compile failure, before it is called done.
- The expected result is a **bit-identical** frame. Verify it as a no-op capture (pinned exposure,
  pinned pose), not by eye: "it still looks right" does not distinguish a working dither from a
  dither that silently returned a constant.
- ⚠️ Do not "improve" the constants while moving them. A different noise field changes the
  temporal-stability signature of four effects at once, which is a measurement, not a refactor.
