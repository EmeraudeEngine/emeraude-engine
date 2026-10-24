---
id: dynamic-sky-atmosphere
title: Dynamic sky — a physical, planet-parametric atmosphere drawn as the background, with the sun disc
status: blocked
priority: unranked
scope: Graphics/Renderable (DynamicSkyResource, AbstractBackground), Graphics (new atmosphere LUT passes), Saphir (background program), Scenes (DefinitionResource background)
opened: 2026-10-24
blocked-by: [scene-colour-pre-exposure]
tags: [sky, atmosphere, photometry, background, owner-decision, design]
---

# Dynamic sky — a physical, planet-parametric atmosphere drawn as the background, with the sun disc

> **Owner decisions (2026-10-24):**
> - The sky is **PHYSICAL**, and it must be able to render **extraterrestrial skies**. The
>   atmosphere is described by its planet, not hard-wired to Earth.
> - The exposure work comes **first**: this item waits on `scene-colour-pre-exposure`.
> - `SkyFollowsSun` **stays**, for the picture skies (`SkyBoxResource`). It does not drive a
>   dynamic sky.
> - Time comes from a celestial clock (`celestial-clock`), not from `SunCourse`'s fixed great
>   circle. This first lot may be driven by `SunCourse` until the clock exists.

## Why

`DynamicSkyResource` (`src/Graphics/Renderable/DynamicSkyResource.cpp`) is an empty shell: both
`load()` overloads end in `setLoadSuccess(false)`. Today a sky is a picture (`SkyBoxResource`) that
`SkyFollowsSun` can only dim. It cannot turn orange at sunset or blue at dawn, and it cannot show
a sun disc that matches the directional light. This item is the first lot of the dynamic sky. The
following lots are `dynamic-sky-sun-light-from-transmittance`, `dynamic-sky-amortised-ibl`,
`dynamic-sky-stars-and-milky-way`, `dynamic-sky-moon`, `celestial-clock` and
`dynamic-sky-aerial-perspective`.

## What remains

- [ ] **The model: Hillaire 2020**, the one behind UE's `SkyAtmosphere`. The LUTs:
  - transmittance (~256×64), rebuilt only when the atmosphere parameters change;
  - multiple scattering (~32×32), rebuilt at the same rate;
  - sky-view (~192×108), rebuilt every frame from the sun direction and the camera altitude.
- [ ] **Planet parameters, so an alien sky is data, not code**: ground radius, atmosphere top,
      Rayleigh scattering and its scale height, Mie scattering/absorption, its scale height and its
      phase anisotropy, the absorption layer (Earth: ozone, a tent profile) and the ground albedo.
      Ship an `Earth` preset (Hillaire's / Bruneton's values) and at least one alien one (e.g. a
      Mars-like dusty atmosphere: butterscotch day, blue sunset) as the test of the decision.
- [ ] **Star spectrum**: the sun's colour and illuminance at the top of the atmosphere come from
      its `CelestialBody` (a red dwarf or a blue star is data). Several suns are allowed by the
      contract (`AbstractBackground::stars()`). Decide with the owner whether lot 1 renders one or
      N scattering sources (the cost is linear in the sky-view pass).
- [ ] **The draw**: a full-screen triangle that rebuilds the view ray from the inverse
      view-projection, shaded only where the depth is at the far plane. No cube, so no seams, and
      only the sky pixels are shaded. It is still an infinity-view background
      (`AbstractVertexStage.cpp:1012`): rotation only, never clipped by the far distance.
- [ ] **The sun disc**: the angular diameter from `CelestialBody` (0.53° on Earth), limb
      darkening, and the transmittance along the view ray, so it reddens and dims on the horizon by
      itself.
- [ ] **The manifest**: `load(const Json::Value &)` reads the planet (a preset name or inline
      values) and the bodies, from the `Backgrounds` store like every other background
      (`Resources/Manager.cpp:862`, `Scenes/DefinitionResource.hpp:225`).
- [ ] **A component carries the TIME, the resource only the description**: the resource is
      shared and static, while the sun direction changes every cycle. This follows the 2026-09-13
      split (`SunCourse` = the sun only, the sky belongs to the background).
- [ ] **Artistic controls on top of the physics**: a tint, a saturation and an intensity scale.
      They are multiplicative and default to neutral, so the physical result is the reference.
- [ ] A doc `docs/subsystems/graphics/` (the model, the units, the presets) and the console
      getters to measure it (sun elevation, sky luminance at the zenith and at the horizon).

## ⚠️ Traps

- ⚠️ **The sun disc is ~1.6e9 nits at the zenith; fp16 stops at 65 504.** Without pre-exposure,
  the disc is +Inf in the scene colour, the same overflow class `LensFlare.cpp:52-56` works around
  locally. This is why the item is blocked.
- ⚠️ **The night must still be readable**: a moonless sky is ~1e-3 nits and less. The auto-exposure
  multiplier is clamped at `maxExposure` 100 (`ToneMapping.hpp:122`). A physically dark night
  reads black, so the owner has to choose between a raised floor and an artistic night lift
  (see `dynamic-sky-stars-and-milky-way`).
- ⚠️ **The sky-view LUT depends on the camera altitude.** On the ground it is constant, but a
  flight scene must feed the real altitude above the planet's ground radius, not the
  engine's Y.
- ⚠️ **TAA and motion vectors**: the background's velocity is synthesized apart from
  `gl_Position` (`synthesizeVelocityClipPositions`). A full-screen draw must keep a rotation-only
  velocity, or the sky will smear when the camera turns.

## References

- S. Hillaire, *A Scalable and Production Ready Sky and Atmosphere Rendering Technique*, EGSR 2020.
- E. Bruneton, F. Neyret, *Precomputed Atmospheric Scattering*, EGSR 2008 (the Earth constants).
- `src/Graphics/CelestialBody.hpp`, `src/Graphics/Renderable/AbstractBackground.hpp`,
  `src/Scenes/Component/SunCourse.hpp`, `src/Scenes/Component/SkyFollowsSun.hpp`.
