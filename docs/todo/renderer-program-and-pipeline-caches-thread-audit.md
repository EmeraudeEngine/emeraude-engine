---
id: renderer-program-and-pipeline-caches-thread-audit
title: Audit which threads reach the renderer's program and pipeline caches — they carry no lock
status: open
priority: unranked
scope: Graphics/Renderer (m_programs, m_graphicsPipelines), Saphir/Generator/Abstract
opened: 2026-11-14
tags: [concurrency, investigation, tsan]
---

# Audit which threads reach the renderer's program and pipeline caches — they carry no lock

## Why

`Renderer::m_samplers` was read and grown by several threads without a lock (ThreadSanitizer, 2026-11-14; guarded
since by `m_samplersAccess`). The same paragraph of the class documentation covered two sibling maps with the same
"written during initialization only" sentence: `m_programs` (`findCachedProgram()`, `cacheProgram()`) and
`m_graphicsPipelines` (`finalizeGraphicsPipeline()`). Both are reached through `Saphir::Generator::Abstract`
(`Abstract.cpp:302-311`, `:375`, `:484`), which the rendering thread runs at an instance's first use
(`getReadyForRender()`), and both grow at run time. ThreadSanitizer reported nothing on them over two `default` runs,
which proves nothing: a generator run from another thread (an effect created from the console on the main thread, a
bake, a warm-up on the pool) would race exactly as the samplers did.

## What remains

- List every caller of a generator (`generateShaderProgram()` and what reaches it) with its thread; a TSan run that
  creates an effect from the console while a scene renders is the measurement.
- If more than the rendering thread comes there: the same guard as the samplers, or the contract written and asserted
  (the render thread only). If the parallel pipeline warm-up of `logic-stalls-on-runtime-pipeline-compile` is built,
  the guard is part of it.

## References

- `Renderer.hpp` § Thread Safety (the class documentation), `docs/caution-points.md` § the sampler cache.
