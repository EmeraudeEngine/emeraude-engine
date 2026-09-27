## 13. Geometry ResourceGenerator: Gem Methods

`ResourceGenerator` provides GPU-ready `IndexedVertexResource` wrappers for all 12 gem cuts. Each method follows the same pattern:

```cpp
std::shared_ptr< IndexedVertexResource > diamondCutGem (
    float radius, float depth, float tableRatio, uint32_t segments,
    std::string resourceName = {}
) const noexcept;
```

### Pattern
1. Auto-generates resource name from class + parameters if empty
2. Calls `ShapeGenerator::generate*CutGem< float, uint32_t >(...)` with `ShapeBuilderOptions`
3. Applies transform matrix if not identity
4. Loads into `IndexedVertexResource` via `getOrCreateResource()`

### Available Methods
`diamondCutGem()`, `emeraldCutGem()`, `asscherCutGem()`, `baguetteCutGem()`, `princessCutGem()`, `trillionCutGem()`, `ovalCutGem()`, `cushionCutGem()`, `marquiseCutGem()`, `pearCutGem()`, `heartCutGem()`, `roseCutGem()`

See: `Graphics/Geometry/ResourceGenerator.hpp`, `Graphics/Geometry/ResourceGenerator.cpp`
