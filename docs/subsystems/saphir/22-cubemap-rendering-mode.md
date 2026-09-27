## Cubemap Rendering Mode (Multiview)

When rendering to a cubemap (e.g., environment probes, reflection captures), the shader system operates differently:

> [!CRITICAL]
> **Rendering INTO a cubemap takes THREE coupled pieces. They are one mechanism — never touch one alone.**
>
> | # | Where | What |
> |---|---|---|
> | 1 | `ViewMatrices3DUBO::CubemapOrientation` | each face looks along ITS OWN axis, **up vectors NEGATED** (sides `-Y`, `+Y` pole `+Z`, `-Y` pole `-Z`) |
> | 2 | `ViewMatrices3DUBO::updatePerspectiveViewProperties()` | the Y-up projection flip is **UNDONE** for cube faces |
> | 3 | `GraphicsPipeline::configureRasterizationState(..., mirroredViewport = renderTarget()->isCubemap())` | the **front face is inverted** |
>
> **Why it cannot be done in coordinates alone.** The cube-face convention is LEFT-handed — a face
> wants `right × up = +look` (the `(dx,dy,dz)` tables in `CubemapResource` / `IBLBaker`) — while a
> camera is right-handed and gives `right × up = -look`. **Measured: with honest look/up vectors all
> six faces come out with exactly the OPPOSITE right vector.** Changing `up` only ROTATES a face
> (`up → -up` gives `right → -right` *and* `up → -up`, a 180° turn), it never MIRRORS it, so no
> re-parameterisation can repair the handedness. Piece 2 supplies the second mirror
> (mirror + mirror = a 180° rotation, orientation-preserving), piece 1's negated ups cancel that
> rotation, and piece 3 pays for the winding the reflection reverses.
>
> **The three symptoms, in the order they peel off** — reproduce with
> `reflexion-debug --demo-options 0,3,0` (mode 3 = `CameraOnce`). ⚠️ **It is the only usable mode**:
> modes 1/2/5 reflect a SKY, which has no left/right landmark, so a mirrored reflection is
> invisible there. Do not "verify" a cubemap orientation against a sky.
>
> | Missing piece | What you see |
> |---|---|
> | face axes wrong (pre-Aug-2026 table) | GROUND at the top of the sphere, SKY at the bottom, faces not joining |
> | 1 + 2 | palm TRUNK left, its CROWN crossed to the RIGHT — the crown lands on the `+Y` pole face whose mirror axis is X. ⚠️ A GLOBAL mirror moves the whole tree; a PER-FACE one cuts it in two. That distinction is what identifies the defect. |
> | 3 | geometry correctly PLACED but the cubemap mostly **BLACK** (culling eats it) |
>
> Correct: palm whole on the left, dragon on the right, continuous horizon, no black.
>
> ⚠️⚠️ **This mechanism feeds EVERY cubemap render target — reflection probes AND point-light shadow
> cubemaps.** Measured across the change on `light-and-shadow-debug`: 3329 differing pixels out of
> 4 665 600, i.e. shadows unaffected.
>
> **What is NOT this defect** (measured, do not re-chase): the material reflection path, the IBL
> split-sum, SSR and RTR are healthy — modes 1, 2 and 5 (both the SSR and the RTR branch) reflect
> correctly, including after a sky change with and without lighting derivation. All three sampling
> paths take the raw world-space `reflect(I, N)` with no negation, which is the correct contract.

### Matrix Sources by Render Mode

| Matrix | Standard Mode | Cubemap Mode |
|--------|---------------|--------------|
| Projection | Push constant (MVP) or UBO | UBO (shared for all faces) |
| View | Push constant (MVP) or UBO | UBO indexed by `gl_ViewIndex` |
| Model | Push constant (MVP) | Push constant (Model only) |

### Push Constant Declaration

The push constant structure changes based on render target type. See: `Generator/Abstract.cpp:declareMatrixPushConstantBlock()`

