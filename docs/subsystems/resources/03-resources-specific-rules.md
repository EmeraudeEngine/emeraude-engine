## Resources-Specific Rules

### MANDATORY Fail-Safe Philosophy
- **NEVER** return nullptr from Containers
- **ALWAYS** provide a valid resource (real or neutral)
- **NEVER** require error checking on client side
- Errors are logged but never break the application

### Neutral Resource Pattern
- **MANDATORY**: Implement `load()` (no parameters) for neutral/default resources
- Neutral resource must ALWAYS succeed (no I/O)
- Be immediately usable and visually identifiable
- No external dependencies
- ServiceProvider is available via `this->serviceProvider()` if needed

### Thread Safety (CRITICAL)

**Atomic status:**
```cpp
std::atomic<Status> m_status{Status::Unloaded};  // Lock-free queries
```

**Mutex for lists:**
```cpp
std::mutex m_dependenciesAccess;  // Protects m_parentsToNotify and m_dependenciesToWaitFor
```

**Two-phase pattern (avoids deadlocks):**
```cpp
void checkDependencies() noexcept {
    Action action = Action::None;
    {
        std::lock_guard lock{m_dependenciesAccess};
        // Phase 1: Determine action under lock
        if (allDependenciesLoaded()) action = Action::CallOnDependenciesLoaded;
    }
    // Phase 2: Execute OUTSIDE lock (virtual calls + notifications)
    if (action == Action::CallOnDependenciesLoaded) {
        this->onDependenciesLoaded();  // Virtual call OUTSIDE lock!
    }
}
```

### Cycle Detection (NEW v0.8.35)

**Automatic in addDependency():**
```cpp
if (this->wouldCreateCycle(dependency)) [[unlikely]] {
    m_status = Status::Failed;
    return false;
}
```

**Recursive DFS algorithm:**
```cpp
bool wouldCreateCycle(const shared_ptr<ResourceTrait>& dep) const noexcept {
    if (dep.get() == this) return true;  // Self-reference
    for (const auto& sub : dep->m_dependenciesToWaitFor) {
        if (sub.get() == this || this->wouldCreateCycle(sub)) return true;
    }
    return false;
}
```

### Dependency Management
- Use `addDependency()` to declare dependencies
- `onDependenciesLoaded()` for finalization (GPU upload, etc.)
- Automatic parent-child event propagation
- Reference counting with `std::shared_ptr`

### CPU Copies: Metadata Outlives the Data (2026-10-04)
The CPU copy of an uploaded resource (a geometry's shape, an image's pixels) is meant to be released once the GPU
holds it (`docs/todo/cpu-copies-retained-after-upload.md`). What a reader needs AFTER the upload therefore lives on
the resource, never only in the data:
- **Geometries** (`IndexedVertexResource`, `VertexResource`): `boundingBox()` / `boundingSphere()` answer a copy
  taken at every load path and again at the upload (`cacheBoundingVolumes()`), not the shape's.
- **Images** (`ImageResource`) and **cubemaps** (`CubemapResource`): `extractMetadata()` computes once and keeps
  what the readers derive from the pixels — dimensions / cube size, grey-scale, average colour, binary alpha
  (`isBinaryAlphaMask()`, which `Texture2D` now delegates to), the hemisphere illuminance factor. Until it runs, the
  accessors compute from the pixels as before (owner decision: "extracted at release", zero cost for a resource
  never released). The release calls it FIRST.
- **Movies share their frames** (2026-10-04): a `MovieResource::Frame` shows a store `ImageResource` (no copy) or
  owns generated pixels; read it through `Frame::pixmap()`. A movie therefore keeps its images alive, and a release
  of an image's pixels (phase 2) covers the movie frames showing it.
- A new per-frame or late reader of a CPU copy adds its value to that metadata — or declares the resource
  "CPU too" — instead of reading the data.
- Verified 2026-10-04 (citadel, a local probe): with every image's pixels, every cubemap's faces and every
  geometry's whole shape freed after extraction, 393 + 2 + 339 resources answered identically, and the frame was
  unchanged except the animated actors (0.2 % of the pixels).
