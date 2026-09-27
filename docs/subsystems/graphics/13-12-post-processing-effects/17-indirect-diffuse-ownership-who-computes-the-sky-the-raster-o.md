## 12. Post-Processing Effects

### Indirect-diffuse OWNERSHIP — who computes the sky, the raster or the effect (Aug 2026)

> [!CAUTION]
> **A sky-lit scene has TWO subsystems able to compute the same diffuse irradiance, and adding
> both counts the sky twice.** The raster ambient pass adds
> `albedo * (1 - metal) * (1 - F) * iblIrradiance * environmentLuminance` (the baked irradiance
> cubemap, reserved bindless cube slot 1). RTGI's miss branch adds
> `cubemap(dir) * FrameContext::skyLuminance` for every ray that escapes the TLAS — the SAME
> integral, from the SAME cubemap, with the SAME luminance (`background->luminance()`), only with
> real visibility. **Measured** on `asset-loader --demo-options 11,0,1,0,0,0` under
> AutumnFieldPureSky (31 800 nits): the watch read 159/255 with both, 116/255 with SSGI (which has
> no sky term), 109/255 once the ownership contract landed — the 43-point gap WAS the double count.
>
> **The contract.** `PostProcessEffect::providesIndirectDiffuse()` (default `false`) declares an
> effect as the OWNER of the frame's indirect diffuse. `PostProcessStack::hasEnabledIndirectDiffuseProvider()`
> aggregates it, `Scene::updateIBLDiffuseOwnership()` polls that every logic tick and pushes a
> `iblDiffuseWeight` of **0** (an owner is active) or **1** (the raster owns it) into the view UBO;
> the ambient pass declares `iblDiffuseIrradiance = iblIrradiance * iblDiffuseWeight` and every
> DIFFUSE leg reads that one. Same shape as the reflection cost ladder
> (`hasEnabledReflectionProvider()` → the Renderer suspends the continuous probes).
>
> - ⚠️ **The weight scales the DIFFUSE leg ONLY.** The specular IBL — prefiltered reflections and
>   the Fdez-Agüera multi-scatter compensation `iblFmsEms` — keeps the RAW `iblIrradiance`: no
>   post-process replaces it, and zeroing it would darken rough metals. That is why the split-sum
>   branch emits TWO additions where it used to emit one.
> - ⚠️ **The scene's SCALAR ambient is untouched.** A hand-lit scene's `setAmbientLightIntensity()`
>   (Sponza's 200 lx) is the owner's deliberate residual — the "skylight leaking" knob of the
>   reference implementations — not a computed term. Only the sky-derived irradiance changes hands.
> - ⚠️⚠️ **SSGI IS an owner since Sep 2026, and this bullet said the exact opposite.** It read
>   *"a screen-space effect can NEVER be an owner […] SSGI has no sky term at all"*. The premise was
>   right and the conclusion was wrong: a ray MISS in screen space indeed means "no occluder found
>   in the depth buffer", never "open sky" (implemented from the miss branch, measured and reverted
>   in Jul 2026: mean 96.2 / median 96.0, zero spatial variation — the flat ambient it was meant to
>   replace). The answer is not to give up the sky, it is to MEASURE its visibility instead of
>   inferring it from a miss: SSGI now runs a GTAO horizon search and composites
>   `irradianceCube(bentNormal) * skyLuminance * V`. See § "The screen-space sky visibility" for the
>   pass, its acceptance measurements and what it still cannot see.
>   ⚠️ Leaving the raster leg on was NOT the safe half of the choice: an enclosed space came out
>   **4.6× brighter than the traced lane** on Sponza, the galleries lit as if the courtyard had no
>   roof. Two disjoint terms only add up when BOTH are right.
> - ⚠️⚠️ **The provider must be gated on its ability to RUN.** `RTGI::providesIndirectDiffuse()`
>   repeats the exact gate `PostProcessor` skips the effect on (hardware, `isRayTracingSettingEnabled()`,
>   `isRayTracingReady()`). Claiming ownership while the TLAS is still building would hand the diffuse
>   to an effect that draws nothing — a black flash over the first frames of every scene.
> - ⚠️ **Known limitation until the frame is reordered**: the indirect effects run AFTER the
>   TranslucentGB pass, so a surface seen THROUGH a transmissive material gets no indirect at all
>   (the raster leg is off, and the G-buffer at that pixel belongs to the glass). Item
>   `docs/todo/indirect-diffuse-before-translucency.md`.
>
> **State of the art**: UE5 Lumen — *"Sky lighting is solved as part of Lumen's Final Gather
> process. It includes sky shadowing"*; Unity HDRP — *"SSGI and RTGI replace all lightmap and Light
> Probe data […] Light Probes and the ambient probe stop contributing"* (and its `LightLoop.hlsl`
> applies that replacement to non-transparent surfaces only). Both REPLACE, neither adds.
>
> **Files**: `Graphics/PostProcessEffect.hpp` (the virtual), `Graphics/PostProcessStack.{hpp,cpp}`,
> `Graphics/Effects/Lighting/RTGI.{hpp,cpp}` and `Graphics/Effects/Lighting/SSGI.{hpp,cpp}` (the two
> providers), `Scenes/Scene.lighting.cpp`
> (`updateIBLDiffuseOwnership`, `refreshAmbientLightProperties`), `Scenes/Scene.cpp` (the poll),
> `Saphir/LightGenerator.cpp` (`iblDiffuseIrradiance`), `Saphir/Generator/Abstract.cpp` +
> `Graphics/ViewMatrices{2D,3D,Cascaded}UBO.*` (the UBO lane — it fits in the EXISTING padding
> after `environmentLuminance`, so the UBO size did not move).
