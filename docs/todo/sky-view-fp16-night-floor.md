---
id: sky-view-fp16-night-floor
title: The physical sky goes to an exact zero below a −12° sun — the sky-view table's half floats underflow
status: open
priority: high
scope: Graphics/SkyAtmosphere (sky-view table format, the radiance's scale)
opened: 2026-10-25
tags: [sky, atmosphere, precision, night, defect]
---

# The physical sky goes to an exact zero below a −12° sun — the sky-view table's half floats underflow

## Why

Measured in sky-lab (Linux, 2026-10-25): Earth, 45° N, day 15 (new moon), camera at the zenith, post-processor
off, the grading raised until the bytes read (the doc 42 nits protocol, extended):

| Sun | Zenith luminance |
|---|---|
| 0° | 187 cd/m² |
| −3° | 31 |
| −6° | 1.03 |
| −9° | 0.039 |
| −12° | 0.0005: R = G = 0, B = 0.00729 nits only |
| −15°, −18°, −24° | **exactly 0**, even at a grading of 2^42 |

At −12° the only non-zero value is ONE quantum of the smallest half float: 2^-24 (5.96e-8, the sky-view's
radiance PER LUX) × 134 870 lx × the star's unit-luminance blue (0.912) = 0.00733 nits, against 0.00729 measured.
The sky-view table is `VK_FORMAT_R16G16B16A16_SFLOAT` (`SkyAtmosphere.cpp`, `m_skyView`) and stores the in-scattered
radiance per lux of the star: everything under 6e-8 per lux, i.e. under ~0.008 nits once multiplied, flushes to zero,
and the deep twilight (−9° to −18°) is quantised to a few levels before it vanishes. A real moonless zenith is ~2e-4
cd/m² (~22 mag/arcsec²), below that floor.

This blocks the night decision of `dynamic-sky-stars-and-milky-way` (night lift vs `maxExposure`): the night must be
measured on a sky that does not underflow.

## What remains

- [ ] Choose the fix (owner): the sky-view in `R32G32B32A32_SFLOAT` (192×108 texels: 332 KB instead of 166 KB, the
      sampling cost to measure), or keep half floats with a stored scale (the table × 2^k, the material divides),
      or store the radiance already × the star's illuminance (pre-exposed). Check the transmittance and the
      multiple-scattering tables for the same floor (half floats too).
- [ ] Re-measure the table above to −24°, with the same protocol; record the twilight curve in graphics doc 42.
- [ ] The scene colour target is half float as well: check that a night sky × E_pre survives it.

## ⚠️ Traps

- The nits protocol needs a grading far above 1 at night (2^6 at −12°): the intensity is not clamped
  (`SkyAtmosphereResource::setGrading()`), only floored at 0.
- A `setDate()` with a FRACTIONAL day shifts the time of day (the fraction adds to it): bisect the time on an integer
  day.

## References

- Graphics doc 42 (`42-physical-sky-atmosphere.md`), § "Measuring a sky in nits"; `dynamic-sky-stars-and-milky-way`.
- projet-alpha `src/Builtin/SkyLab.AGENTS.md` (the night measurement).
