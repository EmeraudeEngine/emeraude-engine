## Fresnel Effect Generation (Reflection + Refraction)

When a material has BOTH reflection AND refraction components, Saphir generates Fresnel blending code.

### Generation Flow

1. **`StandardResource::generateFragmentShaderCode()`** declares the reflection frame
   (`reflectionNormal`, `reflectionI`, at `Location::Top`, reused between the reflection and
   refraction blocks) and samples the reflection/refraction colours — **high quality only**.
2. **`LightGenerator::generateAmbientFragmentShader()`** detects both components
   (`m_usePBRMode && m_useReflection && m_useRefraction && highQualityEnabled()`) and generates
   the `fresnelFactor` itself, with the Schlick approximation:
   ```glsl
   const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);
   const float fresnelFactor = 0.04 + (1.0 - 0.04) * pow(1.0 - NdotV, 5.0);
   ```
3. The blend `mix(refractedColor, reflectedColor, fresnelFactor)` is added in that **same
   ambient pass** — IBL is the whole contribution of glass. The light passes do NOT re-mix
   reflection and refraction.

### Important Notes

- `fresnelFactor` is **only generated when BOTH** reflection AND refraction are present, in
  high quality; using it anywhere else causes an "undefined variable" shader error
- **F0 is the fixed 0.04 dielectric value**, NOT derived from `ubMaterial.refractionIOR` — the
  material IOR drives the refraction vector (`eta = 1.0 / IOR`), not this Fresnel term
- The `amount` parameters are artistic weights on each leg (neutral `1.0` = fully
  Fresnel-controlled), not a blend against the base colour
- ~~`LightGenerator::generateFinalFragmentOutput()`~~ — DELETED (Aug 2026) along with the whole Blinn-Phong machinery: it had no caller left once the Gouraud and Phong generators were removed.

- ⚠️ (historical) `generateFinalFragmentOutput()` used to carry `!m_usePBRMode`
  reflection/refraction branches that CONSUME a `fresnelFactor` declared by the material. Since
  the material merge, the only material declaring reflection/refraction is `StandardResource`,
  which always calls `enablePBRMode()` — those branches are **unreachable legacy**. Do not
  revive them expecting a material to publish `fresnelFactor`.
- Files: `Graphics/Material/StandardResource.cpp:generateFragmentShaderCode()`,
  `LightGenerator.cpp:generateAmbientFragmentShader()`
