## 19. Raw Geometry System

Two geometry resource classes for loading raw vertex/index data directly to GPU without the `Shape<float>` intermediate.

### Classes

| Class | File | Description |
|---|---|---|
| `RawIndexedVertexResource` | `Graphics/Geometry/RawIndexedVertexResource.hpp` | VBO + IBO from raw data |
| `RawVertexResource` | `Graphics/Geometry/RawVertexResource.hpp` | VBO only from raw data |
| `RawGeometryOptions` | `Graphics/Geometry/RawGeometryOptions.hpp` | Shared options struct (topology, bounding box) |

### Design Principles

- **Zero staging**: No CPU-side copies. Data is uploaded to GPU directly inside `load()`.
- **Zero move**: `const` references/`std::span` — caller retains ownership of its data.
- **`std::span` API**: Accepts `std::vector`, `std::array`, C arrays, or raw `{pointer, size}` pairs.
- **`createOnHardware()` is a no-op**: Upload happens in `load()` via `serviceProvider().graphicsRenderer().transferManager()`. When `onDependenciesLoaded()` fires, `isCreated()` is already `true` — skips upload, builds BLAS.

### Two Load Modes

**1. Pre-interleaved** (caller already packed the vertex buffer):
```cpp
// vertexCount deduced from span.size() / vertexElementCount
res.load(vertexData, vertexElementCount, indices, options);
```

**2. Separate attributes** (engine interleaves for optimal GPU layout):
```cpp
// Strides: positions=3, normals=3, UV=2, colors=4 (RGBA)
// Geometry flags auto-set from non-empty spans
res.load(positions, indices, normals, uvs, colors, options);
```

### Resource Container Registration

Containers registered in `Resources/Manager.cpp` alongside existing geometry types:
- `RawVertexGeometries` — `Container<RawVertexResource>`
- `RawIndexedVertexGeometries` — `Container<RawIndexedVertexResource>`

### Key Constraint

**Must use `getOrCreateResourceSync()`** (not `getOrCreateResource()`) when the caller's data is on the stack. The async version dispatches to a thread pool — stack references would dangle.