```cpp
// Standard mode (non-cubemap, non-instanced, no advanced matrices) —
// ONLY when the InstanceTransforms SSBO path is unavailable (fallback):
layout(push_constant) uniform Matrices {
    mat4 modelViewProjectionMatrix;  // Pre-combined MVP
} pcMatrices;

// Standard mode on the InstanceTransforms SSBO (the DEFAULT scene path since B1):
layout(push_constant) uniform Matrices {
    mat4 viewProjectionMatrix;  // VP only — model matrix from the SSBO
} pcMatrices;

// Cubemap mode (non-instanced)
layout(push_constant) uniform Matrices {
    mat4 modelMatrix;  // Model only - View/Projection from UBO
} pcMatrices;
```

### InstanceTransforms SSBO Path (motion vectors B1)

The classic non-instanced scene path reads its model matrix from the per-scene
`InstanceTransforms` SSBO instead of push constants:

- **SetType::PerSceneTransforms** — dedicated set (dynamic index, enabled right after
  PerView by `SceneRendering::prepareUniformSets()` via `useInstanceTransformsSet()`:
  non-instanced, non-MDI, non-cubemap, scene transforms initialized — classic AND advanced).
- **GLSL**: `readonly buffer InstanceTransforms { mat4 viewProjection; mat4
  previousViewProjection; mat4 instanceMatrices[]; } ubInstanceTransforms;` — entries
  interleave `{model, previousModel}` (stride 2). The header is reserved for the
  motion-vector pass.
- **Indexing**: `gl_InstanceIndex * 2` — the slot is encoded in the `firstInstance`
  draw parameter (`CommandBuffer::drawWithFirstInstance()`); with `instanceCount == 1`,
  `gl_InstanceIndex == firstInstance` and NO `shaderDrawParameters` feature is required
  (contrary to `gl_BaseInstance`). Min-spec safe.
- **Push blocks**: classic = VP + frameIndex, advanced = V + frameIndex (projection from
  the view UBO) — both 68 B, declared by `declareMatrixPushConstantBlock()`. The advanced
  V+M+frameIndex fallback (132 B, min-spec VIOLATION) only survives for scenes whose
  instance transforms failed to initialize.
- ⚠️ **Two-condition contract**: the descriptor SET (pipeline layout) follows
  `setIndexes.isSetEnabled(PerSceneTransforms)` — sealed at `prepareUniformSets()` time —
  while the MATRIX SOURCE follows `Program::wasInstanceTransformsEnabled()` (the vertex
  shader flag). The CPU binding in `RenderableInstance::Abstract::render()` follows
  setIndexes; the push constants and `firstInstance` follow the shader flag. NEVER mix
  the two conditions (a bound-but-unreferenced set is legal; a missing set in the sealed
  layout order corrupts every subsequent set index).
- **VertexShader preparations**: `prepareInstanceModelMatrix()` (the template), with
  branches in `prepareModelViewMatrix()`, `prepareModelViewProjectionMatrix()`,
  `synthesizeVertexPositionInWorldSpace()`, world-space normal and world TBN fallbacks,
  plus the non-instanced shadow-receiving reads in `LightGenerator.ShadowMap.cpp`.
- **Assumed limit (owner decision)**: cubemap scene, shadow 2D, CSM and shadow-cubemap
  paths STAY on push constants (64-68 B, min-spec clean — no motion data needed there).
- **Velocity clip positions** — `AbstractVertexStage::synthesizeVelocityClipPositions()` emits
  `svClipPositionCurrent` (recomputed from the MVP, deliberately **independent** of the
  `gl_Position` instruction so output ordering cannot break it) and `svClipPositionPrevious`
  (`previousViewProjection` × the odd-slot previous model, or the previous skinned pose).
  The fragment side outputs the plain NDC delta. Both endpoints are expressed in the
  **same, jitter-free** projection by construction (see the jitter contract below) — no
  subtraction is performed, and reintroducing one would be a regression.
