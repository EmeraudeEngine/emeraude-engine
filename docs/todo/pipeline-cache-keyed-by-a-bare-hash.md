---
id: pipeline-cache-keyed-by-a-bare-hash
title: The renderer's graphics pipeline cache is keyed by a bare size_t hash, with no equality behind it
status: open
priority: high
scope: Graphics/Renderer.cpp (finalizeGraphicsPipeline, m_graphicsPipelines), Vulkan/GraphicsPipeline.cpp (getHash)
opened: 2026-10-31
tags: [vulkan, cache, defect, latent]
---

# The renderer's graphics pipeline cache is keyed by a bare size_t hash, with no equality behind it

## Why

`Renderer::finalizeGraphicsPipeline()` looks a pipeline up in `m_graphicsPipelines` by `GraphicsPipeline::getHash()`
(a `hashCombine` of the render pass, the layout, the shader modules, the states) mixed with the layout handle — a bare
`size_t`, no equality. Two pipelines whose hashes meet share one entry: the second request gets the first pipeline, built
from other shaders or other states. It is the same class of defect as the program cache's, which served a spot-shadow
program for the sun's cascaded pass in 1 run in 6 to 12 (fixed 2026-10-31 by `Saphir::Generator::ProgramKey`,
caution-points § "A cache keyed by a bare hash serves one program for another"). No pipeline collision has been
observed yet; the handles in the hash (addresses) make one run-dependent, like the program one.

## What remains

- [ ] Key the cache exactly: a structure holding the create-info inputs that `getHash()` folds in, compared with
  `operator==` (the `ProgramKey` model: the bytes of every input), the hash only placing it.
- [ ] Prove: the whole cascade builds, the pipeline reuse count unchanged on a large scene (sponza, city), N runs 0 VUID.

## References

- `docs/pipeline-caching-system.md` (the caution on the program cache), `docs/caution-points.md`.
