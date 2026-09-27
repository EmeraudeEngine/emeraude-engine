## 15. Instance-Local Program Cache (RenderableInstance)

> [!IMPORTANT]
> **`RenderableInstance::Abstract` has a local `ResolvedProgram` cache (`StaticVector<16>`)
> that avoids per-draw hashtable lookups into `Renderable::Abstract::m_programs`.**

### Architecture

The `Renderable::Abstract` stores compiled shader programs in a `std::unordered_map` keyed
by `ProgramCacheKey` (render pass handle + material hash). Looking up this map every draw call
was measured at 13.32% of CPU time (perf profiling).

**Solution:** Each `RenderableInstance::Abstract` caches resolved programs locally:

```cpp
struct ResolvedProgram {
    ProgramCacheKey key;
    GraphicsPipeline * pipeline;
    PipelineLayout * layout;
};
StaticVector< ResolvedProgram, 16 > m_resolvedPrograms;
```

`resolveProgram(renderPassHandle, layerIndex)` checks the local cache first (linear scan of
up to 16 entries — cache-friendly). On miss, it falls back to the `Renderable`'s map (protected
by `std::shared_mutex` for concurrent reads) and caches the result locally.

**Cache invalidation:** `m_resolvedPrograms` is cleared on swap-chain recreation (render pass
handle changes) via `invalidateProgramCache()`.

**Measured impact:** Cache lookup -83% (from 13.32% to 2.20% of CPU time).

**Thread safety:** `Renderable::Abstract::m_programs` uses `std::shared_mutex` — shared lock
for reads (render thread), exclusive lock for writes (resource loading thread).

**Code references:**
- `RenderableInstance/Abstract.hpp` — `ResolvedProgram`, `m_resolvedPrograms`, `resolveProgram()`
- `RenderableInstance/Abstract.cpp` — `resolveProgram()` implementation, `invalidateProgramCache()`
- `Renderable/Abstract.hpp` — `m_programs`, `m_programsMutex` (`std::shared_mutex`)
- `Renderable/Abstract.cpp` — `findProgram()` (shared_lock read), `cacheProgram()` (unique_lock write)
