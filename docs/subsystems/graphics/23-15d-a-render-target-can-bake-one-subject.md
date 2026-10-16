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

⚠️ **The keys follow the instance's life** (Ave Robustus II P4, owner decisions 2026-10-16). Until then the bake
subject and the exclusions were bare `RenderableInstance` addresses that nothing cleared when the instance died (an
instance built later at the same address could be excluded, or baked, in its place). `RenderableInstance::Abstract`
belongs to `Graphics`, so the render target takes the instance itself:
- `excludeFromRendering(std::shared_ptr< const RenderableInstance::Abstract >)` keeps a weak reference and its
  address; the render thread matches the address of a LIVE instance only (no lock, no allocation per instance), and the
  expired entries are purged at the next exclusion. `clearRenderingExclusions()` is gone.
- ⚠️ An exclusion is accepted only **before the target renders for the first time** (the render thread reads the list
  without a lock); a later call is refused and traced. `m_hasBeenRendered` is atomic for that check.
- `ImposterBake::enqueue(std::shared_ptr< const RenderableInstance::Abstract >, atlas)`: the job keeps a weak
  reference; a subject that dies before its bake drops its job (traced) instead of keeping the queue waiting for a
  render that never comes. `setBakeSubject()` still takes the address of the live front subject.
- Verified (Linux): `offscreen-rendering`, `reflexion-debug` (no refusal), `citadel` (12 atlases baked), 0 VUID.

The rest of the bake mechanics already existed and were verified rather than assumed: a target
with `setAutomaticRenderingState(false)` renders only while `setRenderOutOfDate()` flags it (the
"bake once" contract, `RenderTarget/Abstract.hpp`), `Scenes::Toolkit::generateTexture2DRenderer()`
is the working camera-to-target rig, and `CommandBuffer::blitImage()`/`copyImage()` assemble
several views into one atlas image (`GrabPass`, `PostProcessor` already use them).

Consumer: the octahedral imposters (§ 15e). The parametrisation the
baker and the shader must share is `EmEn::Base::Math::OctahedralMapping` in emeraude-base.
