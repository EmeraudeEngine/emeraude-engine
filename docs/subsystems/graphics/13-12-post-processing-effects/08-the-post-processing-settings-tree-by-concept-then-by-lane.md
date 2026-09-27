## 12. Post-Processing Effects

### The post-processing settings tree — by CONCEPT, then by lane (Sep 2026)

Every post-processing setting lives under `Core/Graphics/PostProcessing/`, ordered the same way the
code is: **concept first, mechanism second**.

```
Core/Graphics/PostProcessing/
├── LightingLane                       "Auto" (default) | "RayTracing" | "ScreenSpace" | "None"
├── CutFrameAroundTranslucency
├── DebugNonFinite                     false (default): ON, SSR paints WHICH input is NaN/Inf
│                                      (red grabbed colour, blue pyramid, green trace, magenta
│                                      mix) and TAA paints in red any pixel whose 3x3 holds one;
│                                      read at effect creation. The instrument of the 2026-09-13
│                                      black squares — see caution-points.
├── <Concept>/Enabled                  ContactShadows, IndirectDiffuse, Reflections, AmbientOcclusion
├── <Concept>/<param>                  knobs COMMON to both lanes — same meaning, same unit, same
│                                      default — read by both occupants (IndirectDiffuse/MaxDistance,
│                                      …/Intensity, …/SampleCount, …/Temporal/*, …/Denoiser/*,
│                                      ContactShadows/MaxDistance|NormalBias|Intensity|MaxBlurRadius,
│                                      AmbientOcclusion/Intensity)
├── <Concept>/RayTracing/<param>       the ray-traced lane's OWN knobs (ray bias, PixelDoubling,
│                                      MultiBounce/*, GlossyCone/*, Reflections' Temporal/* —
│                                      lane level because SSR has no accumulation — …)
├── <Concept>/ScreenSpace/<param>      the screen-space lane's OWN knobs (march StepCount, Thickness,
│                                      depth Bias, hemisphere Radius, …)
├── DepthOfField/Enabled               user refusal of the two expensive photographic effects;
├── MotionBlur/Enabled                 it OVERRIDES the camera (see below)
├── Clouds/Enabled                     true (default): false DECLINES the scene-driven cloud pass
│                                      for the session (read once, the first frame a cloud exists)
├── Clouds/ShadowsEnabled              true (default): the clouds' Beer shadow map on the lit
│   Clouds/ShadowResolution            materials; 1024 texels, read once, the first frame a cloud is drawn
│   Clouds/ShadowCoverage              the FLOOR of the side (1024 m): the map sizes itself on the clouds
└── <Effect>/<param>                   single-lane effects: TemporalAA, VolumetricLight,
                                       DepthOfField, MotionBlur, Clouds (StepCount 64,
                                       LightStepCount 6, GroundAlbedo 0.2)
```

This replaced a tree split **by mechanism at the top level** (`Core/Graphics/RayTracing/…` vs
`Core/Graphics/ScreenSpace/…`) — the very organisation this engine already rejected for the code in
Sep 2026 when `Effects/Display|Lens|Framebuffer/` became `Effects/Lighting|Atmosphere|Resolve|Camera|Style/`.
The settings were the last place still thinking the old way, and it showed: one concept appeared
under two unrelated roots with key sets that had silently diverged (`RayTracing/AmbientOcclusion/`
carried 7 keys, `ScreenSpace/AmbientOcclusion/` carried 4, and nothing in the tree said they were
two implementations of one thing). Post-processing was also spread over **six** roots —
`RayTracing/`, `ScreenSpace/`, `MotionBlur/`, `DepthOfField/`, `VolumetricLight/`,
`AntiAliasing/Temporal/` — plus one key already sitting at `PostProcessing/`.

**Vocabulary rules, so the settings file and the console never need translating:**

- Section names are `EffectSlot` names. Hence `IndirectDiffuse` (not `GlobalIllumination`, which
  also covers the specular this effect does not do) and `Reflections` (not `Reflection`). The
  console prints those same words in `getStatus()` and takes them in `select(<slot>, <effect>)`.
- Lanes are spelled `RayTracing` / `ScreenSpace` in full — the words `LightingLane`, `to_cstring()`
  and `setLightingMode()` already use. Never `RT` / `SS` in a key. (C++ *symbols* do abbreviate the
  lane — `GraphicsPPIndirectDiffuseRTBiasKey` — because they are not user-facing and the file
  already abbreviated; the string never does. A concept-level key carries no lane at all:
  `GraphicsPPIndirectDiffuseIntensityKey`.)
