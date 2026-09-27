## Critical: LayoutManager Thread Safety

> [!CRITICAL]
> **`LayoutManager` is accessed concurrently by the resource loading thread pool.**
>
> Materials (Basic, Standard) share descriptor set layouts via `LayoutManager`.
> Multiple materials with the same identifier (e.g., `"MaterialBasicResourceSimple"`) can
> be loaded in parallel by `Container::getOrCreateResource()` which dispatches to a thread pool.
>
> **Thread-safety mechanism:**
> - `LayoutManager` protects all map access with `m_access` mutex
> - `createDescriptorSetLayout()` tolerates duplicate UUIDs (returns `true` silently)
> - Material `createDescriptorSetLayout()` re-fetches the layout from the manager after
>   creation to get the canonical instance (another thread may have won the race)
>
> **Bug pattern (fixed Mar 2026):**
> ```cpp
> // BROKEN - TOCTOU race: two threads see nullptr, both try to register
> m_layout = layoutManager.getDescriptorSetLayout(id);  // nullptr
> if (!m_layout) {
>     m_layout = prepare + declare + create;  // second thread fails!
> }
>
> // CORRECT - create with local, re-fetch canonical instance
> auto newLayout = layoutManager.prepareNewDescriptorSetLayout(id);
> // ... declare bindings ...
> layoutManager.createDescriptorSetLayout(newLayout);  // tolerates duplicates
> m_layout = layoutManager.getDescriptorSetLayout(id); // get canonical
> ```
>
> **Code references:**
> - `LayoutManager.hpp:m_access` - Mutex protecting both maps
> - `LayoutManager.cpp:createDescriptorSetLayout()` - Duplicate-tolerant registration
> - `Material/BasicResource.cpp:createDescriptorSetLayout()` - Re-fetch pattern
> - `Material/StandardResource.cpp:createDescriptorSetLayout()` - Same pattern (the merged PBR lit material)
