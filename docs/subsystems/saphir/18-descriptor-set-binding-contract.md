## Descriptor set binding contract (Aug 2026)

**The sealed pipeline layout is the ONLY authority on set indexes.** `prepareUniformSets()`
enables set types in a fixed order (PerView, PerSceneTransforms, PerLight, PerModel,
PerModelLayer, PerBindless); `Generator::Abstract::createDataLayout()` builds the
`VkPipelineLayout` in that same order and `SetIndexes::set(SetType)` returns each set's
final index. Every CPU binding site MUST read that index.

Rules for `RenderableInstance::Abstract` (render tracked, render simple, castShadows):

- Bind **iff** `program->setIndexes().isSetEnabled(SetType::X)`, at index
  `program->setIndexes().set(SetType::X)`. NEVER a running `setOffset++` counter: one
  skipped set shifts every following set one slot down, and Vulkan then rejects the whole
  binding (`VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358`, then
  `VUID-vkCmdDrawIndexed-None-08600` for the sets the shader statically uses).
- A set declared by the layout but unavailable at recording time is a **contract
  violation**, not a case to skip: the draw call is dropped and
  `traceMissingDescriptorSet()` reports it once per instance. Fix the generation/binding
  pair, never the binding alone.
- Every set has TWO conditions — one at generation, one at binding — and they must be
  equivalent by construction. Known pairs:

| Set | Generation condition | Binding condition |
|-----|----------------------|-------------------|
| `PerView` | `SceneRendering`: always · `ShadowCasting`: instancing, cubemap or CSM | render target view UBO (always exists) |
| `PerSceneTransforms` | `useInstanceTransformsSet()` | `sceneTransformsDS != nullptr` |
| `PerLight` | lighting enabled + a light render pass type | `lightEmitter != nullptr && isCreated()` |
| `PerModel` | `SkeletalDataTrait::hasSkeletalData()` (RENDERABLE) | `hasSkinningResources()` (INSTANCE) |
| `PerModelLayer` | `materialEnabled()` / alpha-tested shadows | material `!= nullptr` |
| `PerBindless` | `bindlessTexturesEnabled()` | manager + descriptor set available |

⚠️ **The `PerModel` row is the one that bit us** (Aug 2026, `reflexion-debug` options 3 and 4):
the layout condition is a property of the RENDERABLE, the binding condition a property of the
INSTANCE, and the instance's skinning descriptor sets were created lazily from the LOGIC
thread on the first `processLogics()`. A probe cubemap renders at scene build time, before
that first logic tick: the sealed layout declared `PerModel`, the binding skipped it, the
material landed in the skinning slot and the bindless array in the material slot. They are
now created by `RenderableInstance::Abstract::prepareSkinningResources()` at the top of
`getReadyForRender()` **and** `getReadyForShadowCasting()`, and an instance missing them is
not "ready" even when the renderable-level program cache holds its program. See
[`docs/renderable-instance-system.md`](../../renderable-instance-system.md).