- **A parameter common to both lanes is declared ONCE, at the concept level** (owner decision,
  2026-09-12). "Common" = present in both lanes with the same meaning, the same unit and the same
  default; the two occupants read the one key. What stays under a lane describes THAT lane's
  mechanism: a ray-origin bias against a depth-comparison bias (`AmbientOcclusion/RayTracing/Bias`
  vs `…/ScreenSpace/Bias` — different quantities, kept apart), a march `StepCount`/`Thickness`, a
  `GlossyCone`, `MultiBounce`, and `PixelDoubling` (same meaning, but a per-lane trade-off the
  owner set differently: true for the traced effects, false for SSR). `AmbientOcclusion/SampleCount`
  stays per lane too: 8 rays and 32 depth taps are not the same budget.
  ⚠️ Until Sep 2026 the shared knobs were declared twice and their defaults had drifted
  (`IndirectDiffuse` range 5 m against 8 m) — an A/B of the lanes was an A/B of settings. The
  first slot to follow the rule was `ContactShadows` (its four knobs were born shared); it is now
  the rule for every slot. ⚠️ projet-alpha does not reset its settings: a `settings.json` written
  before the change keeps the old `<Concept>/<Lane>/<param>` entries as orphans — the owner's was
  migrated by hand (values kept, orphans dropped).