- **⚠️ Infinity view**: renderables drawn with the translation-free view
  (`isUsingInfinityView()`, the sky background) take their CURRENT clip position from the pushed
  infinity view, so their PREVIOUS one MUST come from the header's
  `previousViewProjectionInfinity`, never from `previousViewProjection`. The generator flag
  `IsUsingInfinityView` selects it at generation time and reaches the program cache key through
  `flags()` — two instances of the same renderable, one infinity-view and one not, must never
  share a program. Mixing the two forms is a **STRUCTURAL** mismatch: it differs by the whole
  camera translation and therefore does NOT cancel on a static camera (lived: a smooth
  NDC-position-like velocity gradient over the entire sky, blamed on the translucent glass in
  front of it for two sessions before anyone visualised the buffer).
- **⚠️ Infinity view also PINS THE CLIP DEPTH** (added Aug 2026):
  `synthesizeVertexPositionInScreenSpace()` emits `gl_Position.z = gl_Position.w` right after the
  MVP multiply whenever `isInfinityViewEnabled()`. The projection maps near to 0 and far to 1
  (Vulkan range, NOT reversed), so `z = w` lands exactly on the far plane — the standard skybox
  trick (`clipPosition.xyww` in the Khronos glTF Sample Viewer's `skybox.vert`). **The geometric
  size of the sky therefore stops mattering, and no camera far distance can ever clip it.**
  Before the pin, a short far distance silently deleted the whole sky (see
  `Scenes/AGENTS.md` § "The background is drawn FIRST"). Safe by construction: the background is
  the ONLY user of the infinity view in the whole cascade, and it is drawn with the depth test AND
  the depth write disabled, so pinning its depth can neither fail a comparison nor pollute the
  depth buffer. ⚠️ It must stay OUT of the velocity path: the velocity clip positions are
  synthesized independently of `gl_Position` (`synthesizeVelocityClipPositions()`), which is
  precisely what keeps the motion vectors correct — never "simplify" that by reusing
  `gl_Position`.

### TAA Sub-Pixel Jitter — The Per-Draw Push Constant Contract (fixed 2026-07-25)

**Invariant: NO matrix ever carries the sub-pixel jitter.** It is applied to the clip position
and nowhere else, from a per-draw push constant:

```glsl
gl_Position = svModelViewProjectionMatrix * vec4(vaVertex, 1.0);
gl_Position.xy += pcMatrices.projectionJitter * gl_Position.w;   /* NDC translation */
```

Three sites MUST stay in lockstep — the layout is declared, read and written in different
files:

| role | location | rule |
|---|---|---|
| declares the `vec2` member | `Generator/Abstract.cpp::declareMatrixPushConstantBlock()` | after the pushed V/VP matrix; `FrameIndex` stays appended last, so the vec2 lands at offset 64 with no padding (76 B total) |
| emits the read | `VertexShader::isProjectionJitterPushed()` → `synthesizeVertexPositionInScreenSpace()` | mirrors the predicate above |
| writes the value | `RenderableInstance::Unique`/`Multiple` (rendering **and** shadow casting) | floats 16-17, push size 76 B |

Blocks that carry the jitter: instanced (advanced/billboard → V, classic → VP) and the
non-instanced InstanceTransforms paths (advanced → V, classic → VP). Blocks that do NOT:
MDI, cubemap/CSM (nothing is pushed for them), the MVP fallback and the 132 B advanced
fallback. Those last three bake the jitter into their CPU-computed matrix instead — legal
**only** because none of them outputs a velocity (the 132 B fallback simply rasterizes
unjittered: no room for the member, assumed limit).

> [!CAUTION]
> Adding the member to one branch and forgetting another fails in two different ways:
> a shader reading a member the generator did not declare is a **hard glslang error**
> (`'projectionJitter' : no such field in structure 'pcMatrices'` — this is how the classic
> `RenderableInstanceSimplePassVertexShader` path was caught), while a CPU push that is
> shorter than the declared block is **silent** — the shader offsets `gl_Position` by
> uninitialized memory with no validation warning. Cheap exhaustive check: enable
> `Core/Graphics/Shader/ShowSourceCode` and grep the generated GLSL for
> declaration-versus-use across every program.

> [!CAUTION]
> **Sub-pixel jitter must NEVER travel through the view UBO.** The first TAA implementation
> wrote the jitter into `ubView.projectionMatrix`, which the advanced path multiplies by the
> pushed view matrix to rebuild its MVP. That UBO is single-buffered, so the raster could read
> frame N±1's jitter while the velocity subtracted frame N's → motion vectors polluted by an
> NDC-constant offset on a perfectly static camera. Mechanism in `docs/caution-points.md` §
> "Sub-pixel projection jitter raced the single-buffered view UBO" and
> `src/Graphics/AGENTS.md` § 16 Rule 4.

### Push Constant Min-Spec (128 B) — Engine-Wide Rules

- `Vulkan::PipelineLayout::createOnHardware()` VALIDATES every push constant range:
  hard error above the device `maxPushConstantsSize`, warning above the 128-byte Vulkan
  minimum guarantee. A min-spec warning in the logs is a portability defect to fix.
- The INSTANCED advanced/billboard block pushes **V only** (+ frameIndex); the shader
  recomposes VP from the view UBO projection × V (`prepareModelViewProjectionMatrix()`
  instanced branches). The former V + VP + frameIndex block was 132 B. This applies to
  shadow casting too: the ShadowCasting vertex shader declares the view uniform block for
  EVERY instanced program (the PerView set was already enabled/bound for instancing).
- Plain (non-advanced, non-billboard) instanced still pushes VP; MDI pushes BDA + VP.

### View Matrix Access Pattern

Code that needs the view matrix must check the render mode:

```cpp
// In generator code
const auto viewMatrixSource = vertexShader.isCubemapModeEnabled() ?
    ViewUB(UniformBlock::Component::ViewMatrix, true) :    // UBO: ubView.instance[gl_ViewIndex].viewMatrix
    MatrixPC(PushConstant::Component::ViewMatrix);         // Push constant: pcMatrices.viewMatrix
```

### Files Implementing Cubemap Support

- `Generator/Abstract.cpp:declareMatrixPushConstantBlock()` - Push constant declaration
- `AbstractVertexStage.cpp:prepareModelViewMatrix()` - ModelView matrix computation
- `AbstractVertexStage.cpp:prepareModelViewProjectionMatrix()` - MVP computation
- `AbstractVertexStage.cpp:prepareSpriteModelMatrix()` - Billboard sprite support
- `LightGenerator.PerFragment.cpp` - Light direction/position in view space
- `LightGenerator.PerFragment.NormalMap.cpp` - Normal mapping light calculations
- `LightGenerator.PerVertex.cpp` - Per-vertex (Gouraud) lighting
- `LightGenerator.PBR.cpp` - PBR lighting calculations

### CPU-Side Matrix Push (Graphics Layer)

The CPU code must match the shader expectations. See: `Graphics/RenderableInstance/Unique.cpp:pushMatricesForRendering()`

```cpp
if ( passContext.isCubemap ) {
    // Push only model matrix (View/Projection in UBO)
    vkCmdPushConstants(..., MatrixBytes, modelMatrix.data());
} else if ( pushContext.useAdvancedMatrices ) {
    // Push View + Model separately
    vkCmdPushConstants(..., MatrixBytes * 2, buffer.data());
} else {
    // Push combined MVP
    vkCmdPushConstants(..., MatrixBytes, modelViewProjectionMatrix.data());
}
```

### Common Pitfall

> [!WARNING]
> When adding code that uses `MatrixPC(ViewMatrix)`, always check if cubemap mode requires using `ViewUB(ViewMatrix, true)` instead. Failure to do so causes shader compilation errors: `'viewMatrix' : no such field in structure 'pcMatrices'`
