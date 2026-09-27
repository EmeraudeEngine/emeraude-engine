## 12. Post-Processing Effects

### The albedo G-buffer: BASE colour in RGB, DIFFUSE WEIGHT in ALPHA (Aug 2026)

> [!CAUTION]
> **Attachment 3 (`VK_FORMAT_R8G8B8A8_SRGB`) has TWO families of reader, and the two lanes serve
> them apart** (written by `Saphir/Generator/SceneRendering.cpp`, ambient/simple pass only):
>
> | Lane | Content | Readers |
> |------|---------|---------|
> | `.rgb` | the surface **BASE colour** (sRGB-encoded) | **RTR** (`RTR.cpp`, `F0 = mix(0.04, albedo, metalness)`) and **SSR** (`SSR.cpp`, same model) — a metal's reflection carries its own colour |
> | `.a` | the **DIFFUSE WEIGHT** `(1 - metalness) * (1 - transmissionFactor)`, linear (`LightGenerator::diffuseWeightShaderExpression()`) | **SSGI** and **RTGI** combines: `gi *= albedo.rgb * albedo.a` — the energy the diffuse lobe actually receives |
>
> Why the weight exists at all: the GI combines re-modulate a demodulated IRRADIANCE, and the two
> materials that have no diffuse lobe were lit as if they were sheets of paper when the base colour
> alone was applied —
> - a **metal** (gold: baseColor 1.00/0.72/0.32, metalness 1) re-emitted 72 % of the incoming
>   irradiance as diffuse light;
> - a **`KHR_materials_transmission` glass**, whose default base colour is WHITE and which
>   overwrites the G-buffer of everything behind it (translucent materials REPLACED every G-buffer
>   attachment — the "flat water reflections" fix; since Aug 2026 the ALBEDO attachment is
>   opacity-blended instead, see the blended-material bullet below), turned into an opaque milky
>   plate: the watch dial under it was unreadable.
>
> Same convention as NVIDIA NRD (its demodulation albedo is `baseColor * saturate(1 - metalness)`,
> `MathLib::ConvertBaseColorMetalnessToAlbedoRf0`) and as the glTF dielectric BRDF, which MIXES the
> diffuse lobe into the transmission by the transmission factor rather than adding to it.
>
> - ⚠️⚠️ **The one-day regression (2026-08-29 → 30) that dictated the two-lane layout:** the first
>   fix wrote the diffuse albedo INTO the rgb lanes, after a grep for the GI combines' identifier
>   concluded "the only consumers are SSGI and RTGI". The reflections read the same attachment
>   under another name (`albedoTex`): every metal's F0 became 0 and **RTR and SSR stopped
>   reflecting anything on any metal**, in every scene. Found by the first capture of the
>   `post-processor-effect-debug` bench (six white metal panels: flat 95/95/95, no band). **Before
>   changing what an attachment carries, grep for the ATTACHMENT — its binding, `context.albedo`,
>   `requiresAlbedo()`/`needsAlbedo` — never for one consumer's variable name.**
> - ⚠️ A material declaring neither metalness nor transmission writes `a = 1.0`: its GI
>   re-modulation is bit-identical to the pre-Aug 2026 one.
> - ⚠️ The transmission factor is published NOWHERE ELSE in the G-buffer (matprops has no
>   transmission nibble, the normals alpha packs only roughness + a BINARY `round(metalness)`), and
>   the reflectivity nibble is a PARTICIPATION mask `max(metalness, 1 - roughness)` — a smooth
>   dielectric publishes ~1.0 exactly like a metal, so it can never stand in for metalness. The
>   alpha lane is what makes the weight reach the combine at all, with a FRACTIONAL metalness.
> - ⚠️ The RTGI trace applies the SAME rule at BOUNCE HITS (`albedo *= 1 - metalness`, metalness
>   texture included, mirroring RTR): a bounce is a diffuse event, and the multi-bounce feedback is
>   damped by that same albedo product — which is what keeps its geometric series convergent.
> - The unlit path writes `vec4(displayedColour, 1.0)`, the no-material path `vec4(1.0)`; the light
>   passes write nothing (zeroed write mask), the attachment is `LOAD_OP_LOAD` on their pass.
- **A BLENDED material (opacity) contributes in proportion to its coverage** (Aug 2026): the shader
  writes `a = diffuseWeight · opacity` and the albedo attachment is alpha-BLENDED by that lane
  (`rgb = src·a + dst·(1−a)`, `a = a + dst.a·(1−a)`, `SceneRendering::onGraphicsPipelineConfiguration`).
  A transparent texel leaves the surface below untouched; a 35 %-opaque metal decal weighs it to
  0.65; an opaque leaf owns it. **Material properties**: the packed nibbles cannot be interpolated,
  so a blended material writes `a = step(0.5, opacity)` and the attachment blends by `SRC_ALPHA` —
  an exact per-fragment replace-or-keep (the lane itself stays 1). **Normals keep REPLACE**: their
  alpha lane is packed roughness + metalness, and the top-most surface's normal is what the
  reflections want (the flat-water fix; a coplanar decal has the wall's normal anyway). Measured on
  Sponza with the reflectivity nibble displayed as the frame: the dirt decals — metal by glTF
  default — read 0.74 (display) over their whole quads before, 0.60 after (only their ≥ 50 %-opaque
  texels still say metal, which is the asset's word); the share of the frame above 0.7 halved
  (26.9 → 13.2 %). Those quads, rendered by RTR as blurred mirrors over matte stone, were the
  owner's "gros pâtés flous partout". ⚠️ Until then a blended quad
  REPLACED the lanes over its whole extent, transparent texels included: Sponza's `dirt_decal`
  (BLEND, opacity 0.35, glTF-default `metallicFactor` 1 = metal, weight 0) zeroed the RTGI under
  every decal quad — the "gros carrés sombres" — and its ivy (`LeafSpring`, BLEND) painted leaf
  albedo over the background behind its transparent texels, a white haze around the foliage.
  Measured on Sponza: under-decal / elsewhere luminance in shadow 0.17 → 1.47 (the remaining
  excess is the asset's metal decal reflecting the sky IBL, unoccluded), foliage haze gone.
