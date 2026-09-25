---
id: foliage-diffuse-transmission
title: Leaf translucency — a thin diffuse transmission lobe (KHR_materials_diffuse_transmission) in both lanes
status: blocked
priority: unranked
blocked-by: [tlas-one-instance-per-multiple-visuals, rt-thin-two-sided-hit-normal, rtcontactshadows-backlit-self-hit, subsurface-component-defects]
scope: Graphics/Material (StandardResource), Saphir (LightGenerator light + ambient passes), RT hit shaders (RTGI, RTR, IrradianceProbeVolume), ImposterAtlas/ImposterBake, glTF loader, Scenes::Toolkit vegetation material
opened: 2026-09-25
tags: [vegetation, lighting, material, gltf, ray-tracing]
---

# Leaf translucency — a thin diffuse transmission lobe (KHR_materials_diffuse_transmission) in both lanes

## Why

The owner asked for it on 2026-09-25: a leaf lit from behind must glow (yellow-green), in `forest` and `terrain`,
in both lighting lanes. Today a back-lit leaf receives nothing from the sun. The shading normal is flipped toward
the viewer (`LightGenerator.PBR.cpp` ~577/587), so the sun behind the card gives `N·L ≤ 0`. The only back-lit term
in the engine is the **Subsurface** component (`StandardResource.hpp` ~926-944, `LightGenerator.PBR.cpp` ~938-962),
a skin model with defects (listed below). The Toolkit foliage material does not declare it.

Measured leaf optics (Majasalmi & Bright 2019, band averages): R ≈ 0.08-0.11, T ≈ 0.04-0.06 in the visible band,
so T/R ≈ 0.5-0.75. Leaf transmission is practically Lambertian (Habel et al. 2007; Baranoski & Rokne 2001). The
dominant visible effect is the sun through the FIRST leaf; the leak through several leaves is secondary.

## State of the art surveyed (2026-09-25, a design workflow with two adversarial verifiers)

- Tiago Sousa, "Vegetation Procedural Animation and Shading in Crysis", GPU Gems 3 ch. 16 (2007): a wrapped back
  diffuse `saturate(-N·L·0.6 + 0.4)` plus a view-dependent pow4 lobe, times an averaged shadow. Artistic, not
  energy conserving.
- Barré-Brisebois & Bouchard, "Approximating Translucency for a Fast, Cheap and Convincing Subsurface-Scattering
  Look", GDC 2011 (Frostbite 2), and the 2012 spherical-Gaussian revision: a distorted back light
  `pow(saturate(V·-(L + N·δ)), p)·scale` times a baked thinness map. ~13 ALU.
- Habel, Kusternig & Wimmer, "Physically Based Real-Time Translucency for Leaves", EGSR 2007: a thin-slab
  multi-dipole BSSRDF projected into the Half-Life 2 basis. Its baseline is exactly the Lambertian BTDF
  `L_t = -L_D·ρt·(n_g·ω_D)` on the GEOMETRIC normal (eq. 24).
- Unreal Engine "Two Sided Foliage" (TwoSidedBxDF; transmission lit through a separate subsurface shadow), Unity
  HDRP thin-object translucency (diffusion profile + thickness map), Filament subsurface (not physically based by
  its own header), Godot backlight (a constant 1/π over the back hemisphere).
- **KHR_materials_diffuse_transmission** (Release Candidate): `diffuse = mix(brdf, btdf, t)`, a Lambertian BTDF
  tinted by `diffuseTransmissionColorFactor`; the light is mirrored through the tangent plane for the Fresnel
  term. Khronos sample renderer: `pbr.frag` ~331-350 for the punctual lobe, ~198-206 for the IBL
  `mix(E(n)·ρ, E(-n)·T, t)` (NOT `punctual.glsl`, which the first draft cited). pbrt-v4 `DiffuseTransmissionBxDF`
  and Cycles "Translucent BSDF" are the offline references. `fastgltf` already parses the extension, and the
  Khronos `DiffuseTransmissionPlant` / `DiffuseTransmissionTeacup` are in the glTF-Sample-Assets corpus.

## Options (the owner decides)

