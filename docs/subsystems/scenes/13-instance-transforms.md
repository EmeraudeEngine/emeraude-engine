## Instance Transforms (SceneInstanceTransforms)

`SceneInstanceTransforms` owns the per-scene **InstanceTransforms SSBO** (one buffer per
frame-in-flight, `SceneMetaData::initializePerFrameBuffers()` pattern). It is the B1 step of
the motion-vectors chain (its design is recorded in `docs/caution-points.md`): move the non-instanced model matrices out of
push constants into a per-instance SSBO indexed by `gl_BaseInstance`, and carry
`{model, previousModel}` per entry for temporal effects (TAA, RTGI reprojection, motion blur).

**GPU layout** (std430, header `static_assert`-ed at 128 B):
`Header {mat4 previousViewProjection; mat4 previousViewProjectionInfinity;}` followed by
`Entry {mat4 model; mat4 previousModel;}[]`. The header is reserved for the motion-vector pass and
written **only** by the primary view target (`RenderTargetType::View`); the regular matrix path
keeps pushing the view-projection matrix through push constants (MDI precedent) and only reads the
entries. The second matrix serves the renderables drawn with the **translation-free infinity
view** (the sky): their current clip position comes from the pushed infinity view, so their
previous one must match, or the velocity is off by the whole camera translation even on a static
camera. The header used to hold the CURRENT view-projection in that slot — read 0 times by the
generated GLSL, hence recycled at no size cost.

⚠️ **Both header matrices MUST be UNJITTERED** — stage them from
`ViewMatricesInterface::unjitteredProjectionMatrix()`, never from `projectionMatrix()`, which
serves the jittered form while TAA is active. The velocity clip positions are built from this
header, and the TAA sub-pixel offset is applied to `gl_Position` alone through a per-draw push
constant. A short-lived revision carried the current/previous jitters in a third `vec4` member
(header at 144 B) so the vertex shader could subtract them back — that design required the
jitter to also sit in the single-buffered view UBO, which raced the GPU; see engine
`docs/caution-points.md` § "Sub-pixel projection jitter raced the single-buffered view UBO".

**Frame contract (frame-linear slots):**
1. The **Renderer** calls `Scene::beginRenderFrame()` once per rendered frame (windowed
   and window-less runs: ONE flow since 2026-09-22, the swap-chain is headless in the latter), BEFORE any `Scene::prepareRender()` — this resets the staging cursor
   and targets the current frame-in-flight buffer.
2. Every `prepareRender()` of the frame (render-to-textures first, main view last) stages one
   entry per **visible non-instanced** instance (`!useModelVertexBufferObject()`) via
   `RenderableInstance::Abstract::stageInstanceTransforms()` — a mirror of
   `Unique::pushMatricesForRendering()`'s model matrix computation — then uploads the whole
   staged range (`updateVideoMemory()`, cumulative and idempotent within the frame).
3. The instance retains its slot (`instanceTransformsSlot()`) for the draws recorded until the
   next `prepareRender()`. The same instance may hold a different slot per render target within
   one frame — REQUIRED for sprites, whose model matrix depends on the camera position.
4. Buffers grow on demand (power of two); the old buffer is retired through the
   `Vulkan::DeferredDestructor`.

**Descriptor binding (milestone 2):** the SSBO is exposed through a DEDICATED descriptor set
owned by `SceneInstanceTransforms` (one set per frame-in-flight, shared renderer descriptor
pool, layout cached under UUID `InstanceTransformsSSBO` via
`SceneInstanceTransforms::getDescriptorSetLayout()`), at the dynamic set index
`Saphir::SetType::PerSceneTransforms`. NOT inside the per-render-target view UBO set — the
SSBO is per-scene/per-frame while view sets are per-target and frame-agnostic (incompatible
lifecycles; same reasoning as the skinning SSBO's dedicated PerModel set). On buffer growth,
the current frame's set is rewritten in place (legal: the frame fence guarantees no in-flight
reference).

**Consumption (milestone 3):** the classic non-instanced scene path (non-MDI, non-cubemap,
non-advanced) READS the SSBO: push constants shrink to VP + jitter + frameIndex (76 B, or
68 B before the TAA jitter member), the model
matrix comes from `instanceMatrices[gl_InstanceIndex * 2]`, the slot travels through the
`firstInstance` draw parameter (`CommandBuffer::drawWithFirstInstance()` — instanceCount
is 1, so `gl_InstanceIndex == firstInstance`, no shaderDrawParameters feature needed).
The descriptor set is passed from `Scene::prepareRender()`'s cached
`m_preparedInstanceTransformsDS` down through `render()`. Full shader-side contract
(incl. the ⚠️ two-condition binding rule): `src/Saphir/AGENTS.md` § "InstanceTransforms
SSBO Path". Advanced/lighted, cubemap/CSM and shadow paths still push their matrices
(milestone 4).

