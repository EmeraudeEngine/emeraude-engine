## 12. Post-Processing Effects

### The chain order is a STRUCTURE, not a call sequence — `EffectSlot` (Aug 2026)

> [!CAUTION]
> **`PostProcessStack` is no longer an insertion-ordered vector.** It is a fixed table of
> CONCEPTS — `Graphics/EffectSlot.hpp` — walked in enum declaration order, and every framebuffer
> effect declares the concept it implements (`IndirectPostProcessEffect::slot()`, **pure
> virtual**). `addEffect()` files the effect into its slot; **the order of the calls has no
> effect whatsoever**. Twelve scenes used to restate the order by hand, three of them
> differently, and a wrong order was silent.
>
> **The canonical order (this IS the enum):**
> ```
> ContactShadows → IndirectDiffuse → Reflections → AmbientOcclusion
>   → Clouds → VolumetricLight → Fog → Custom → TemporalAA
>   → [camera: DepthOfField → MotionBlur] → LensFlare → [camera: Glare → ToneMapping]
>   → PostToneMapping
> ```
>
> ⚠️ **Three slots moved in Sep 2026, and none of the three costs anything** (measured on
> `sponza`, same pose, GPU profiler: frame 63.75/64.29 ms before, 63.94 ms after — inside the
> run-to-run envelope). They are ordering-LOGIC fixes, not optimizations:
> - **`ContactShadows` went FIRST**, ahead of the three indirect terms. Its snippet is a multiply
>   and a contact shadow occludes the DIRECT light (its ray marches the depth buffer toward the
>   light), so it must land while `em_Color` still holds the raster output alone. Last of the
>   group, it also darkened the indirect diffuse and the reflections, which no light transport
>   justifies — a surface in contact shadow still receives bounced light. Measured effect on the
>   bench: frame mean luminance 85.95 → 87.71. It also had to join `isPreTranslucencySlot()`:
>   the two halves of a cut frame are two separate walks of the table, so the enum position
>   alone would have left it running after the indirect diffuse on every scene with glass.
> - **`VolumetricLight` went BEFORE `Fog`.** The fog is a `mix()` toward the medium colour, i.e.
>   an extinction; the shafts are a pure add. Added after the fog they escaped the very medium
>   that scatters them. ⚠️ This buys them the transmittance of the SCENE DEPTH, which is an upper
>   bound on the true one (a shaft scattered close to the camera is now over-attenuated). Feeding
>   the transmittance into the shaft march is still the correct fix and stays **OPEN**.
> - **`LensFlare` LEFT the scene phase** for between `MotionBlur` and `Glare`. A flare is formed
>   in the optics and locked to the SCREEN, not to the world: sitting before `TemporalAA` it was
>   reprojected along the scene's motion vectors and smeared whenever the camera moved. It now
>   also feeds the glare. ⚠️ This is what forced `isCameraEffectSlot()` to become an explicit
>   SET: as a RANGE (`>= DepthOfField && <= ToneMapping`) it captured the newcomer, and
>   `addEffect()` refuses a camera-owned slot — every scene building a LensFlare would have lost
>   it, with a trace error and no flare.
>
> ⚠️⚠️ **`Glare` STAYS after `MotionBlur`, and the physical argument for moving it is a trap.**
> The sensor order would be optics → glare → shutter integration → response, so the glare
> "should" precede the motion blur. It must not, because the glare is **paired** with the tone
> mapping (`syncCameraEffects()`: `setBloomSource()` + `setCompositeBypassed(true)`): at its own
> slot the VeilingGlare only BUILDS its pyramid and returns its input unchanged (`VeilingGlare.cpp:735-737`),
> and the ToneMapping APPLIES it at the very end. Moved earlier, the pyramid would be built from
> the un-blurred image and still applied after the blur — a sharp halo over a smeared source.
> Reordering the slot would require un-bypassing the composite first, which costs a full-res
> pass. **The pairing owns this order, not the enum.**
>
> ⚠️⚠️ **This order CHANGED with the redesign, and the change is a correctness fix.** It used to
> be `Reflections → AmbientOcclusion → IndirectDiffuse`, which broke one thing:
> - **SSR reflected an unlit world.** `SSR` samples the chain colour to fetch what a reflected
>   ray sees (`reflColor = texture(colorTex, traceData.xy).rgb`) and declares
>   `readsChainColorUpstream()` PRECISELY so the pending combine group is flushed before it — but
>   placed first, it had nothing to flush. Since the indirect-diffuse OWNERSHIP contract, an
>   enabled RTGI also switches the raster's ambient IBL leg off, so those reflections carried no
>   sky light at all. `RTR` is immune: it shades its own hits and never reads the chain.
>
> ⚠️⚠️ **THE AMBIENT OCCLUSION IS LAST OF THE THREE, and both neighbours are load-bearing.** The
> members of a combine group emit their snippets into ONE generated pass in slot order, and AO's
> is a GLOBAL MULTIPLY (`em_Color.rgb *= ao;`) while GI's and the reflections' are adds:
> **everything emitted BEFORE the AO is attenuated, everything after is not.**
> - Placed FIRST (the historical order) it multiplied the direct lighting and left the indirect
>   diffuse it exists to occlude untouched.
> - Placed between the GI and the reflections — which is where the redesign put it for a few
>   hours — it stopped attenuating the reflections, and traced reflections came out at FULL
>   strength inside creases and occluded corners. **Owner-reported the same day**: "la réflexion
>   éjecte des gros pâtés lumineux partout". Measured on Sponza at the spawn pose, moving it back
>   after the reflections darkens **8.72 % of the frame by more than 10/255** (2.99 % by more than
>   30, max 204) while the frame mean moves by 0.48 — i.e. it does not darken the image, it
>   restores occlusion where it was missing. The difference map is concentrated on the FOLIAGE and
>   on the arch edges.
> - ⚠️ A global multiply is not the physically right instrument (an occlusion for the diffuse and
>   a specular occlusion for the reflections are different terms) — it is the same simplification
>   UE4 makes. The slot order is the best available placement for it, not a proof that the term
>   is correct.
>
> **A slot holds as many occupants as the application builds** — several RTGI and several SSGI
> with different `Parameters`, all resident so a runtime switch compares them on the very same
> framing — **of which AT MOST ONE IS ENABLED**. That exclusivity is mechanical, not a convention:
> `PostProcessEffect::enable()` is virtual, `IndirectPostProcessEffect` overrides it, and enabling
> an effect asks its stack to `disableSlotSiblings()`. **Enabling one is SELECTING it.**
> `EffectSlot::Custom` is the single multi-occupant slot, the extension point for an application
> effect the engine has no concept for.
>
> - ⚠️ **The requirement aggregation ignores `isEnabled()`** (`requiresAlbedo()`, `requiresHDR()`,
>   …): the attachment snapshot the Renderer takes on the frame the scene target is created must
>   cover EVERY alternative, or selecting a disabled one later would find no attachment. Concrete
>   case: `SSGI` declares `requiresHDR()`, `RTGI` does not. The cost is the attachments of an
>   alternative that never runs — the price of the runtime A/B, and it is what the old
>   "create everything enabled, select after a 200 ms timer" trick was paying blindly.
>   `hasEnabledReflectionProvider()` / `hasEnabledIndirectDiffuseProvider()` keep filtering on
>   `isEnabled()`: they are about what RUNS, not about what is allocated.
> - ⚠️ **The four camera slots are REFUSED to `addEffect()`** with a trace error. `DepthOfField`,
>   `MotionBlur`, `Glare` and `ToneMapping` are materialized by `syncCameraEffects()` from the
>   camera's own switches, which owns their lifetime — an application-added one would be
>   destroyed under its feet at the next camera change. That method lost its
>   erase → `find_if(runsAfterToneMapping)` → insert dance: it assigns four slots.
> - ⚠️ `runsAfterToneMapping()` is now DERIVED (`slot() == EffectSlot::PostToneMapping`) instead
>   of being an independent virtual that could disagree with the effect's position.
> - ⚠️ The back-pointer an effect keeps to its stack is RAW, and safe by who clears it: the stack
>   clears it on removal, in `clearEffects()` and in its own destructor, always while it still
>   holds a `shared_ptr` to the effect. An effect DOES outlive its stack — a demo keeps copies to
>   toggle it.
