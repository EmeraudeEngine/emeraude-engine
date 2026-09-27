## The reflection effects owed the ENVIRONMENT BRDF, not a Fresnel (Sep 2026)

`RTR.cpp` and `SSR.cpp` weight their composite with `environmentBRDF(F0, roughness, NdotV)` —
Karis' analytic fit of the split-sum's second half (*Real Shading in Unreal Engine 4*, 2013), the
form Unreal and Filament ship. It returns `F0 * A + B`, the integral of the specular BRDF over the
lobe, which is what must multiply a PREFILTERED environment.

Both effects used a raw Schlick Fresnel instead, compensated by a roughness fade and a reflectivity
nibble. Schlick rises to **1.0** at grazing incidence whatever the roughness; the real integral does
not, because a rough dielectric cannot return its whole lobe. Measured on Sponza's cypress
(F0 = 0.04, roughness 0.5) at grazing: **0.500 against 0.155**, a 3.2x overestimate applied exactly
on the alpha silhouettes — where the authored normal is near-tangent (mean Z **+0.272** against
+0.676 on the needle body), i.e. on most of a canopy's visible surface.

⚠️⚠️ **The raster IBL path was already correct** — it goes through the engine's BRDF LUT
(`IBLTexture::Role::BRDFLut`). Only the reflection effects were not, so the raster and the
reflections disagreed on the same surface. Closed analytically rather than by binding the LUT into
two more effects.

Measured on one frame, one variable, pinned exposure — share of foliage pixels that are bright AND
desaturated (the white rim), against a "reflections off" floor of 0.9 %:

| | rim | foliage saturation |
|---|---|---|
| raw Schlick (RT) | 71.15 % | 8.45 % |
| environment BRDF (RT) | **2.17 %** | **30.92 %** |
| raw Schlick (SS) | 16.42 % | 38.72 % |
| environment BRDF (SS) | **2.98 %** | **47.15 %** |

⚠️ **This is not "reflections turned off"**, and that distinction is the whole point: the stone keeps
its reflection (34.39 against 28.15 with the slot off) while the foliage stops being flooded. A
smooth metal is untouched by construction — F0 = 1, roughness 0 returns 1.000.

Three smaller terms landed with it and are cumulative, but this one carried ~90 % of the result:

- **Roughness-bounded Fresnel** (Lagarde 2014) — superseded as the composite weight, kept nowhere.
- **Geometric specular antialiasing** (Kaplanyan; Tokuyoshi & Kaplanyan) folded into the roughness
  WRITTEN TO THE G-BUFFER (`SceneRendering.cpp`), so both lanes and the direct specular inherit it
  from one place. ⚠️ It filters SUB-PIXEL normal variance: it does little at point-blank range where
  the normal is resolved, and earns its keep on distant foliage.
- **Traced specular occlusion** in RTR: the mirror ray is a ONE-SAMPLE estimate of the lobe, so four
  visibility-only rays spread over `coneTan` weight the sky-miss branch. ⚠️ MISS branch only — a hit
  is already occluded and shaded as such. ⚠️ A smooth surface is untouched (`coneTan -> 0`).
  ⚠️ RT lane only: SSR cannot trace.

⚠️ **Never "fix" this with the roughness fade window.** Moving `roughnessFade` to
`smoothstep(0.2, 0.6, roughness)` reaches the same rim number and looks like a cure — it dims the
reflection of EVERY material under 0.6 roughness in both lanes and would wreck the calibrated 0.82
mirror of `post-processor-effect-debug`. The difference is that the environment BRDF re-weights,
where the fade merely dims.

⚠️ The direct-lighting Fresnel uses the half-vector `dot(H,V)` and keeps plain Schlick — bounding
that one would break the specular highlight of every light in the scene.
