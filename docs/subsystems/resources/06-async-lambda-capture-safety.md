## Async Lambda Capture Safety (CRITICAL)

> [!CRITICAL]
> Lambdas passed to `getOrCreateResource()` execute on the **thread pool**, potentially after
> the calling object is destroyed. All captured data must be **self-contained**.

### Rules

1. **NEVER capture `this`** or local references to stack-allocated objects
2. **Pre-resolve** all `shared_ptr` dependencies before the lambda
3. **Copy scalars** (float, int, Color) by value
4. **Move-capture** containers of `shared_ptr` to avoid atomic refcount overhead

### Pattern: Pre-Resolve + Value Capture

```cpp
// Pre-resolve outside the lambda (on the calling thread)
auto texture = (texIdx < m_textures.size()) ? m_textures[texIdx] : nullptr;
float roughness = pbr.roughnessFactor;
Color< float > color{pbr.baseColorFactor[0], pbr.baseColorFactor[1], ...};

// Lambda captures only values and shared_ptrs — fully self-contained
auto material = container->getOrCreateResource(name, [
    texture = std::move(texture), roughness, color
] (auto & res) {
    if ( texture != nullptr ) res.setAlbedoComponent(texture);
    else res.setAlbedoComponent(color);
    res.setRoughnessComponent(roughness);
    return res.setManualLoadSuccess(true);
});
```

### Why This Matters

`getOrCreateResource()` (Container.hpp:1208) enqueues the lambda to the thread pool and returns immediately. If the caller is stack-allocated (e.g., `GLTFLoader loader{...}`), `this` becomes dangling before the lambda executes → **use-after-free crash**.

**Code references:**
- `Container.hpp:getOrCreateResource()` — Async path (thread pool)
- `Container.hpp:getOrCreateResourceSync()` — Sync path (blocks calling thread)
- `Scenes/GLTFLoader.cpp` — Reference implementation of safe async captures
