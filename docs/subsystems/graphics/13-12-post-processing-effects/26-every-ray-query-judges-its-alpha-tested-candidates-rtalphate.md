## 12. Post-Processing Effects

### Every ray query judges its alpha-tested candidates — `RTAlphaTestGLSL.hpp` (Aug 2026)

> [!CAUTION]
> **A cutout is a TLAS instance flagged `FORCE_NO_OPAQUE`; a ray launched with
> `gl_RayFlagsOpaqueEXT` overrides that flag and accepts every triangle whole.** A leaf becomes a
> solid quad, a fence a wall. RTGI did it on BOTH its rays (bounce + shadow) and RTR on its
> shadow ray: Sponza's ivy blocked the sky for the GI and cast a solid shadow at every bounce hit
> while the raster drew leaves. Only RTR's reflection ray judged its candidates — with a 60-line
> hand-written loop no other effect had copied.
>
> **The rule now lives in ONE place**: `Effects/Shared/RTAlphaTestGLSL.hpp`, two macros
> holding GLSL string literals (a `constexpr const char *` cannot be spliced into the effects'
> `constexpr` shader literals — a macro can):
> - `EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS` — `rtHitMaterialIndex()` and `rtCandidateIsSolid()`,
>   the latter sampling the opacity texture, else the albedo alpha, else the scalar alpha, at the
>   candidate's UV, against the material's own `alphaCutoff` (raster parity);
> - `EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(query)` — the body of a `while (rayQueryProceedEXT)`
>   loop.
>
> Usage: `gl_RayFlagsNoneEXT` (or `TerminateOnFirstHitEXT` alone for a shadow ray — it composes,
> the traversal ends at the first CONFIRMED candidate), then the loop macro. **Never
> `gl_RayFlagsOpaqueEXT` on a scene ray again.**
>
> - ⚠️ The macros rely on names the host shader must declare — `meshSSBO`, `materialSSBO`,
>   `textures2D`, `getMeshAccessor()`, `getHitUV()`, `IsAlphaTest`, `HasOpacityTexture`,
>   `HasAlbedoTexture`. A missing one is a GLSL error at RUNTIME, not at build time.
> - **RTAO and ContactShadows apply it too** (same day). Neither declared the scene data, so the
>   header carries a third macro, `EMEN_RT_SCENE_DATA_GLSL(bindlessSet)`: exactly the SSBO,
>   bindless and UV-fetch declarations the rule needs, for an occlusion-only effect. Both effects
>   gained the bindless set (set 2) in their pipeline layout; ContactShadows dropped its private
>   descriptor set with its own TLAS binding and now uses the Renderer's RT set at 0 like every
>   other RT effect, with a custom three-set recording (the shared fullscreen recorder binds one).
> - ⚠️⚠️ **A candidate-judging ray costs more than an opaque one** — it returns to the shader for
>   EVERY cutout triangle it crosses, with a texture fetch each time — **and the bill scales with
>   the ray count.** Measured on Sponza at the spawn pose (dense ivy in the frame), GPU scopes:
>
>   | scope | opaque flag | alpha-test rule |
>   |---|---|---|
>   | `RTAOEffect/trace`, full-res × 8 spp (owner settings) | 12.3 ms | **26.3 ms** |
>   | `RTAOEffect/trace`, half-res × 8 spp | — | **7.2 ms** |
>   | `ContactShadowsEffect/trace` | 1.45 ms | 2.28 ms |
>   | `RTGIEffect/internal` (half-res × 4 spp, already judging) | 23.5 ms | 24.1 ms |
>
>   RTAO at full resolution DOUBLES under the rule (37 M rays/frame through the foliage); at half
>   resolution the corrected effect costs LESS than the uncorrected full-res one. The resolution is
>   the owner's knob (`RayTracing/AmbientOcclusion/PixelDoubling`), not an engine default to flip
>   behind their back. Visual result of the RTAO/ContactShadows port: 23.5 % of the frame changed
>   by more than 5/255, symmetric (11.7 % brighter — the ivy no longer self-occludes as solid
>   cards; 11.8 % darker), frame mean +0.45.
> - ⚠️ RTR paid the candidate price from the start; RTGI pays it since the same day and did not
>   move (23.5 → 24.1 ms, noise) — it was already judging in both measurements.