- **A. A thin mode on the existing Subsurface component, direct light only.** Smallest change, no G-buffer or RT
  change. But one component would carry two unrelated physical models, the lanes diverge (SSR/SSGI propagate the
  glow, RTR/RTGI/probes do not), and there is no glTF path.
- **B. A new `DiffuseTransmission` material component, following KHR (RECOMMENDED by the design).**
  - Same side of the light: `(1-t)(1-F)(1-m)·ρ/π·E·(n·l)`, plus the specular.
  - Far side: `t·(1-F')(1-m)·(1-transmissionFactor)·T_col/π·E·(-n·l)`, with `F'` taken with the light mirrored
    through the tangent plane. The `(1 - transmissionFactor)` factor is prescribed by the spec: specular
    transmission overrides diffuse transmission. The first draft omitted it.
  - It is evaluated identically in the raster light pass and in the three RT direct-lighting copies (RTGI, RTR,
    the probe volume). `GPURTMaterialData` grows to 8 vec4, with a `ThinTwoSided` flag.
  - `t` is published in the free matprops R low nibble. ⚠️ That nibble was also wanted for the vegetation density
    floor of 2026-09-22: arbitrate.
  - Imposters: the bake keeps its matprops attachment, and `t` goes into the free atlas `normal.w` before the mips
    are built. An imposter has no shadow, so a back-lit crown glows evenly.
  - Imported from glTF.
- **C. B plus the back-hemisphere INDIRECT** (sky and bounce from behind the leaf). Traced lane: a probe query at
  `-N` (⚠️ pass `-V` as the view direction or the two 0.1 m biases cancel; the query returns 0 outside the
  ~22.5 × 10.5 × 22.5 m camera-centred volume, and most of a canopy is outside it). Screen-space lane: the back
  visibility CANNOT be measured in screen space, so it needs an owner-chosen proxy (front GTAO, the baked crowding
  density, or none). The unoccluded proxy would repeat the Sponza 4.6× over-lighting class.
- **D. A canopy leak** (Beer attenuation through the crown instead of a binary shadow): raw CSM depth in raster,
  coloured shadow rays in RT. A secondary effect. Build it only after B is measured. It needs a PER-CHANNEL τ: a
  scalar loses the chain filtering that gives the saturation.

Design recommendation: **B first; then C's traced half once B is measured; C's screen-space half only with an
owner-chosen proxy; D deferred.**

## Owner decisions (2026-09-25)

- **Model: KHR_materials_diffuse_transmission (Lambertian thin BTDF, energy split with the reflection) PLUS an
  optional artistic forward-scatter lobe, defaulting to 0.** The lobe must stay energy-preserving: the first
  draft's `mix(1, exp2(n·(V·-L) - n), k)` is ≤ 1 everywhere and only removes energy.
- **Home: a new `DiffuseTransmission` material component (option B)** — not a Subsurface thin mode.
- **Scope: B first, then measure.** C (back-hemisphere indirect) and D (canopy leak) wait for B's measurement.
- **Prerequisites first**: one item each (`blocked-by` above), the TLAS suspicion verified by measurement, all
  fixed before B.

## What remains

1. Owner decisions still open (the four above are settled):
   - Lambertian KHR BTDF vs an artistic forward lobe (or KHR with an optional forward-scatter parameter at 0).
   - Home: a new component (B) vs a Subsurface thin mode (A).
   - Scope: B / B+C / later D.
   - The C screen-space proxy.
   - The transmission colour in the lanes' indirect: the albedo hue in 4 bits vs a new RGBA8 G-buffer attachment.
   - Toolkit leaf values:
     - base colour compensated by `1/(1-t)` or KHR darkening;
     - `t`/`T_col` per species in JSON vs `-translucency` images;
     - ⚠️ a STORE material of the same name wins over `Toolkit::vegetationMaterial()`, and a `<name>-normal`
       image is taken when it exists.
   - Geometric vs shading normal for the far lobe.
   - The imposter policy: glow evenly, density attenuation, or fade toward 250 m.
   - The RTContactShadows fix.
   - Approval of the RT normal-orientation change, which CHANGES today's traced foliage.
