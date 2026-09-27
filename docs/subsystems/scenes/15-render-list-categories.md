## Render List Categories

The Scene dispatches renderable layers into 7 render lists (defined in `Scene.hpp`):

| Index | Constant | Sort Order | Description |
|-------|----------|------------|-------------|
| 0 | `Opaque` | State-sorted (pipeline\|material\|geometry\|distance) | Opaque objects, no lighting. Special objects (sprites, InfinityView, depth-disabled) use distance-only fallback |
| 1 | `Translucent` | Back-to-front | Translucent objects (no grab pass), no lighting |
| 2 | `OpaqueLighted` | State-sorted | Opaque objects, with lighting. Same special-object fallback |
| 3 | `TranslucentLighted` | Back-to-front | Translucent objects (no grab pass), with lighting |
| 4 | `Shadows` | Distance | Shadow-casting objects |
| 5 | `TranslucentGB` | Back-to-front | Translucent objects requiring grab pass, no lighting |
| 6 | `TranslucentGBLighted` | Back-to-front | Translucent objects requiring grab pass, with lighting |

**Rendering order**: Opaque → Translucent → TranslucentGB (grab pass capture happens between Translucent and TranslucentGB).

**The background is drawn FIRST, and without depth.** `Scene::registerSceneVisualComponents()` gives
the background visual (`m_sceneVisualComponents[BackgroundVisualIndex]`, index 0)
`setUseInfinityView(true)` + `disableDepthTest(true)` + `disableDepthWrite(true)` (plus shadow
casting and receiving OFF). `populateRenderLists()` walks the scene visual components before the
nodes and the static entities and inserts them at distance `0.0F`; the background takes the
`isSpecial` branch of `insertIntoRenderLists()` (distance-only key, NOT the state-sorted composite),
so its key is `0` in the `Opaque` multimap — head of the list, drained first. The geometry is a
512 m cuboid (`Renderable::AbstractBackground::SkySize`) with flipped winding drawn on the
translation-free infinity view: it fills every pixel, and the level geometry drawn afterwards with
depth test + write ON simply overwrites it.

> [!CAUTION]
> **The sky's clip DEPTH is pinned to the far plane, and its geometric size is therefore
> irrelevant. Never "fix" a missing sky by growing the cuboid or by pushing the camera's far
> distance out.** `AbstractVertexStage::synthesizeVertexPositionInScreenSpace()` emits
> `gl_Position.z = gl_Position.w` for every infinity-view program (the standard skybox trick,
> `clipPosition.xyww` in the Khronos glTF Sample Viewer's `skybox.vert`); the projection maps near
> to 0 and far to 1, so `z = w` lands exactly on the far plane and can never be clipped.
>
> **Why this exists (measured Aug 2026).** Disabling the depth TEST does not disable Z CLIPPING.
> Before the pin, the 512 m cuboid put its faces 256 m from the camera and its corners at 443 m, so
> **any scene whose camera far distance fell under that lost its sky ENTIRELY** — silently: no
> error, no warning, no broken-renderable trace, and the environment cubemap still adopted and the
> IBL still baked, so the log looked perfectly healthy. Hit on `Viewers::ModelViewer`, whose far
> distance is derived from the model size (`max(100, radius * 20)`): every asset under ~22 m of
> radius rendered against a **bit-exact (0,0,0)** void, which also made every reflective,
> transmissive and clearcoat material unjudgeable in the viewer.
>
> ⚠️ **Attribution trap, lived twice on the way to this diagnosis.** A black surround is NOT proof
> that the background failed to load: measure the pixels. Bit-exact `(0,0,0)` means "never drawn";
> small non-zero values mean "drawn and crushed by the exposure" — two completely different bugs.
> And the first hypothesis here (far plane vs the 256 m faces) was **falsified** by a model whose
> far distance was 290 m and which still came out black; only a model at far 1547 m brought the sky
> back, because the corners sit at 443 m. A hypothesis that predicts the right answer for the wrong
> reason is worth nothing — vary the ONE parameter and check both outcomes.

That is what lets a loader emit **no geometry at all** where the sky must show through — the `F_SKY1`
sectors of a Doom map are holes on purpose, no stencil and no portal involved. With no background
installed those pixels are opaque black, never garbage. See [`@Scenes/Loaders/AGENTS.md`](../../../src/Scenes/Loaders/AGENTS.md)
→ WADLoader.

**Dispatch logic** in `Scene::insertIntoRenderLists()`:
1. `renderable->isOpaque(layerIndex)` → Opaque/OpaqueLighted
2. `renderable->requiresGrabPass(layerIndex)` → TranslucentGB/TranslucentGBLighted
3. Otherwise → Translucent/TranslucentLighted

**Code references:**
- `Scene.hpp` — Constants and `m_renderLists` array (7 elements)
- `Scene.rendering.cpp:insertIntoRenderLists()` — 3-way dispatch
- `Scene.rendering.cpp:populateRenderLists()` — Clear and populate all 6 non-shadow lists

**Populate gate exclusions** (`checkRenderableInstanceForRendering()`, Aug 2026): before the
readiness checks, an instance is skipped for a given target when (1) the caller registered it in
the target's manual exclusion list (`RenderTarget::Abstract::excludeFromRendering()`), or (2) —
**automatic, no registration** — its material `samplesTexture()` the Texture/Cubemap target being
populated. Rule 2 is what keeps a probe self-sampling feedback loop structurally impossible: on
Apple Silicon that loop is a GPU fault → `DEVICE_LOST`, not a mere artifact. See engine
`docs/caution-points.md` § "Probe self-sampling" and `docs/reflection-pipeline.md` § 2.3 fix 4.
