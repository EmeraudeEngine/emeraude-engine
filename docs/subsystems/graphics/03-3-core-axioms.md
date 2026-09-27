## 3. Core Axioms

### Architecture
1.  **Declarative**: You define WHAT (Material/Geometry), engine handles HOW (Vulkan pipeline).
2.  **Instancing**: `Renderable` is shared. `RenderableInstance` is the unique usage.
3.  **Visuals**: Scene nodes use `Visual` components to attach to Graphics.

### Constraints
1.  **Thread Safety**: `TransferManager` handles CPU->GPU. Main thread for Logic.
2.  **Y-UP**: Strictly Y-up coordinate system (`+X` right, `+Y` up, `-Z` forward).
3.  **Fail-Safe**: Resources must never be null. Use neutral fallbacks.
4.  **G-Buffer MRT (fixed order)**: scene target color attachments are
    `[0]=color, [1]=normals, [2]=materialProperties, [3]=albedo, [4]=velocity` (+ depth
    last), allocated ON DEMAND from the enabled post-process effects' `requires*()` flags —
    each attachment forces every one before it, and the shader generator detects the layout
    **by color attachment count**. Velocity is RG16F (NDC-delta motion vectors), written by
    the ambient/simple passes only (light passes have a zeroed write mask), consumed with a
    3x3 depth-nearest dilation (RTGI temporal; TAA/motion blur later). Full contract + pitfalls (clear-value indices, blend-state counts,
    GrabPass copies): `docs/caution-points.md` § "SSGI Indirect Light Ignored Receiver
    Albedo — New Albedo G-Buffer Attachment".
5.  **Instance transforms (motion vectors B1)**: every visible NON-instanced
    `RenderableInstance` stages its `{model, previousModel}` matrices into the scene's
    `InstanceTransforms` SSBO during `Scene::prepareRender()`
    (`Abstract::stageInstanceTransforms()`, slot retained via `instanceTransformsSlot()`).
    The classic scene path CONSUMES it: push = VP + frameIndex, model matrix from the SSBO,
    slot in the `firstInstance` draw parameter (`CommandBuffer::drawWithFirstInstance()`).
    Advanced/cubemap/CSM/shadow paths still push their matrices (B1 milestone 4 pending).
    Contract details: `src/Scenes/AGENTS.md` § "Instance Transforms (SceneInstanceTransforms)"
    and `src/Saphir/AGENTS.md` § "InstanceTransforms SSBO Path".
5b. **The sky is depth `1.0` EXACTLY** (the clear value; the background writes no depth). An effect
    separating sky from geometry tests `depth >= 1.0` / `depth < 1.0`, never a "close to 1"
    threshold: the depth is conventional, so `1 - depth ≈ near / z`, and `0.9999` is a camera
    distance of ~890 m with the default near plane. Eight sites held one until 2026-09-25 and the
    `terrain` clouds were drawn over every mountain in front of them: `docs/caution-points.md`
    § "a sky test at `depth >= 0.9999` is a DISTANCE".
5c. **Unproject CAMERA-RELATIVE**: invert `projection * viewMatrix(readStateIndex, true, 0)` (the
    infinity view: rotation, no translation) and add the camera position afterwards. The float
    inverse of the FULL view-projection distorts the screen by ±1-13 px, differently at each pose —
    the `terrain` clouds slid against the relief (fixed 2026-09-25). RTR, RTGI, RTAO,
    RTContactShadows and the GIDenoiser used it too, for SURFACE positions (error up to 4.4 m),
    and were fixed the same day. Their members are `invRelativeViewProj` /
    `inverseRelativeProjViewMatrix`, and the shader computes `worldPos = camera + relativePos`.
    `docs/caution-points.md` § "The float inverse of the full view-projection".
6.  **`Renderer.hpp` include diet (no regrowth)**: `Graphics/Renderer.hpp` is included by ~77 TUs
    directly and propagates via `Core.hpp` and `Overlay/UIScreen.hpp`, so one `#include` added
    there is paid by most of the engine. `SwapChain`, `Vulkan::Instance`, `Window`,
    `Resources::Manager`, `GrabPass`, `MDI::BatchBuilder`, `SceneRenderTarget` and
    `TextureResource::TextureCubemap` are **forward-declared on purpose** (legal because the
    destructor is out-of-line — the "exported pimpl" pattern). `setSwapChainDegraded()` /
    `isSwapChainDegraded()` are out-of-line for the same reason: do not re-inline them.
    When a consumer TU stops compiling, **add the include to the consumer**, never back into
    `Renderer.hpp`. Full contract: `docs/windows-export-api.md` § "Exported pimpl".
