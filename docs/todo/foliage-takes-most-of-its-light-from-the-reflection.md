---
id: foliage-takes-most-of-its-light-from-the-reflection
title: Foliage washes out to grey — every NdotV-driven term spikes on the alpha silhouette
status: in-progress
priority: unranked
scope: Graphics/Effects/Lighting (RTR, SSR), Saphir/LightGenerator
opened: 2026-09-15
blocked-by: []
tags: [reflection, measured, owner-reported]
---

# Foliage washes out to grey — every NdotV-driven term spikes on the alpha silhouette

## Why

Owner-reported: *"je le trouve nul à cause des gros pâtés fades induits par la réflexion"*, then,
at point-blank range on a single branch: *"on peut voir en raytracing un blanc dégueulasse qui
entoure toutes les aiguilles du cyprès, et en SSR ça existe aussi, mais moins violent"*.

Sponza's cypress renders grey-white where it must be green. It is NOT a distance or coverage
artefact: at point-blank range the mask is sampled near mip 0, every needle silhouette is crisp,
and the canopy washes out anyway.

## The A/B that settles it — one frame, one variable

Same pose, exposure pinned (f/5.6 · 1/125 · ISO 100), `PostProcess.disable("Reflections")`:

| configuration | foliage luminance | saturation | green excess (G−R) |
|---|---|---|---|
| RayTracing + reflections | 85.36 | **8.45 %** | +0.65 |
| ScreenSpace + reflections | 70.86 | 38.72 % | +7.92 |
| **RayTracing, reflections off** | **21.60** | **52.88 %** | **+4.50** |
| ScreenSpace, reflections off | 21.51 | 51.62 % | +4.45 |

⚠️ With the slot off the two lanes agree to within 2 % — that is the control that makes every other
reading here trustworthy. The stone control moves the other way (RT 15.43 % against SS 11.19 %), so
the desaturation is specific to the foliage, not a property of the frame.

## The chain, read in the code and confirmed by measurement (2026-09-15)

1. **The reflection carries no leaf colour, by construction.** `F0 = mix(vec3(0.04), albedo,
   metalness)`; `LeafSpring` is `metallicFactor = 0`, so F0 is a colourless 0.04 and
   `fresnelTint = fresnelColor / fresnel` is **white**. A metal tints its reflection by its albedo;
   a dielectric does not. The owner's words — *"sans prendre la couleur"* — are literally what the
   code does.
2. **The alpha silhouette is a grazing normal, and it is AUTHORED that way.** Measured on
   `Cypress_LF01_Spring_Normal.png` against the base colour's alpha:

   | zone | mean normal Z |
   |---|---|
   | needle body (opaque) | **+0.676** |
   | under the transparent texels | +0.980 (flat, the unpainted default) |
   | **alpha edge** | **+0.272** — near TANGENT |

   A blend between 0.676 and 0.980 cannot produce 0.272: the rolled edge of the needle is encoded
   faithfully. So `NdotV → 0` on every silhouette, and Schlick's Fresnel → 1 there.
3. **Nothing attenuated it.** `roughnessFade = 1 - smoothstep(0.6, 0.9, roughness)` and the leaf
   declares `roughnessFactor = 0.5` (flat, no metallic-roughness texture — its sibling `IvyLeaf`
   has one), so the fade was exactly **1.0**: full mirror treatment.
4. **And `rtrReflectivity = max(IBLIntensity · (1 − roughness), metalness) = 0.5`** multiplies on
   top — a glossiness used as a reflectance weight.

Net: up to **half of every silhouette pixel replaced by white sky**. With thousands of needles the
silhouettes ARE the visible surface.

## ⚠️⚠️ The defect is SHARED, which is what makes it interesting

Owner's observation, then measured: the residual rim occupies the same pixels in both lanes.
Rim = share of foliage pixels that are bright AND desaturated (`lum > 55`, `sat < 18 %`):

| configuration | rim |
|---|---|
| plain Schlick | 71.15 % |
| roughness-bounded Fresnel (delivered) | 51.26 % |
| + a blunt roughness-fade experiment (NOT delivered) | 2.09 % |
| reflections off — the FLOOR | 0.91 % |