> [!CAUTION]
> **The per-concept gate — `<Concept>/Enabled` holds across lane switches (fixed 2026-09-13,
> owner-reported).** The stack keeps one boolean per slot, `m_conceptEnabled`, that says WHICH
> concepts run; the lane says HOW. `installLightingFamily()` fills the four lighting gates from
> their settings *before* selecting the lane, `selectLightingLane()` gives the lane's occupant only
> to the concepts whose gate is on, and `selectNoLightingLane()` (the live `setLightingMode("None")`,
> the launch `LightingLane = "None"`, a cycle's "family off" step) switches the family off without
> touching any gate — so the next lane brings back exactly the concepts that were on. The gate is
> edited by the explicit per-slot intents only: `selectOccupant()` / `addEffect()` switch a concept
> ON for the session (an explicit `select(Reflections, SSREffect)` on a concept the settings had
> switched off makes it follow the lanes from then on), `selectNoOccupant()` (console `disable()`)
> switches it OFF for the session — a lane switch leaves it off too.
> ⚠️ Until 2026-09-13 the gates were applied AFTER the lane as a plain `selectNoOccupant()`, on the
> doctrine *"the settings decide how the scene starts, the console owns the session"*: the first
> `setLightingMode()` or KeyPad9 brought a disabled concept back, and the owner read
> `Reflections/Enabled = false` as ignored — it was, after one press. ⚠️ The refusal of
> `selectLightingLane()` is about RESIDENCY, never about the gates: a lane every concept has
> switched off is still selected and the family stays dark by choice; `getStatus()` says
> `<Concept>: off  (concept switched off — a lane switch leaves it off; …)` so that this "off" is
> not read as a fallback. Verified 2026-09-13 through `getStatus()` on `light-and-shadow-debug` and
> `asset-loader`: setting → two switches → `select()` → two switches → `disable()` → switch →
> `None` → lane, every step as stated, 0 VUID.

> [!CAUTION]
> **`LightingLane` decides whether the acceleration structures exist AT ALL — it is a launch
> decision, not a starting preference.** `Renderer::onInitialize()` creates the
> `AccelerationStructureBuilder` only when the device is capable **and** the lane is `"Auto"` or
> `"RayTracing"` (owner decision, 2026-09-10). A BLAS is built when a geometry **loads**
> (`Geometry::Interface::onDependenciesLoaded()`, which skips when the builder is null) and a
> geometry cannot gain one afterwards. Consequences, all deliberate:
> - With `"ScreenSpace"` or `"None"`, nothing can be traced for the whole session.
> - `installLightingFamily()` therefore asks **`renderer.accelerationStructureBuilder() != nullptr`**,
>   not `device()->rayTracingEnabled()`, to decide residency — one pointer, one truth. The traced
>   lane is simply not filed, so `listEffects()` shows it absent instead of offering a lie.
> - `setLightingMode("RayTracing")` is **refused**, atomically, with an error naming the key.
>   ⚠️ It used to switch the whole lighting family OFF and answer "selected": `selectLightingLane()`
>   applied slot by slot and discovered there was nothing to select on the way. It now resolves
>   every slot first and returns `false` without touching anything.
> - Going to a traced lane is a **relaunch**. By design.
>
> **The key this replaced.** `Core/Graphics/RayTracing/Enabled` was deleted (2026-09-10, owner
> decision): it said the same thing as `LightingLane` while silently **dominating** it, and the
> owner hit it on the first settings reset — *"la clé est RayTracing alors que je vois le mode en
> ScreenSpace"*. ⚠️ Its documentation here also **overstated** what it gated: the
> `SHADER_DEVICE_ADDRESS | ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY` usage flags
> (`VertexBufferObject.cpp`), `createRTDescriptorSet()` and the descriptor-pool AS entry were, and
> still are, gated on **`Device::rayTracingEnabled()` alone** — pure hardware detection, since the
> RT extensions are requested from `physicalDevice->supportsRayTracing()` without consulting any
> setting (`Instance.cpp`). The key's only unique job was the builder, which is what moved.
> `Core/Graphics/RayTracing/` survives for ray tracing that is **not** an effect — `TLASDistance`,
> `IrradianceProbes/*` and, since 2026-09-22, **`TerrainBLASMaxTriangles`** (default 2 000 000): the
> triangle budget of the proxy an ADAPTIVE terrain grid is given in the BLAS. That geometry's index
> buffer holds every LOD of every sector at once, so the proxy is regenerated from the grid at the
> finest step that fits the budget — read at BLAS build time, i.e. once per geometry.
> ⚠️⚠️ **It is not a preference, it is a VRAM ceiling**: the full-resolution surface is quadratic in
> the division count and unbounded. Measured on `terrain` (4096 divisions): step 1 = 33.5 M triangles
> and **7.5 GiB** of VRAM, against **5.05 GiB** at the default step 8 (524 288 triangles), idle 1.9 GiB
> on an 8 GiB card. `forest` (128 divisions) is 32 768 triangles and keeps its exact surface.
>
> **Two rules the episode leaves behind**, beyond this key:
> - **`LightingLane` defaults to `"Auto"`**, never to a lane. Any fixed default makes a freshly
>   written file contradict itself on a machine that cannot honour it.
> - **An explicit request that cannot be honoured is TRACED** at startup, naming the reason.
>   `"Auto"` is a policy, not a request, and stays silent — warning on the default would train the
>   reader to ignore the warning. `getStatus()` carries the same information on its first line.
> ⚠️ Naming the key well was **not** a substitute for either: it was deliberately called
> `LightingLane` rather than `EnableRayTracing` precisely to avoid the confusion, and the confusion
> happened anyway, because the failure was silent.

> [!CAUTION]
> **A lane-wide section must never duplicate a per-effect key.** `PixelDoubling` exists four times
> today (IndirectDiffuse/RayTracing, AmbientOcclusion/RayTracing, Reflections/RayTracing,
> Reflections/ScreenSpace) — and with **different defaults**: `true` for the three ray-traced ones,
> `false` for the screen-space reflection. Hoisting it into a `PostProcessing/RayTracing/` group
> while leaving the per-effect copies would need a precedence rule, and an invisible precedence
> rule is the trap already recorded for `VolumetricLight` (§ Available Effects: an engine-wide
> default would have silently doubled five demos' god rays). A key lives at **exactly one level**.
> Consequently `PostProcessing/RayTracing/` and `PostProcessing/ScreenSpace/` do **not exist yet**:
> nothing today is genuinely lane-wide and nothing else, and an empty section in a settings file is
> a promise nobody keeps. Create them with their first real occupant.

> [!CAUTION]
> **The camera REQUESTS its photographic effects; two settings keys DISPOSE.**
> `PostProcessing/DepthOfField/Enabled` and `PostProcessing/MotionBlur/Enabled` override
> `Camera::enableDepthOfField()` / `enableMotionBlur()`: `syncCameraEffects()` will not materialize
> the effect when the key says no, so a demo turning it on for style is simply ignored. Those two
> are the expensive (7-pass and 4-pass) and intrusive ones — that is the whole reason they, and
> only they, get a switch.
> - ⚠️ Enforced in `syncCameraEffects()` because that is the **only** place either effect comes
>   into existence. The motion-blur refusal used to live in projet-alpha's `Player` at spawn time
>   and covered the player's own request alone: any demo calling `Camera::enableMotionBlur()`
>   directly walked straight past it. (The key was briefly named `MotionBlur/UserAllowed`, Sep 2026,
>   on the since-abandoned premise that a camera slot could not have a real `Enabled`.)
> - ⚠️ **`Glare` and `ToneMapping` get no such key, and must not.** Tone mapping is not an option
>   but the sensor response; refusing it sends raw photometric radiance to an LDR swap chain —
>   measured as a white screen in daylight and a black one at night.
> - ⚠️ **Both default to `true`** since 2026-09-13 (owner decision). `MotionBlur` defaulted to
>   `false` when the key was introduced, preserving the behaviour of the time, and was switched on
>   on purpose afterwards; this bullet said `false` until 2026-09-26.
> - ⚠️ **Camera motion blur smears every silhouette during a camera move, by design.** Rule it out
>   before blaming the TAA for a trail: pin the same EV at a 1/8000 s shutter. Measured 2026-09-26
>   on `basic-scenery`, a 1 m sideways step: 25-30 % of the pixels near silhouettes above 8/255 with
>   it, 5-7 % without, for exactly one frame per moving frame (so on every frame of a continuous
>   move). The TAA's own residual is the one that outlives the move.

**Switching lanes changes what the frame MEANS**, not merely how fast it was obtained. Verified on
Sponza (2880×1620, zero VUIDs): the two captures differ on 99.7 % of pixels, mean |Δ| 20/255, while
the mean luminance is unchanged (84.96 vs 84.97 — the auto-exposure absorbs it, so **a luminance
comparison cannot detect this switch**). The visible difference is the shadowed arcade, where the
two lanes measure the sky differently. ⚠️ This paragraph said *"SSGI has no sky term at all and it
crushes to black"* until Sep 2026: SSGI HAS a sky term since the sky-visibility pass, and the
failure it produced was the opposite one — the raster kept the diffuse IBL leg and lit the arcade
with the UNOCCLUDED sky. See § "Indirect-diffuse OWNERSHIP" and § "The screen-space sky visibility".
