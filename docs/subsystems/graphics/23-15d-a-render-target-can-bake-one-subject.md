## 15d. A render target can BAKE one subject (Sept 2026)

⚠⚠ **The offscreen pass renders the SCENE, not an object.**
`Renderer::renderRenderToTextures()` opens the render pass then calls `Scene::renderOpaque()`,
`renderTranslucent()` and `renderTranslucentGB()`. That is what an environment probe wants and
exactly what a **bake** does not: an imposter atlas, an asset thumbnail or an icon must show ONE
subject over nothing. Baked without a filter, a tree's atlas carries the ground, the sky and the
neighbours, over an **opaque** background.

Two knobs on `RenderTarget::Abstract` answer it:

- `setBakeSubject(const void * renderableInstance)` — the target renders that instance alone.
  `nullptr` (the default) means the whole scene, so nothing changes for every existing target.
- `setClearColorOverride(VkClearColorValue)` / `clearClearColorOverride()` — the target's own
  colour clear. Without it the pass clears with the **renderer's** colour, which is opaque, and a
  card composited over it shows a rectangle of background.

⚠️ **The filter lives at ONE site**, `Scene::checkRenderableInstanceForRendering()`, which runs
while `populateRenderLists()` fills the lists — not at the draw sites. The MDI batches
(`MDIBatchBuilder::buildBatches()`) and the lighted selections (`renderLightedSelection()`) are
built FROM those lists, so they inherit the restriction for free. Filtering at draw time instead
would have meant six sites, and the MDI path would have batched the excluded geometry anyway.

⚠️ Same lifetime contract as `excludeFromRendering()`, and for the same reason: the key is the
opaque `RenderableInstance` **address**, so `Graphics` keeps no dependency on `Scenes`. Nothing
clears it when the instance dies — **clear it when the bake is done**.

The rest of the bake mechanics already existed and were verified rather than assumed: a target
with `setAutomaticRenderingState(false)` renders only while `setRenderOutOfDate()` flags it (the
"bake once" contract, `RenderTarget/Abstract.hpp`), `Scenes::Toolkit::generateTexture2DRenderer()`
is the working camera-to-target rig, and `CommandBuffer::blitImage()`/`copyImage()` assemble
several views into one atlas image (`GrabPass`, `PostProcessor` already use them).

Consumer in progress: `docs/todo/vegetation-octahedral-imposter-atlas.md`. The parametrisation the
baker and the shader must share is `EmEn::Base::Math::OctahedralMapping` in emeraude-base.