- **74.0 % of the RT rim pixels are also rim pixels in SS.** Same zone, same pixels.
- **42.2 % of the RT rim exists with NO reflection at all** — so a large share of it is the DIRECT
  specular highlight, which no reflection-side fix can reach.

⚠️ **Consequence: specular occlusion is NOT the right target.** It would only address the
reflection share. Three symptoms (RT reflection, SS reflection, direct specular) have ONE cause:
a near-tangent shading normal at the mask edge. The fix must sit where all three read — the normal
and the roughness — not in either lane.

## What is delivered

**Roughness-bounded Fresnel for the ENVIRONMENT term** (Lagarde, *Moving Frostbite to PBR*, 2014;
the form Filament and UE use), in `RTR.cpp` and `SSR.cpp`:

```glsl
F = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
```

A rough surface cannot form a sharp grazing mirror, so F90 is bounded by `1 − roughness` instead of
1. Head-on behaviour is unchanged (F still starts at F0), so a smooth metal is untouched. Measured:
foliage luminance 85.36 → 63.47, saturation 8.45 → 12.20 %, **34 % of the excess luminance removed**,
stone control stable. Real, physically founded, and **not sufficient on its own**.

⚠️ **The direct-lighting Fresnel uses the half-vector `dot(H,V)` and must keep plain Schlick.**
Bounding that one would break the specular highlight of every light in the scene.

## What remains

**Geometric specular antialiasing** (Kaplanyan *et al.*, *Filtering Distributions of Normals*;
Tokuyoshi & Kaplanyan, *Improved Geometric Specular Antialiasing*): where the shading normal varies
sharply from pixel to pixel — the definition of a needle edge — widen the effective roughness by the
normal variance. The lobe spreads, the spike dies, and it reaches the RT reflection, the SS
reflection and the direct specular **at once**, because it acts on the roughness they all read.
Physically founded: a surface whose normal varies within the pixel IS rough at pixel scale.

Apply it ONCE, before the roughness is consumed — the BRDF and the G-buffer write at
`Saphir/Generator/SceneRendering.cpp` (attachment 1 packs `roughness + round(metalness) · 2`), so
both lanes inherit it by reading the G-buffer.

⚠️ Unlike the texture-side work ([`alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain.md`](alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain.md)),
this one DOES reach Sponza: it works on the interpolated normal at shading time, not on texture
pixels, so the KTX2 block-compressed payload does not block it.

## ⚠️ Traps this has already cost

- ⚠️⚠️ **A retracted measurement lived in this file**: "the reflectivity nibble is 0.195 on the
  foliage against 0.205 on the stone, so that lead is CLOSED". Those numbers are exactly `47.98/255`
  and `48.20/255` — the ORDINARY frame. The debug lane was never active (the lane was missing from
  the program-cache key, so the cached program was served). **An instrument must be PROVEN on before
  anything is read from it**, and a lead must never be closed on an unproven one.
- ⚠️⚠️ **A blunt global lever looks like a fix and is not.** Moving `roughnessFade` to
  `smoothstep(0.2, 0.6, roughness)` took the rim from 51 % to 2.09 % and the owner confirmed it by
  eye — but it dims the reflection of EVERY material under 0.6 roughness in both lanes (the stone
  control's saturation moved +27 %) and would wreck the calibrated 0.82 mirror of
  `post-processor-effect-debug`. Reverted. Its value was diagnostic: it proved a ~6× reduction is
  what the foliage needs.
- The first three observations on this frame — "white speckles on the stone", "blown-out leaves",
  "the asset is pale" — were made by LOOKING at a downscaled view and none survived measurement.
- A wrong exposure triad hides the whole thing: at sunny-16 both A/B frames sit near black.
- ⚠️ **The app SAVES its settings on exit**: a debug key set for a session is written back to the
  owner's `settings.json` by `Core.shutdown()`. Restore it AFTER the process is gone, not before.