**Status:** B1 (e080399e) + B2 (4d500626) + B3 (velocity MRT + RTGI dilation consumption) + B4 (double skinning: the skinning SSBO interleaves {current, previous} bone matrices, stride 2 — limb-level velocity)
DONE 2026-07-25 — the header {VP, previousVP} is now CONSUMED by the velocity vertex
shaders and the entries' previousModel by the same path. B1 details: — classic AND advanced
paths consume the SSBO (advanced pushes V + frameIndex = 68 B, killing the historical 132 B
min-spec violation). Cubemap/shadow/CSM paths stay on push constants (owner decision —
min-spec clean, no motion data needed). Validated: `doom-loader` (unlit classic path),
`global-illumination` A/B pixel diff vs pre-B1 baseline within the stochastic noise floor +
M4 BIT-IDENTICAL to M3 (deterministic console camera), `basic-scenery` (skybox infinity view
+ sprites + instanced + shadow-receiving).

**Previous model matrices (motion vectors B2, 2026-07-25):**
- **Unique (non-instanced)**: `previousModel` is REAL — `Abstract::m_lastModelMatrix` holds
  the matrix staged at the previous rendered frame; only the PRIMARY view staging advances
  it (`advanceHistory` parameter, gated on `RenderTargetType::View` in
  `insertIntoRenderLists()`). First staging (and post-culling reappearance) falls back to
  `previousModel == model` (zero object velocity beats a bogus one).
- **Multiple (instanced)**: opt-in `RenderableInstanceFlagBits::EnableInstanceMotionHistory`
  (MUST be set at construction — it fixes the VBO stride at
  `MeshVBOWithHistoryElementCount` = 16+9+16 floats). `updateLocalData()` archives the
  current model matrix into the previous slot before overwriting (one history step per
  logic update); ⚠️ the flag rides the whole chain: generator flag
  `IsInstanceMotionHistoryEnabled` → `VertexShader::enableInstanceMotionHistory()` →
  `VertexBufferFormatManager` (declare-or-jump `PreviousModelMatrixR0..R3`) →
  `ProgramCacheKey::isInstanceMotionHistory`. Breaking ANY link desynchronizes the pipeline
  vertex input stride from the actual VBO. No demo content uses it yet.
- **Skinned poses (double skinning, 2026-09-28)**: the previous pose is the one the PREVIOUS
  RENDERED FRAME skinned with, never the previous logic tick's. The logic thread stages the
  current pose only (`Abstract::updateSkinningMatrices()`, even slots of the stride-2 staging);
  the render thread fills the previous slots at the once-per-frame upload
  (`flushSkinningMatrices()`, `m_previousRenderedSkinningMatrices`). An instance not uploaded on
  the immediately previous frame (first frame, culled, bone count change) gets
  `previous == current`. ⚠️ Why: the logic runs at 60 Hz and nothing interpolates the pose between
  ticks, so the renderer draws the same pose on several frames (94 fps on the owner's screen, ~290
  during a `temporalCapture`); archiving per tick reported the last tick's motion on every one of
  them while the geometry stood still, and the TAA combed the whole Paladin (`animation-debug`,
  frame-to-frame change of the actor crop on no-tick frames 1.4/255 → 0.1/255 = the static floor).
  ⚠️ The instanced history above still archives per logic update — the same defect if a demo
  ever enables it.
- ⚠️ **A/B capture protocol**: the RTGI accumulation converges asymptotically after a
  camera move — A/B pixel diffs are only valid at IDENTICAL post-placement timing
  (a 0.5-1 s window difference showed up as ~3/255 RMSE of pure reconvergence residual).
- **A local transformation that changes every tick** (2026-09-28): `RenderableInstance::Abstract::publishTransformationMatrix(writeStateIndex, m)`
  stores the author's local transformation PER RENDER-STATE SLOT; `applyLocalTransformation(model, readStateIndex)`
  (staging, push path, TLAS via `SceneMetaData::rebuild(..., readStateIndex, ...)`) reads the frame's slot. It used to be
  one plain member written by the logic thread and read by the render thread — a race the day it moves. Staged here
  like any model matrix, it gets a REAL previous model matrix for free: `Scenes::Component::Beam` places its unit
  segment this way (its endpoints ARE its model matrix). `setTransformationMatrix()` still writes every slot (constants
  set at build time).
