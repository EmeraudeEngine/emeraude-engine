---
id: cubemap-peak-luminance-nits
title: Cubemaps carry no peak luminance — the sky's absolute level is unanchored
status: open
priority: medium
scope: Scenes/Sky, Graphics/Texture, Resources
opened: 2026-09-08
tags: [photometry, ibl, sky]
---

# Cubemaps carry no peak luminance

> This is what remains of *Photometry phase 2* after the 2026-09-08 verification. **The other two
> parts are done and pushed:** the demos are relit in real units (the sun is `100000.0F` = 100 klx
> across `AbstractDemo`, `LightAndShadowDebug`, `Sponza`, `Collision`, `RawGeometryLoader`,
> `AnimationDebug`) and `legacyUnitCompensation` is **deleted** — `b7066e1d1`, *"Lighting: light
> generators take photometric units; drop the legacy compensation"*, 2026-07-26. Do not re-open
> either.

## Why

`peakLuminanceNits` does not exist anywhere in the engine (grep, 2026-09-08: zero hits outside
documentation). Every other light source in the engine is now absolute — lights are authored in
lux/candela, emissives are nits by the glTF spec, the exposure is a real sensor — but a cubemap
still arrives as unitless texels. Its contribution to the ambient/IBL term therefore has **no
anchor**: the same sky asset is bright or dim depending only on how it happened to be authored,
which is precisely the class of thing the photometric pass exists to remove.

## What remains

- [ ] Give a cubemap a declared **peak luminance in nits**, and scale its sampled contribution by it.
- [ ] Decide where it is declared — the sky manifest, the texture resource, or both — and who owns
      the default when an asset does not declare one.

## ⚠️ Traps

- ⚠️ **The trap the old item carried here is STALE**: it said *"`PixelFactory` has no HDR format at
  all, which bounds what can be authored"*. It does now —
  `dependencies/emeraude-base/src/PixelFactory/FileFormatHDR.hpp` (RGBE / Radiance `.hdr`). The
  authoring bound is lifted; verify what the format actually supports before designing around it
  rather than inheriting either claim.
- An absolute sky level interacts with the **RTGI ownership contract** — RTGI owns the sky's
  contribution to the indirect diffuse (`iblDiffuseWeight`). Anchoring the sky's magnitude without
  reading that contract first risks counting it twice.
- The 28 sky manifests are under a **separate owner gate** — see
  [`sky-review-28-manifests.md`](sky-review-28-manifests.md). If the declaration lands in the
  manifest, these two items touch the same 28 files.

## References

- Emissive semantics and the exporter trap now live with the loader:
  [`../../src/Scenes/Loaders/AGENTS.md`](../../src/Scenes/Loaders/AGENTS.md) § *Emissive is a
  LUMINANCE IN NITS*.