2. Prerequisites — each its own item when approved:
   - (a) VERIFY the reading that the TLAS holds ONE instance per `MultipleVisuals` component (a grove traced as
     one tree). Unmeasured.
   - (b) RT hit-normal orientation for two-sided thin materials, and the probe back-face rule
     (`IrradianceProbeVolume` skips back-face rays with `continue`). Orient the GEOMETRIC normal before RTR
     perturbs it with the normal map.
   - (c) The RTContactShadows back-lit self-hit. ⚠️ A light-side ray origin does NOT fix it under wind: the BLAS
     holds rest-pose triangles (owner decision 2026-09-21), so the ray hits the rest-pose copy of its own card.
     Only a facing gate like SSContactShadows' (~148-152), or rejecting the pixel's own instance, survives.
   - (d) The Subsurface defects, whatever the choice:
     - no 1/π;
     - the wrap reaches `N·L = -w`;
     - specular without `N·L`;
     - an undefined smoothstep at intensity 0 or 1;
     - a NaN at radius 0 with a thickness map;
     - a unitless ambient constant that is NOT multiplied by `IBLDiffuseWeight` (active under both lanes);
     - with a TEXTURE the UBO intensity is dead (the texel .r alone);
     - stale docs: `Saphir/AGENTS.md` ~482 (a 0.99 clamp that does not exist, and stale line numbers), and
       `Graphics/AGENTS.md` ~5029-5031 ("needs a thickness": false, a no-thickness branch exists).
     Porcelain and the Liminal gems change look when it is repaired.
3. Tooling first:
   - a console command to freeze the vegetation wind AT ZERO DISPLACEMENT (flutter included; none exists);
   - a runtime A/B weight for the term that also reaches the RT hit shaders and the imposters;
   - an HDR readback (not 8-bit ACES PNGs with glare, DoF and TAA jitter);
   - a per-pixel shadow-visibility debug view.
4. Measurement:
   - A single-card formula bench with ONE directional light. NOT `+ModelViewer`: its 15 klx fill light and flat
     ambient contaminate the ratio by ~20 %.
   - Isolate the term as a DIFFERENCE (light on minus off, or term weight 1 minus 0), not as a raw back/front ratio.
   - Validate against pbrt-v4 or Cycles.
   - Then `forest`/`terrain` in both lanes: pinned exposure, frozen sun and wind, and `terrain` option 7 = 0 (its
     tree shadows are drawn two LODs coarser, so a leaf's visibility is decided by proxy cards).

## ⚠️ Traps

- The CSM normal offset uses the UNFLIPPED normal (`LightGenerator.ShadowMap.cpp` ~771-786). It is dormant only
  because `NormalOffsetScale` defaults to 0. Face it toward the light for cull-none materials before anyone
  raises it, or a leaf looks up its shadow behind itself.
- Raster leaves use a HASHED alpha test; every ray query judges candidates against the fixed material cutoff
  (`RTAlphaTestGLSL.hpp` ~232). The raster and RT leaf coverage differ.
- The wind displaces positions, never normals: the lobe choice follows the rest orientation.
- JungleRuins (USD, the gold goal) ships per-species `*_translucency*` maps: a second importer to map.
- A closed or volume KHR material (Teacup, the spec's candle) is NOT a thin sheet: thin-shared visibility is wrong
  there, and the object's own shadow kills the far-side glow.
- Unreal lights its transmission through a separate subsurface shadow (a D-like leak): it is NOT an example of
  binary thin-shared visibility.

## References

- The design workflow of 2026-09-25 (research, three code audits, a design, two adversarial verifiers). Full
  result archived in the session only: the options above are its verified summary.
- KHR_materials_diffuse_transmission: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_diffuse_transmission
- Sousa, GPU Gems 3 ch. 16: https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-16-vegetation-procedural-animation-and-shading-crysis
- Barré-Brisebois & Bouchard, GDC 2011: https://colinbarrebrisebois.com/2011/04/04/approximating-translucency-part-ii-addendum-to-gdc-2011-talk-gpu-pro-2-article/
- Habel, Kusternig & Wimmer, EGSR 2007: https://www.cg.tuwien.ac.at/research/publications/2007/Habel_2007_RTT/
- UE Two Sided Foliage: https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine
