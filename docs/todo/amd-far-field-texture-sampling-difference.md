---
id: amd-far-field-texture-sampling-difference
title: The far field of a tiled ground samples differently on AMD, with repeat lines toward the horizon
status: parked
priority: high
scope: Graphics/TextureResource samplers (anisotropy, LOD), Vulkan/Sampler
opened: 2026-09-22
tags: [amd, sampling, cross-platform-bench]
---

# The far field of a tiled ground samples differently on AMD, with repeat lines toward the horizon

## Parked

No AMD machine available (owner, 2026-10-13); it must be tested on one when available. The part visible on every
device (dark dashes under the horizon) is `far-field-dark-dashes`, workable now.

## Why

Windows peer session, 2026-09-22, `relief` at the same pose on the RTX 3060 Laptop and the AMD iGPU of the
same machine. Both are deterministic run to run (|Δ| < 1/255). The near and mid bands have the same means,
but the FAR band (y 170-260) differs:
- POM: NV 219.0, AMD 206.2, gradient 6.94 against 8.79;
- normal mapping: NV 218.5, AMD 204.0.
So it is NOT the relief technique. AMD shows a visible grid of texture-repeat lines converging toward the
horizon, in both modes. Near and mid, |Δ| is ~12 with 25 % of the pixels above 16: a real per-pixel
difference.

The dark dashes seen on EVERY device (2026-09-23) may be one symptom of the same cause, or a separate one: settle
`far-field-dark-dashes` first, then compare AMD against it.

## What remains (on an AMD machine)

- Re-take the 2026-09-22 comparison on the current engine (`TextureCache::Version` 3, exact box-filtered mips, since
  2026-09-23).
- Compare the sampler state both drivers get (anisotropy `Core/Graphics/Texture/AnisotropyLevels` 8, mip LOD bias, the
  mip chain of the 1024² Pavement006 maps).
- Decide which one is right with a controlled ground (a checkerboard at a known frequency).

## References

- `far-field-dark-dashes`.
