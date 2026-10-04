## LightSet & Background-Derived Lighting (Jul 2026)

**The "static lighting" mode was REMOVED** (`Saphir::StaticLighting`, `enableAsStaticLighting()`,
`isUsingStaticLighting()`, the `SimplePass` remap and its program-cache-key bit). It was a
performance shortcut (one forward pass with a single light baked as GLSL literals) predating the
photometric migration; owner decision: more trouble than it was worth. `RenderPassType::SimplePass`
is now strictly UNLIT (light set disabled or instance lighting disabled). The `LightSet` is a pure
aggregator: lights + photometric ambient (`setAmbientLightColor()` — a CHROMATICITY since 2026-09-25, scaled to unit
luminance before it reaches the GPU, `ambientEmissionChromaticity()` — + `setAmbientLightIntensity()` in LUX, the
delivered illuminance whatever the hue; `Graphics/AGENTS.md` § *A light colour is a CHROMATICITY*).

⚠️⚠️ **A DISABLED light set lights NOTHING** — not an ambient-only mode. `LightSet::initialize()` returns early (no
light is created on the hardware) and the render lists file every instance as unlit
(`isLighted = lightSet().isEnabled() && instance->isLightingEnabled()`), so a `Lighting::Lit` Visual draws its raw
albedo. Installing a background does not enable it; `LightSet::enable()` (or `applyBackgroundLighting()`) must run
BEFORE the scene is enabled. Since 2026-09-25 `initialize()` WARNS when lights were added to a disabled set
(`… yet N light(s) were added to it: they will light nothing !`) — it used to be one Info line, and a demo's three
fire lights were dead for an unknown time.

**Light lifetime across the two threads (2026-09-03).** The light-set mutex guards the
CONTAINERS, nothing else. The render thread (`renderLightedSelection()`) takes it ONCE per call to
SNAPSHOT the three light lists, then records with the copies; the logic thread
(`updateCSMCascades()`) takes it once per tick to copy the CSM lights, then refits the cascades
outside it. ⚠️⚠️ It used to be taken **per render batch**, around the ambient and every light pass
— held while command buffers were recorded (most of the frame with the validation layers on) —
while the logic thread took the same mutex every tick: measured as one futex block per tick of the
logic thread on `game-logic`. **Never hold a shared mutex while recording.** Two consequences the
snapshot forces, both of which crashed the first attempt (`pure virtual method called` on the
render thread the moment a barrel exploded):

- **A light can outlive its entity on the render thread.** `Node::destroyTree()` →
  `clearComponents()` → `LightSet::remove()` runs on the logic thread while a frame still holds the
  snapshot; the node is then destroyed and the component's `m_parentEntity` dangles. Hence
  `AbstractLightEmitter::touch(sphere, readStateIndex)`: the render-thread culling reads position
  AND radius from the **published block** of the latched slot (`publishedBlock()`), never from the
  parent entity. **No render-thread code may reach a light's parent entity.**
- **`LightSet::remove()` RETIRES, it does not destroy.** The light is erased from the sets and
  pushed to `m_retiredLights` stamped with the render frame counter; `destroyRetiredLights()`, called
  from `Scene::beginRenderFrame()` **behind the in-flight fence**, destroys the hardware of lights
  retired more than `framesInFlight()` frames ago, on the render thread. This also closed a defect
  older than the snapshot: `destroyFromHardware()` used to run synchronously on the LOGIC thread,
  resetting a shadow descriptor set and freeing a shared UBO element that frames in flight were
  still reading. `removeAllLights()` (scene teardown) drops the retired list with the others.

**Sky → LightSet bridge (OPT-IN)**: `Scene::applyBackgroundLighting(BackgroundLightingOptions)`
derives the scene lighting from the background photometric manifest — ambient = the ambient illuminance
in the hue of the average colour (a unit-luminance chromaticity since 2026-09-25), plus one `StaticEntity` + `DirectionalLight` per declared celestial body
(`Graphics::CelestialBody`; the entity sits at `direction × 1000`, the component default shines
along `-normalize(position)`). The first star becomes `mainDirectionalLight`. Options carry the
NON-photometric choices only: `applyAmbient`, `applyStars`, and the shadow policy as a
`DirectionalShadowOptions` sub-struct (`options.shadows`, since 2026-09-13 — the same type
`SceneDataConsumer::setDirectionalLightShadows()` takes for an asset's own directional lights;
call sites write `{.shadows = {.shadowMapResolution = 2048, .shadowCoverage = 500.0F}}`). Scene
JSON opt-in: `"ApplyLighting": true` in the `Background` block; console:
`setBackground(name, true)`.

