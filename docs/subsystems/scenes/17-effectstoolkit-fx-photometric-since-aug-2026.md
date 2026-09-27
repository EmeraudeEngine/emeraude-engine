## EffectsToolkit/FX — photometric since Aug 2026

`EffectsToolkit::FX::createFlashEffect()` was the LAST authoring entry point still taking raw
candela after the photometric migration converted `Toolkit::generate{Point,Spot,Directional}Light()`
to authoring units. It now takes **lumens** and converts once (the keyframes drive `setIntensity()`,
which is candela, so converting per interpolated frame would be waste).

Its keyframes are also the reference shape for a **detonation**, and the reason is a contract
change nobody sees coming:

> [!CAUTION]
> **Do not shape a light effect with its RADIUS.** Under the windowed inverse square
> (`Graphics/Effects/Shared/LightFalloffGLSL.hpp`, the raster and traced lanes alike) the radius is
> `saturate(1 - (d/r)^4)^2`, a culling window that sits near 1.0 over most of the range; the
> falloff is `1 / max(d^2, 0.01^2)`, distance only. (It was lost from 2026-08-12 to 2026-09-25, when the radius
> DID dim.) Animating the radius — the natural move under the old
> `max(1 - (d/r)^2, 0)` falloff — brightens nothing and just pops the hard cut in and out. The
> envelope belongs in the intensity. Full write-up in `docs/caution-points.md` § "The light RADIUS
> is a culling bound, not a dimmer".

The flash also ramps its COLOUR (white hot → yellows → the caller's settling tint), which
`Component::PointLight` supports natively: `playAnimation()` handles the `Color` id and `Sequence`
interpolates `Variant`s of type `Color` (linear and cosine).
