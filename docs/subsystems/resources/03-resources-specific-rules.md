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
holds it (history and measurements: [`11-cpu-copy-release.md`](11-cpu-copy-release.md)). What a reader needs AFTER the upload therefore lives on
the resource, never only in the data:
- **Geometries** (`IndexedVertexResource`, `VertexResource`): `boundingBox()` / `boundingSphere()` answer a copy
  taken at every load path and again at the upload (`cacheBoundingVolumes()`), not the shape's.
- **Images** (`ImageResource`) and **cubemaps** (`CubemapResource`): `extractMetadata()` computes once and keeps
  what the readers derive from the pixels — dimensions / cube size, grey-scale, average colour, binary alpha
  (`isBinaryAlphaMask()`, which `Texture2D` now delegates to), the hemisphere illuminance factor. Until it runs, the
  accessors compute from the pixels as before (owner decision: "extracted at release", zero cost for a resource
  never released). The release calls it FIRST.
- **Movies share their frames** (2026-10-04): a `MovieResource::Frame` shows a store `ImageResource` (no copy) or
  owns generated pixels, read through `Frame::pixmap()`; a `CubemapMovieResource::Frame` likewise shows a store
  `CubemapResource` or owns its faces, read through `Frame::faces()`. A movie therefore keeps its images alive, and a release
  of an image's pixels (phase 2) covers the movie frames showing it.
- **The release (phase 2, 2026-10-04)**: `IndexedVertexResource`, `VertexResource`, `ImageResource` and
  `CubemapResource` are "GPU only" by type (`releasesLocalData()`) — **an image or a cubemap only when its pixels can
  come back**: a store source (`hasLocalDataSource()`), or, for an image a SCENE LOADER built from a model, its
  **`EncodedSource`** (2026-10-04): the whole external file (glTF `uri`, FBX / USD external textures), a BYTE RANGE
  of a file (a glTF bufferView in a `.glb`'s BIN chunk or an external `.bin`, located from the model's own JSON; a
  USDZ entry, from the archive's asset table), or an FBX's EMBEDDED content re-read by `FBXLoader::readEmbeddedTexture()`
  (ufbx gives no file offset: the FBX is re-parsed without geometry or animation, its size checked). The reload
  decodes exactly as the loaders did (forced RGBA) through `reloadLocalDataFromOwnSource()`, tried after the store
  entry and before the GPU readback. Stay resident: generated images and cubemaps, base64 `data:` URIs, a
  meshopt-compressed bufferView, the KTX2 fallback of a GPU without BC — their texture is BC7 on every BC-capable
  GPU, a lossy copy that cannot give the pixels back; code that reads a copy at any time declares it
  "CPU too" with `retainLocalData()` (`CursorAtlas` for its images). A copy becomes releasable after the upload that
  consumed it (`markLocalDataReleasable()`: a geometry's own upload; a texture's upload for an image / cubemap — an
  image no texture ever read stays resident). About once a second `Core::logicsTask()` SCHEDULES a release pass on a
  pool worker (one at a time; the logic loop never runs it: a pass measured up to ~165 ms on terrain — metadata scans
  of large images and frees); the worker releases the copies releasable for `Core/Resources/LocalDataReleaseDelay`
  seconds (default 5) that no lease holds — each container snapshots its resources under its lock and releases
  OUTSIDE it — `onReleaseLocalData()` extracts the metadata then frees. When a release burst ends (the first pass that
  frees nothing after one that did), the worker gives the memory back on Linux / glibc (`malloc_trim(0)`: 1-60 ms,
  holds the arena locks; without it citadel's RSS barely moves, 5143 MiB untrimmed vs ~2.5 GiB). **ON by default** since 2026-10-04
  (`Core/Resources/ReleaseLocalData`; ⚠️ an existing `settings.json` keeps the `false` its first run wrote). Console:
  `Core.ResourcesManagerService.releaseLocalData()` releases now, whatever the setting, without the grace delay.
- **Every reader of a releasable copy takes a lease** (RAII, copyable; an asynchronous job captures it) and tests it.
  Three ways (phase 3, 2026-10-04): `acquireLocalData()` RELOADS a released copy first, blocking the caller — the
  late readers are all load-time ones (loader / pool threads, or a synchronous load that did its I/O there already);
  refused on the render thread (`forbidBlockingLocalDataReload()`, called by `Core::renderingTask()`).
  `requestLocalData()` never waits: "not resident" + the reload scheduled on the pool, for a per-frame reader that
  asks again later. `leaseLocalData()` never reloads. The reload re-reads the STORE ENTRY the container recorded
  before the load (`setLocalDataSource()`) with the type's own reading code (`reloadLocalDataFromSource()`:
  geometries and images from their file, `readLocalData()` shared with `load()`; a cubemap through a temporary
  `CubemapResource` of the SAME name — a packed / equirectangular definition finds its image by the name), else a
  GPU readback (`reloadLocalDataFromGPU()`); a reloaded copy is releasable again after the grace delay. A procedural
  resource (`load(shape)`, generated pixels) has no store source. **Geometry readback** (phase 3b): the vertex and
  index buffers carry `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`; `TransferManager::downloadBuffer()` copies one into a
  temporary host-readable buffer on the graphics queue and waits (load-time readers only); the shape is rebuilt by
  base `Shape::readIndexedVertexBuffer()` / `readVertexBuffer()` with the upload's own formats
  (`skeletalAnimationFormat()` is shared by both) and the sub-geometries as groups — what the GPU draws, not the
  original shape (base `vertexfactory/07`). Leases today: the texture uploads
  (Texture1D / 2D / Cubemap, the animated textures through `MovieResource::leaseFrameImages()` /
  `CubemapMovieResource::leaseFrameCubemaps()`), the automatic LOD jobs, the ground / terrain displacement, the
  cursor, projet-alpha's terrain heightmap. ⚠️ Reading `localData()` / `data()` / `faces()` of a releasable
  resource WITHOUT a lease races the release.
- A new per-frame or late reader of a CPU copy adds its value to that metadata — or declares the resource
  "CPU too" — instead of reading the data.
- Verified 2026-10-04 (citadel, a local probe): with every image's pixels, every cubemap's faces and every
  geometry's whole shape freed after extraction, 393 + 2 + 339 resources answered identically, and the frame was
  unchanged except the animated actors (0.2 % of the pixels).