⚠️ **A body declared IN the texture is masked out of the IBL bake (2026-09-13).**
`Scene::environmentStarMask()` picks the brightest star with `InTexture: true` and hands
`Graphics::Compute::IBLBaker::StarMask` (direction, cone half-angle = the body's angular diameter)
to `bakeEnvironment()`, which reads the sky at the rim of that cone instead of inside it, in both
the irradiance and the prefiltered chains; the visible skybox keeps the body. Why: the analytic
directional light derived from the manifest already carries that energy WITH shadows, and a bake
that kept it lit every surface a second time, unshadowed — on Kloppenheim 05 (the Sponza sky) the
in-texture sun is 80 % of the illuminance the whole sky pours on the ground. The mask is part of
the bake identity (`m_IBLBakedStarMask`): a manifest change re-bakes.
`applyStars = false` (Sponza: the asset's own `SUN` is the sun) still masks the body — the mask
follows the MANIFEST, not the stage, which is what makes the two paths consistent.
⚠️ **Since 2026-10-05 the SAME mask applies to the ray-traced lanes** that read the raw cubemap
(RTGI's and the irradiance probe volume's sky term, `Graphics/Effects/Shared/StarMaskGLSL.hpp`):
unmasked, a ray landing on the disc brought the sun back unshadowed — whole-frame probe flashes on
Sponza (`docs/caution-points.md` § The in-texture sun was counted twice by the ray-traced lanes).

⚠️⚠️ **FIXED 2026-09-25 — the store manifests' `Direction` vectors were Y-DOWN legacy** (found
2026-09-13): 13 manifests declared a negative Y where the doc says "toward the body, UP = +Y"; the
derived entity sat BELOW the ground and the star shone UPWARD (measured on `basic-scenery
--demo-options 1`: 50 klux sun, no ground shadow at all), and the mask of the IBL bake followed the
same wrong direction — a hole in empty sky while the painted body stayed in. Every body was measured
in its picture (`tools/sky-manifest.py --locate`), see `src/Graphics/AGENTS.md` § Background
photometric contract. ⚠️ A demo with `applyStars = false` (`terrain`, `sponza`, `relief`) still
depends on that direction: the mask is where its own sun stops being doubled by the painted one.

⚠️ **Threading contract (rewritten Jul 2026 after two live crashes)**: the entry point only
RAISES a request (`m_backgroundLightingRequested`, atomic) — it may be called from ANY thread
(console TCP thread, input callbacks, demo constructors). The actual application
(`applyBackgroundLightingNow()`: entity/light creation, LightSet, view UBOs) happens exclusively
at the top of `Scene::processLogics()` (logic thread), which POLLS the request and honors it once
the background resource `isLoaded()` — no observer, no notification race. Re-application
(background switch) first removes the star entities recorded in `m_backgroundStarEntities`, so
switching skies never stacks directional lights.

**Environment luminance is a View UBO value, not a baked literal**:
`Scene::refreshAmbientLightProperties()` pushes ambient color + intensity + background luminance
to EVERY render target's view UBO (main, render-to-view, render-to-texture — offsets
`EnvironmentLuminanceOffset` in the three `ViewMatrices*UBO`). Called at `LightSet::initialize()`,
`Scene::setBackground()`, background `LoadFinished`, and by `applyBackgroundLightingNow()`.
See `src/Graphics/AGENTS.md` § "Background photometric contract" for the manifest schema.

**IBL ambient replaces the scalar under the applyAmbient contract (Jul 2026, IBL lot 3)**:
when `applyBackgroundLighting({applyAmbient: true})` runs, `refreshAmbientLightProperties()`
pushes a **ZERO** scalar ambient intensity to the view UBOs (the baked irradiance cubemap
takes over in the ambient pass — a directional E(n) and a flat scalar would double-count
the same sky) while the LightSet keeps the photometric values for effects reading it
directly. With `applyAmbient: false` (RTGI demos, manual ambient) the irradiance slot is
NOT published (parked on the default black cubemap) and the scalar path stands alone —
`Scene::updateEnvironmentIBL()` re-evaluates that publication every tick, so a scene can
switch lighting modes after the bake. `Scene::setBackground()` now writes the bindless SET
directly when the cubemap is already created (the per-frame sync mirrors it; the late
adoption covers async loads) — the IBL re-bake is keyed on that set identity.

**Environment IBL follows the adopted cubemap (Jul 2026, IBL lot 2)**:
`Scene::updateEnvironmentIBL()` is polled every `processLogics` tick (idle cost: one mutex
lock + a pointer compare). When the identity of `BindlessTextureSet::environmentCubemap()`
changes — `setBackground()` can run on any thread, the late adoption of an async-loaded
cubemap runs on the render thread, so the mutex-protected set is the source of truth — the
scene bakes irradiance + prefiltered cubemaps through `Renderer::iblBaker()` (blocking GPU
job, ~1 ms) into a scene-owned **ping-pong pair** of `Graphics::IBLTexture` (frames in
flight keep sampling the published pair), then publishes via
`setIrradianceCubemap()/setPrefilteredCubemap()` (reserved bindless slots 1 and 2). The
engine default black cubemap is never baked. Failures mark the source as attempted — no
retry storm. See `src/Graphics/AGENTS.md` § "Graphics/Compute/IBLBaker".
