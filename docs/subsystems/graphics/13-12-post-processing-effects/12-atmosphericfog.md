## 12. Post-Processing Effects

### AtmosphericFog (Exponential Height Fog)

Single-pass analytical fog using closed-form integral (no iterative sampling). Reads depth buffer to reconstruct world-space positions, applies exponential height fog with directional inscattering.

**Algorithm:**
1. Reconstruct world position from depth + camera basis vectors (push constants)
2. Exponential height fog integral: `ρ(y) = density * exp(k * (y - baseHeight))` along the view ray
3. Directional inscattering (simplified Henyey-Greenstein): bright halo when looking toward the sun
4. Sky fog option: when `skyFogEnabled = true`, fog covers skybox pixels using `maxDistance` as fictive distance

**Push constants** (116 bytes): Camera basis (pos, right, forward), depth reconstruction (near, far, tanHalfFovY, aspectRatio), fog params (density, heightFalloff, baseHeight, maxDistance, color), inscatter params (lightDir, exponent, color, intensity), skyFogEnabled.

**Height fog sign convention:** `Parameters::heightFalloff` is a **POSITIVE decay rate** — how fast density falls off going **UP** (`+Y`). The shader NEGATES it (`float k = -fogHeightFalloff;`) before feeding `exp(k · (y − baseHeight))`. Never pass a negative value.

⚠️ The negation was missing until Aug 2026, so under Y-up the fog grew **DENSER WITH ALTITUDE**. It was silent because the analytic integral below it stays valid for either sign of `k`: nothing breaks, nothing warns, the fog is simply upside down.

> [!CAUTION]
> **This section used to claim the ray reconstruction "was always sound […] it rides the signed
> `tanHalfFovY` contract and followed the flip on its own. Do not fix it in passing." That was
> WRONG, and it actively told the next reader to leave a live defect alone.** The shader rides the
> contract; the **C++ did not hand it the contract**. `execute()` recomputed
> `std::tan(fovDeg · π / 360)` locally, without `projectionYSign` — the only site in the engine that
> did — while `PostProcessor` publishes the signed value in `context.constants` and SSAO, SSGI and
> SSR all forward it. The shader's own header even states "the DOWNWARD screen direction is carried
> by the SIGN of tanHalfFovY (negative since the Y-up flip)", i.e. it documented a contract its
> caller was breaking.
>
> ⚠️ **A genuine Y-up residual, unlike two others found the same week.** Before the flip
> `projectionYSign` was `+1`, so recomputing the magnitude was *harmless*; the local recomputation
> became wrong at the exact moment of the flip. (Contrast the point-light gobo and SSR's
> camera-ward ray rejection, both of which `git log -S` places six months BEFORE the flip — check
> the history before filing a sign error under a nearby migration.)
>
> ⚠️ **Commit `2571a4b6` claimed this file** in its signed-`tanHalfFovY` pass and delivered nothing:
> it added `abs(t)` to the X terms, and `t` was already a positive magnitude, so the edit was a
> **no-op**. Its own message admitted the fog was never verified by observation. A fix applied to a
> site nobody has run is a fix you have not made.
>
> Measured on `light-and-shadow-debug` at a pinned sunny-16 (auto-exposure OFF), same pose
> throughout: with the sign wrong, `exp(k · heightDiff)` overflows to `+inf` for the whole sky above
> the horizon, `fogAmount` is exactly 1.0, and the sky is REPLACED — sky mean **207.5**, ground
> 114.9, i.e. fog thicker with altitude. After forwarding `constants.tanHalfFovY`: sky **70.1**,
> ground **134.7** — thinner up, thicker down, the palm's fronds legible again and the skybox back.
> Without the sign bug the saturation would have been confined to ~1.3° above the horizon.

> [!CAUTION]
> **The MEDIUM belongs to the scene, not to the effect.** `AtmosphericFog::Parameters` used to hold
> both: a participating medium (density, height falloff, base height, max distance, chromaticity,
> luminance) and the technique's own knobs (inscatter exponent and intensity, sky fog). The medium
> half now lives in `Scenes::ParticipatingMedium`, owned by the `Scene` beside its
> `EnvironmentPhysicalProperties`, and reaches effects through `FrameContext::medium` the way
> `skyLuminance` already does.
>
> ⚠️ It had to move before anything else could share it: an effect's `Parameters` are private to one
> instance and effects in a stack cannot see each other, while `AtmosphericFog` is used by ONE demo
> and `VolumetricLight` by EIGHT. "Share the fog's medium" had no instance to share with in seven
> cases out of eight.
>
> ⚠️ **No medium means NO FOG** — the effect returns the chain colour untouched and warns once. An
> atmospheric fog without an atmosphere is a pass-through, not a fog with invented parameters; that
> is the whole point of having exactly one place that describes the air. Every `Scene` defaults to
> `ParticipatingMedium::Vacuum()`, so a scene that adds the effect must declare a medium.
>
> ⚠️ `ParticipatingMedium::density` is an extinction coefficient in **1/m**.
> `VolumetricLight::Parameters::density` is a **screen-space step multiplier** for a radial blur and
> has no physical unit. Same word, nothing in common — never merge them.
>
> Verified as a pure move on `light-and-shadow-debug`: the four band means agree to **four decimal
> places** across the change (sky 70.1415 → 70.1416, ground 130.3641 → 130.3644), with the deviation
> the same order as between two runs of the SAME binary. The differing-pixel count sits above a
> single-sample noise floor (480 k against 138 k) while its maximum stays below it and no band mean
> shifts — dithering phase, not a value change.

> [!CAUTION]
> **`fogColor` and the inscatter colour are CHROMATICITIES and must be scaled to nits.** The effect
> composites into the ABSOLUTE-LUMINANCE buffer, before tone mapping. Until Aug 2026 the [0,1]
> parameters were pushed raw, so the fog carried ~0.6 **nits**: with `skyFogEnabled` the sky's fog
> amount saturates (the fictive ray length is `maxDistance`) and the sky was not fogged but
> **overwritten with black**. Measured: sky mean 7.05 with fog off → **1.34** with fog on, while the
> ground did not move — the fog only ever touched the sky.
>
> `Parameters::luminance` (nits) now carries the scale; negative, the default, derives it from the
> scene's main directional light as `L = E · ρ / π` — the same Lambertian relation the engine uses
> for a lit surface, with `E` the illuminance in lux and `ρ` the chromaticity. Same separation
> `VolumetricLight` already makes between the light's colour and `mainLight->intensity()` — since 2026-09-25 the
> colour is `mainLight->emissionChromaticity()` (unit luminance), never the removed `color()`.
> `execute()` now also returns `inputColor` untouched when there is no directional light, instead of
> dereferencing a null `mainDirectionalLight()`.
>
> ⚠️ **A [0,1] constant reaching this buffer is always a bug.** It reads black at any real exposure,
> and the mistake is invisible in code review because the numbers look like colours.

See `docs/caution-points.md` for the Y-reconstruction pitfall.

**Code references:**
- `Effects/Atmosphere/AtmosphericFog.hpp` — Parameters, FogPushConstants, API
- `Effects/Atmosphere/AtmosphericFog.cpp` — GLSL shaders, pipeline setup, camera extraction
