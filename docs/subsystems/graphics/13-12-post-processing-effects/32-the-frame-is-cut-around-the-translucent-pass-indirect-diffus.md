## 12. Post-Processing Effects

### The frame is CUT around the translucent pass — indirect diffuse before the glass (Aug 2026)

> [!CAUTION]
> **What is seen THROUGH a transmissive surface must receive its indirect diffuse, and the only
> way is to composite that term BEFORE the material grab pass copies the scene.** The indirect
> chain used to run after the TranslucentGB pass, so a glass transmitted a scene with no indirect
> light in it; once the indirect-diffuse ownership contract switched the raster's own IBL leg off
> under RTGI, the watch dial behind its crystal received no indirect light AT ALL (measured:
> −6.11 on the dial vs −4.64 on the opaque case, RTGI minus no-GI, before the cut).
>
> **The mechanism.** `EffectSlot::isPreTranslucencySlot()` names the slots that run early — ONLY
> `IndirectDiffuse`. `PostProcessStack::hasEnabledPreTranslucencyEffect()` says whether a frame
> has anything to cut. `Renderer::renderFrameWithInternal()` cuts the frame when that is true AND
> the frame contains TranslucentGB objects:
> ```
> opaque + translucent scene pass
>   → PostProcessor::recordBlit  →  executeIndirectPostProcessEffects(PreTranslucency)
>       (the IndirectDiffuse slot, on the OPAQUE G-buffer)  →  recordWriteBack() into the scene colour
>   → material grab pass (now carries the indirect diffuse)  →  TranslucentGB pass
>   → PostProcessor::recordBlit  →  executeIndirectPostProcessEffects(PostTranslucency)  →  composite
> ```
> An uncut frame (no glass, or no enabled indirect-diffuse effect) runs `ChainPhase::Whole`
> exactly as before and pays nothing new.
>
> - ⚠️ **Only the indirect diffuse moves forward.** The ambient occlusion stays in the closing
>   half: its snippet is a global multiply and it must come AFTER the reflections (placed before
>   them it stopped attenuating them — the owner-reported bright patches). It still attenuates the
>   transmitted indirect diffuse, which is already in the colour by then; the price is that AO is
>   evaluated at the glass surface rather than at the surface behind it. The reflections stay after
>   the translucent pass so a water surface keeps its SSR/RTR through its own G-buffer footprint.
> - ⚠️ **The write-back is a blit** (`PostProcessor::recordWriteBack`, same two-barrier discipline
>   as `recordBlit`): chain target SHADER_READ_ONLY → TRANSFER_SRC, scene colour
>   COLOR_ATTACHMENT → TRANSFER_DST, blit, restore both. The scene colour MUST come back to
>   COLOR_ATTACHMENT_OPTIMAL — the material grab pass and the TranslucentGB LOAD pass expect it.
>   Rendering the combine straight into the scene colour would save the blit; not done, measure
>   first.
> - ⚠️ **The closing half owns the frame.** Only `ChainPhase::Whole`/`PostTranslucency` update
>   `m_lastChainFrameTime` (the motion blur's frame duration — the two halves run milliseconds
>   apart) and the final composite descriptor. The pre-translucency half returns right after its
>   write-back.
> - ⚠️ **The RT descriptor set is refreshed BEFORE the pre-translucency half** (it used to be
>   refreshed after the translucent pass): RTGI traces there.
> - ⚠️ **TranslucentGB programs keep their raster IBL diffuse.** The indirect diffuse can no longer
>   reach a surface drawn by that pass, so its own diffuse leg (a frosted glass, a tinted water)
>   is exempt from the ownership weight — decided at shader GENERATION through the grab-pass
>   marker (`LightGenerator::m_transmissionIsSceneRadiance`), zero runtime cost.
> - ⚠️ The swap-chain (direct) path is never cut: an indirect-diffuse effect needs the G-buffer,
>   which forces the internal scene target. `recordWriteBack` refuses to run without it.
> - ⚠️⚠️ **Two usage flags had to be added for the write-back, and a blit without them RUNS anyway
>   on NVIDIA** — with a plausible image and four VUIDs a frame (`oldLayout-01212/01213/01197`,
>   `vkCmdBlitImage-srcImage-00219`/`dstImage-00224`): the CombinePass targets now carry
>   `TRANSFER_SRC` (`CombinePass.cpp`), the scene colour image `TRANSFER_DST`
>   (`SceneRenderTarget.cpp`, colour only). ⚠️ `RenderTarget/Texture.hpp` is NOT the scene target
>   — it is the render-to-texture probe; the flag was first added there by mistake and reverted.
>   The undefined-behaviour blit also poisoned the first measurement (a −2.5 shift on the SKY,
>   which the cut cannot touch): **never read a number off a run that reports VUIDs.**
> - **Measured** (asset-loader ChronographWatch, 2880×1620, RTGI on in both runs, `Core/Graphics/
>   PostProcessing/CutFrameAroundTranslucency` true vs false, run-to-run noise from two identical
>   uncut runs):
>
>   | region | cut − uncut | noise floor |
>   |---|---|---|
>   | dial, behind the crystal | **+3.47** | +0.48 |
>   | case + strap (opaque) | −0.71 | +0.45 |
>   | whole watch | +0.27 | +0.51 |
>   | sky | −0.97 | +0.58 |
>
>   The dial gains its indirect diffuse (7× the noise); everything the cut must not touch stays
>   inside the noise. Cost: frame 5.65 → 6.28 ms (**+0.64 ms**: the pre-translucency half at
>   1.33 ms including its own grab blit, minus the GI that left the closing half, 4.55 → 3.81 ms).
>   0 VUID. The key is an A/B switch for exactly this kind of measurement, not a quality knob:
>   OFF, nothing seen through a glass receives indirect light.
