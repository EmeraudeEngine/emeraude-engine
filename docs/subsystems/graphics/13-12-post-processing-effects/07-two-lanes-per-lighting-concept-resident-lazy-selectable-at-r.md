## 12. Post-Processing Effects

### Two LANES per lighting concept — resident, lazy, selectable at runtime (Sep 2026)

Each of the four lighting concepts exists in two implementations: a **screen-space lane** and a
**ray-traced lane**. `PostProcessStack::installLightingFamily(renderer)` files **both** into their
slots and selects one; an application never names an effect.

| Slot | ScreenSpace lane | RayTracing lane |
|---|---|---|
| `ContactShadows` | `SSContactShadows` | `RTContactShadows` |
| `IndirectDiffuse` | `SSGI` | `RTGI` |
| `Reflections` | `SSR` | `RTR` |
| `AmbientOcclusion` | `SSAO` | `RTAO` |

**A lane is not a taxonomy.** An effect is ray-traced **iff** `requiresRayTracing()` is true.
Nothing else says so, and nothing else may — a second way of asking is a second way of getting a
different answer.

**Residency vs. creation vs. selection**, three distinct things that used to be one:

- **Resident** — filed into a slot. The screen-space lane always is; the ray-traced lane is only
  when the two *session-constant* conditions hold (`device()->rayTracingEnabled()` **and**
  `Renderer::isRayTracingSettingEnabled()`). ⚠️ `isRayTracingReady()` is deliberately **not**
  consulted here: it answers a question about *this frame*.
- **Selected** — what the owner wants, one index per slot, written from the **main thread**
  (console, an input handler) and read by the render thread. Written *only* through
  `selectOccupant()` / `selectNoOccupant()` / `selectLightingLane()`.
- **Created** — holds GPU resources. **Lazy**: an occupant nobody selected allocates nothing.
  `PostProcessStack::syncSlotSelection()` materializes the one it is about to enable.

**`syncSlotSelection()` is the SINGLE site where a chain effect is enabled or disabled.** It runs
on the render thread, once per frame, between `syncCameraEffects()` and `syncSlotPairings()`. That
split — intent on one thread, application on the other — is what makes a runtime switch safe.
Calling `enable()` from a key handler or a console command races the chain walk.

**The scene-effects bypass (2026-09-26)** rides the same site: `PostProcessStack::bypassSceneEffects(bool)`
records an atomic intent, and `syncSlotSelection()` gives every `isSceneEffectSlot()` slot NO effective
occupant while it holds (the multi-occupant `Custom` slot switches off exactly the members that were on,
and back). Selections, concept gates and lane are never written, so lifting it restores the chain as it
was, and every reader of an ENABLED effect follows by construction — `requiresJitter()` stops the TAA
jitter, `hasEnabledIndirectDiffuseProvider()` gives the raster its diffuse IBL leg back. The camera chain
(DoF, MotionBlur, LensFlare, Glare, ToneMapping) keeps running: the sensor is not an effect. It replaces
`PostProcessor::enable(false)` as the user's "no effect" switch — that master switch forces the DIRECT
path and shows an unexposed frame (`docs/caution-points.md` § *KeyPad4 broke every lit shader*).

**Automatic fallback.** When the selected occupant cannot run, the slot falls back to the first
sibling that can, and recovers on its own when the selected one becomes runnable again. The
predicate is `canOccupantRun()`, which carries **the same three ray-tracing conditions the
executor skips on** — they must stay identical, or the fallback and the recorder disagree.

> [!CAUTION]
> **The fallback and the lazy residency will eat each other without a grace period.**
> `isRayTracingReady()` is false for the first frames of *every* scene (async TLAS build) and
> cannot distinguish "not yet" from "never" — it is a null/created test on the current TLAS.
> Falling back on the first stalled frame therefore materialized the **entire screen-space lane on
> every launch** of a ray-traced scene, seconds before the traced one took over: measured on
> Sponza, all three screen-space effects reported `created` while the traced lane was the one
> running. `FallbackGraceFrames` (120) is what buys the warm-up out, while still rescuing a scene
> that genuinely holds no ray-traced geometry.

> [!CAUTION]
> **A lane switch costs no reconfiguration, and that is a property of `requiresXXX()`.** The
> stack aggregates its G-buffer requirements over **every resident** effect, selected or not, so
> the scene target always carries the union of what both lanes need. Break that — gate any
> `requires*()` on `isEnabled()` — and every switch starts recreating the scene target under the
> render thread.

> [!CAUTION]
> **`resizeAll()` must skip an occupant that was never created.** `resize()` destroys *then*
> creates and the caller raises the created flag on its result, so a resident alternative nobody
> selected would **materialize itself at the first window resize** — the laziness evaporating on a
> gesture that has nothing to do with it. Same reason `createAll()` creates only what is selected
> and `destroyAll()` only destroys what exists.

**Console** — the stack is a `Console::ControllableTrait` registered by `Scenes::Manager` on scene
activation, so the active scene's chain is addressable as `Core.SceneManagerService.PostProcess.*`:
`listEffects()`, `getStatus()`, `select(slot, effect)`, `disable(slot)`,
`setLightingMode("ScreenSpace"|"RayTracing"|"None")`. A selection is applied on the next frame, never
immediately. `getStatus()` opens with the lane the family stands on (`Lane selected: …`) and marks a
concept its gate holds off — see the caution *The per-concept gate* below. ⚠️ A stack the renderer materializes on its own (`Scene::requirePostProcessStack()`,
the camera-only path) is **not** registered — that call runs on the render thread and mutating the
console tree from there would race the main thread reading it.

⚠️ The four camera-owned slots carry **no selection** (`syncSlotSelection()` skips them, the camera
owns their lifetime). The console reports them from the effect's own state and labels them
`camera-owned`; reading them from the selection indices made it print `ToneMapping: off` on a
visibly tone-mapped frame.
