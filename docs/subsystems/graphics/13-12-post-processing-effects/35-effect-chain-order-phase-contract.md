## 12. Post-Processing Effects

### Effect Chain Order & Phase Contract (Jul 2026)

The chain has THREE phases, enforced structurally:

1. **Scene effects (HDR, linear)** — declared by the application/demo stack: contact shadows,
   GI, reflections, AO, volumetric light, fog, then the temporal resolve.
2. **Photographic effects (HDR resolve)** — materialized by the ACTIVE CAMERA
   (`enableDepthOfField()`/`enableHDR()`, see "Physical Camera" below): DepthOfField,
   MotionBlur, Glare, then ToneMapping (HDR→LDR). ⚠️ Since Sep 2026 `LensFlare` sits INSIDE
   this phase (between MotionBlur and Glare) while remaining APPLICATION-owned — a phase is a
   region of the chain, not a statement about who creates the effect. `isCameraEffectSlot()`
   names the four camera slots explicitly for exactly that reason.
3. **Post-tonemap effects (LDR, display-referred)** — effects overriding
   `IndirectPostProcessEffect::runsAfterToneMapping()` (FXAA, FXAASharpen, Sharpen).

> [!CRITICAL]
> `PostProcessStack::syncCameraEffects()` inserts the camera effects BEFORE the first
> `runsAfterToneMapping()` effect. Running AA/sharpen on linear HDR input produces severe
> posterization and halo streaks (observed live on Sponza, Jul 2026) — any new LDR effect
> MUST override `runsAfterToneMapping()`.

**The order lives in ONE place: the `EffectSlot` enum** (§ "The chain order is a STRUCTURE",
above). ⚠️⚠️ This section used to restate it as a list — `RTR|SSR → RTAO|SSAO → RTGI|SSGI →
ContactShadows → AtmosphericFog → VolumetricLight → LensFlare` — and that list had been **wrong
since the Aug 2026 redesign**: it put the reflections before the indirect diffuse and the AO
before both, i.e. the exact arrangement the redesign fixed as a correctness bug, and it survived
two more revisions because nothing links the two sections. **Do not restore a second ordering
here.** A scene no longer declares an order at all: `addEffect()` files into a slot and the call
sequence has no effect.

**Rationale, per slot**, is documented on the enum members themselves, next to the code that
enforces it. The one claim worth repeating because it is a KNOWN simplification: the AO snippet
is a global multiply, so it attenuates the direct light as well as the two indirect terms. ⚠️ The
earlier revision justified that with *"matches UE4's approach"* — **that attribution is wrong**:
UE4/UE5 apply SSAO/GTAO to the INDIRECT lighting (`r.AmbientOcclusionStaticFraction` only widens
it to the baked term), not to dynamic direct lighting. Separating the terms needs an ambient lane
the single HDR colour buffer does not have, so the current placement is the best available for a
global multiply — not a proof that the term is right. **OPEN**, and unmeasured on this codebase.

> [!CAUTION]
> **This list is the one the scenes build; the previous revision matched NO scene and carried a
> rationale the code does not implement.** It read `RTR → SSR → ContactShadows → SSAO →
> AtmosphericFog → VolumetricLight → LensFlare → VeilingGlare`, which differed on three counts, all
> verified against `Sponza`, `Citadel`, `WaterWorld` and `LightAndShadowDebug`:
> - it put **ContactShadows before AO**; every scene puts AO first;
> - it **omitted GI entirely** (`RTGI|SSGI`), which every scene inserts after AO;
> - it ended the SCENE stack with **VeilingGlare** (then named `Bloom`), which is not a scene effect: veiling glare is a LENS
>   phenomenon carried by the active camera (`enableHDR`/glare threshold), and adding it to the
>   scene stack would run it before the defocus.
>
> ⚠️ **"AtmosphericFog before VolumetricLight so god rays bloom through the fog" was FALSE.** The
> shafts cannot be attenuated by an effect placed before them: `VolumetricLight::producesOverlay()`
> is true, its combine snippet is a pure add (`em_Color.rgb += texture(vlightTex, vUV).rgb`), and
> `recordOverlayPasses()` takes its `inputColor` **unnamed and unused** — the shafts are built from
> the depth occlusion mask and the light colour/intensity alone and never see the chain.
>
> **Acted on in Sep 2026, in the OTHER direction: `VolumetricLight` now precedes `Fog`**, so the
> fog's `mix()` finally applies an extinction to the shafts instead of leaving them on top of a
> fogged background. ⚠️ What that buys is the transmittance of the SCENE DEPTH — an upper bound on
> the true one, so a shaft scattered close to the camera is now over-attenuated. Feeding the fog's
> transmittance into the shaft integration remains the correct fix and stays **OPEN**; the reorder
> is a bounded, free improvement over no extinction at all, not that feature.
>
> ⚠️ An effect that overrides `readsChainColorUpstream()` — `LensFlare` does, for its bright pass —
> IS genuinely sensitive to what precedes it: `PostProcessor` flushes the pending combine group so
> the effect sees the real chain colour. That coupling is real, unlike the VolumetricLight one. But
> the conclusion drawn from it — that the LDR fog was starving LensFlare's 2000-nit threshold and
> deleting the flares — is **FALSE, and was disproved by measurement before it could be committed**.
>
> Four states were shot looking straight at the sun, same pose: fog off; fog fixed; fog with its
> luminance forced back to 1.0 (the LDR bug re-emulated); and the fog ALSO given back its unsigned
> `tanHalfFovY`. **The halo ring and the shafts are present in all four.** The fog never gagged
> LensFlare. What hid the flare was the DEMO's exposure: at f/5.6 with auto-exposure, 42 % of the
> frame clipped and a soft grey ring on a white sky is invisible by construction. Pinning the triad
> (`6018bc7`) is what revealed it — the owner had never seen this effect run.
>
> ⚠️ **Two rules.** A plausible causal chain between two real defects is still a guess: "the fog is
> LDR" and "the flare is missing" were both true and unrelated. And when an effect appears to come
> back after a fix, suspect the EXPOSURE before crediting the fix — on an auto-exposing camera the
> sensor is the loudest variable in the frame.
