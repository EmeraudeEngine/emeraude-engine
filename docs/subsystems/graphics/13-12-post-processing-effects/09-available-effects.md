## 12. Post-Processing Effects

### Available Effects

| Effect | File | Passes | Dependencies |
|--------|------|--------|-------------|
| **SSAO** | `Effects/Lighting/SSAO.hpp/cpp` | Multi-pass | Depth, Normals |
| **SSR** | `Effects/Lighting/SSR.hpp/cpp` | 5-pass (Trace→Resolve→BlurH→BlurV→Composite) | Depth, Normals, HDR |
| **VeilingGlare** (ex-Bloom) | `Effects/Camera/VeilingGlare.hpp/cpp` | Multi-pass | HDR |
| **DepthOfField** | `Effects/Camera/DepthOfField.hpp/cpp` | 7-pass (Focus→Setup→DilateH/V→FarGather→NearGather→Composite) | Depth, MaterialProps, **camera-materialized** |
| **ToneMapping** | `Effects/Camera/ToneMapping.hpp/cpp` | Multi-pass (auto-exposure chain) | HDR, **camera-materialized** |
| **VolumetricLight** | `Effects/Atmosphere/VolumetricLight.hpp/cpp` | 2-pass (Occlusion+EMA ping-pong → RadialBlur); IGN-dithered march, jitter-compensated mask, `temporalAlpha` 0.2 (sub-pixel sources rasterize jitter-unstable — caution-points § dash train) | Depth, HDR |

> [!CAUTION]
> **`VolumetricLight` is NOT a volumetric effect, and its settings keys are an OVERRIDE, not a
> default.** It is the screen-space radial-blur god ray (Mitchell, GPU Gems 3): the occlusion mask
> is a depth threshold at 0.9999 — "this pixel is sky" — and the second pass marches in SCREEN space
> toward the sun's projected position. `density` is a screen-space step multiplier, `decay` an ad-hoc
> geometric falloff, and `exposure` an arbitrary gain converting the light's **LUX** into the **nits**
> buffer. **There is no participating medium anywhere in it**: no scattering coefficient, no phase
> function, no height profile, and nothing shared with `AtmosphericFog`'s.
>
> ⚠️ Its keys (`Core/Graphics/PostProcessing/VolumetricLight/{Density,Decay,Exposure,SampleCount,TemporalAlpha}`)
> are read with `settings.get(key, m_parameters.x)` — **`get()`, not `getOrSetDefault()`, and the
> CURRENT parameter as the fallback**. This deliberately breaks the TAA/MotionBlur contract, where a
> setting overrides the constructor and registers itself in the file. Five demos pass deliberately
> tuned values (Citadel 1.2/0.97/0.12/96, Liminal 0.6/0.98/0.12/96, LightAndShadowDebug and
> BasicScenery 0.8/0.98/0.12/64) and an engine-wide default would **silently double their god rays**
> (exposure 0.12 against a 0.25 default); worse, `getOrSetDefault` would let whichever demo runs
> FIRST write its own values into a key the other seven then inherit. An absent key must change
> nothing.
>
> Verified: with no key the frame sits at 580 k differing pixels against a 634 k run-to-run noise
> floor — below it, so nothing moved; with `Exposure = 1.0` the sun-facing mean goes 188.4 → 240.5.
>
> **The keys exist to make this effect comparable at runtime**, because it had none at all while
> eight demos used it, and the world-space single-scattering pass meant to replace it needs an A/B
> that does not require a rebuild. Every one of these knobs becomes meaningless the day the medium
> is real.
| **AtmosphericFog** | `Effects/Atmosphere/AtmosphericFog.hpp/cpp` | 1-pass | Depth, HDR |
| **VolumetricClouds** | `Effects/Atmosphere/VolumetricClouds.hpp/cpp` | 1-pass, FULL-res, two shader variants (with / without cascades) | Depth, HDR, **the scene's `CloudSet`**, bindless (3D shapes + irradiance cubemap), CSM when present — **scene-driven: never added by an application** |
| **RTR** | `Effects/Lighting/RTR.hpp/cpp` | 4-pass (Trace→BlurH→BlurV→Composite) | Depth, Normals, RT (TLAS+SSBOs) |
| **RTGI** | `Effects/Lighting/RTGI.hpp/cpp` | SVGF chain (Trace→Temporal→Moments→NormalHistory→À-trous×N→Apply); all post-trace passes live in the owned `GIDenoiser` | Depth, Normals, MaterialProps, Albedo, Velocity, RT (TLAS+SSBOs) |
| **RTAO** | `Effects/Lighting/RTAO.hpp/cpp` | Multi-pass | Depth, Normals, RT (TLAS+SSBOs) |
| **SSGI** | `Effects/Lighting/SSGI.hpp/cpp` | SkyVisibility (GTAO horizon search) → SVGF chain (Trace→GIDenoiser, same shape as RTGI) | Depth, Normals, MaterialProps, Albedo, Velocity, HDR, **bindless irradiance cubemap** |
| **RTContactShadows** | `Effects/Lighting/RTContactShadows.hpp/cpp` | Multi-pass, HALF-res | Depth, Normals (READ since Sep 2026 — the ray origin's normal offset), MaterialProps, LightSet, RT (TLAS+SSBOs) |
| **SSContactShadows** | `Effects/Lighting/SSContactShadows.hpp/cpp` | Multi-pass, FULL-res | Depth, Normals, MaterialProps (combine only), LightSet |
| **LensFlare** | `Effects/Camera/LensFlare.hpp/cpp` | 2 passes, half res (source + ghosts) | Depth (the sun probe), HDR; the main directional light is OPTIONAL (it is the injected sun) |
| **FogEnvironment** | `Effects/Atmosphere/FogEnvironment.hpp/cpp` | 1-pass | Depth |

The quality keys an effect reads with `getOrSetDefault()` are BOUNDED (triad 7f, owner ruling 2026-10-01): outside
its range a value warns and takes the default (`Settings::getOrSetDefaultInRange()`, the ranges beside the
defaults in `SettingKeys.hpp`): `TemporalAA/Alpha` (0, 1], `TemporalAA/VarianceGamma` (0, 10],
`MotionBlur/SampleCount` [1, 128], `MotionBlur/SoftDepthExtent` (0, 10], `Clouds/StepCount` [1, 512],
`Clouds/LightStepCount` [1, 64], `DepthOfField/SampleCount` [1, 256], `DepthOfField/MaxRadius` [1, 128] px,
`DepthOfField/AutoFocusSpeed` (0, 100]. `VolumetricLight`'s override keys above are not bounded yet (triad 7g).
