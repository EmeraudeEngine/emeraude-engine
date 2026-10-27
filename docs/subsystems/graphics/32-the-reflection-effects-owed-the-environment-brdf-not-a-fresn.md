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
  WRITTEN TO THE G-BUFFER (`SceneRendering.cpp`), so both lanes inherit it from one place. ⚠️ It filters
  SUB-PIXEL normal variance: it does little at point-blank range where the normal is resolved, and earns
  its keep on distant foliage. ⚠️ This line used to say "and the direct specular": FALSE until
  2026-10-27 — the direct lights read the unfiltered roughness (`docs/caution-points.md` § Geometric
  specular antialiasing). Since then the code is ONE function,
  `LightGenerator::specularAntialiasedRoughness()`, called by the direct lights' BRDF, the ambient pass
  (split-sum lookup and G-buffer write, one variable), the prefiltered environment's LOD and the clear coat
  (not the sheen: owner decision).
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

### What is left on the foliage, and what this hunt cost (from the closed item, 2026-10-11)

- **The residual.** The foliage rim is the share of foliage pixels that are bright and desaturated (lum > 55,
  sat < 18 %). Its floor is 0.9 %: the needle tips the sun really lights. The two lanes now sit at 2.2 % (RT) and 3.0 % (SS).
  - The SS/RT luminance gap that remains (55.95 against 32.17 on the foliage, 69.60 against 34.39 on the stone) is the
    documented screen-space sky-visibility over-estimate in enclosed spaces.
  - The edge albedo is still only 48 % of the true colour for want of alpha dilation: engine item
    `alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain`.
- **The amplifier is authored.** `Cypress_LF01_Spring_Normal.png` encodes the rolled edge of each needle. The mean
  normal Z is +0.676 on the needle body, +0.980 under the transparent texels, and **+0.272 at the alpha edge**, close
  to tangent. So `NdotV -> 0` on every silhouette.
- ⚠️⚠️ **An instrument must be PROVEN on before anything is read from it.** A "reflectivity nibble 0.195 vs 0.205"
  reading closed a lead, but it was the ORDINARY frame (47.98/255): the debug lane was missing from the program-cache
  key, so the cached program was served.
- ⚠️⚠️ **Three physically-founded fixes in a row moved the number and missed the cause.** Lagarde, specular AA and
  traced occlusion took the rim 71 % -> 51 % -> 40 % -> 28 %, while the defect was a MISSING term. The raster IBL
  (`IBLTexture::Role::BRDFLut`) already computed it correctly. **When a value looks wrong, first check whether another
  path in the engine already computes it correctly.**
- **A global lever versus a correct term.** The stone tells them apart: a lever dims the stone too, while the correct
  term leaves it reflecting.
- **Measure, never look.** The first three observations on this frame were made on a downscaled view, and none
  survived measurement: "white speckles on the stone", "blown-out leaves", "the asset is pale".
- **Exposure.** At sunny-16 both frames of the A/B sit near black: pin a usable exposure (f/5.6, 1/125, ISO 100).
- **The application SAVES its settings on exit.** A debug key set for a session is written back by
  `Core.shutdown()`: restore it after the process is gone.
