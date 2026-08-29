---
id: volume-absorption-probe-cannot-discriminate
title: VolumeAbsorptionProbe no longer separates its three spheres — it needs a bright background
status: open
priority: medium
scope: tools/gltf-conformance-bench
opened: 2026-08-29
tags: [measured, bench, transmission, methodology]
---

# `VolumeAbsorptionProbe` no longer discriminates

## Why

The probe carries three glass spheres — `NoVolume`, `ColourNoDistance` (attenuation colour, no
distance, so `+INFINITY` and per spec **no** tint) and `ColourAndDistance` (colour `0.05, 0.6, 0.25`
at distance `0.25`, thickness `1.0`). Its whole point is that only the third may be tinted.

It used to separate them: measured `(161,175,155)` for the third against `(219,220,220)` for the two
controls. After the transmission composition was corrected (Aug 2026, transmission moved to the
ambient pass alone) the three come out **identical to within 0.5 of a code value** — pixel-wise
central crops of `(28.55,26.33,13.53)`, `(28.48,25.82,13.59)`, `(29.09,25.90,13.71)`.

**That is not proof of an absorption regression.** The green used to come from the *additive
light-pass term* — `albedo × beerAbsorption × radiance` under a 100 000 lux key — which is exactly
the defect that was removed. The probe was discriminating **through** the defect. With it gone the
pixel is dominated by `reflectedColor * fresnelDielectric`, and the transmitted term the probe is
supposed to read is sampled from a **near-black background**: the spheres float against dark trees,
so absorbing 99 % of almost nothing is invisible.

## What remains

- [ ] Give the probe a **bright, opaque backdrop** behind the spheres — the way `TransmissionTest`
  puts a lit checkered cloth behind its own. Absorption is a multiplication; it needs something to
  multiply. Generate it in `make-volume-probe.py` so the asset stays deterministic.
- [ ] Re-derive a numeric criterion once the backdrop exists: the third sphere's G/R ratio against
  the two controls, measured on a crop **fitted to the sphere** and not guessed.
- [ ] Separately, quantify how much `reflectedColor * fresnelDielectric` contributes at normal
  incidence — see the unit-asymmetry note below; it may be swamping transmission on every
  reflective+transmissive material, not just here.

⚠️ Meanwhile the bench is **not** without a transmission criterion: `TransmissionRoughnessTest`'s
IOR sweep discriminates refraction cleanly (see the bench README), and `TransmissionTest` is a
bit-exact thin-walled control. What is missing here is specifically an **absorption** criterion.

## ⚠️ Traps

- ⚠️⚠️ **A control that discriminates through a defect stops discriminating when the defect is
  fixed, and its silence then reads as a regression.** Check what the criterion actually measured
  before declaring one.
- ⚠️ **Fit the crop to the image.** Guessed sphere centres put the first "measurement" on the
  background and reported the two runs as byte-identical. The reliable route: threshold the
  before/after diff, take the column runs and the row centroid of the mask.
- ⚠️ **Unit asymmetry, worth its own investigation.** In the ambient composition
  `reflectedColor` is a cubemap texel scaled by `iblIntensity * environmentLuminance`, while
  `transmittedLight` is a grab-pass sample **already in nits** and deliberately unscaled
  (`transmissionIsSceneRadiance`). The two are added with Fresnel weights; if the scale is off the
  reflection wins everywhere.

## References

- `tools/gltf-conformance-bench/make-volume-probe.py`, `bench.py`.
- `Saphir/LightGenerator.cpp::generateAmbientFragmentShader()` (the three transmission sites).
