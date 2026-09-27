## IBL Ambient Pass (Jul 2026, IBL lot 3)

`LightGenerator::generateAmbientFragmentShader()` is where ALL image-based lighting lands
(contract: IBL only in the ambient pass, never in light passes). When the program carries
the bindless set (`generator.bindlessTexturesEnabled()` — enabled for every LIT instance
since lot 3, see `RenderableInstance/Abstract.cpp` gating), the pass reads the reserved
slots: cube 1 = baked irradiance (E/π), cube 2 = GGX-prefiltered environment, 2D 3 =
split-sum BRDF LUT.

- **Diffuse irradiance** (every branch): `baseColor × texture(irradiance, N) × envLuminance`
  with the **RAW** world normal `N` (Y-up cubemap convention — the former `(N.x, -N.y, N.z)`
  compensation belonged to the Y-down era and is gone) — the GEOMETRIC world normal
  (`NormalWorldSpace`, synthesized in the ambient-pass vertex branch when bindless is on);
  a 32² cosine-convolved cubemap carries no frequency a normal map could reveal.
  ⚠️ The tint is ALWAYS the raw base color (albedo/diffuse), never the 1/π photometric
  `surfaceColor` (the cubemap already stores E/π) and never the Phong ambient component
  (an artistic constant-ambient hack — a light-grey ka under a 17k lx sky washes
  everything out; it stays on the legacy scalar path).
- **PBR HQ specular** (reflective branch): split-sum reconstruction
  `FssEss = F0·lut.x + lut.y` + **Fdez-Agüera multi-scatter** (`FmsEms`, no extra
  resource) + energy-conserving `kD` on the irradiance.
- **Scalar coexistence**: the legacy `ambientLightColor × intensity` term REMAINS in the
  shader — the scene zeroes the pushed intensity when the sky drives the ambient
  (`Scene::refreshAmbientLightProperties`), so scalar-lit scenes (RTGI demos, manual
  ambient) are untouched. The irradiance SLOT publication itself is gated by the
  `applyAmbient` contract (`Scene::updateEnvironmentIBL`), parking on the default BLACK
  cubemap otherwise — the IBL term then contributes exactly nothing.
- **Baked AO** now modulates ONLY the diffuse ambient terms (`aoFactor` at each addition
  site) — the old global multiply darkened the emissive and the specular IBL.
- The reflection sample itself (`SurfaceReflectionColor`) is roughness-driven since lot 3:
  `textureLod(prefiltered, R, roughness × (mips-1))` in the `StandardResource` bindless
  generator, mip 0 being an exact environment copy for mirrors. The transmission reads the
  prefiltered slot too. (`LightGenerator::roughnessShaderExpression()` still falls back to
  `sqrt(2/(shininess+2))` for a material that only declared a shininess — `BasicResource`,
  which carries no reflection, so that branch never drives a prefiltered fetch.)
