## CRITICAL Attention Points

| Point | Importance | Description |
|-------|------------|-------------|
| **Thread safety** | CRITICAL | Atomic status + mutex on lists |
| **Deadlock prevention** | CRITICAL | Virtual calls OUTSIDE lock |
| **`onDependenciesLoaded()` runs ONCE** | CRITICAL | `checkDependencies()` CLAIMS the finalization under `m_dependenciesAccess` (`m_dependenciesFinalizing`) before calling it outside the lock. The last dependency completing (its thread) and the resource's own `setLoadSuccess()` (another thread) both find every dependency loaded: until 2026-10-07 both ran it — two `Texture2D::createFromPixelData()` on one texture, the second reassigning `m_image` and so destroying the image the first was creating (`vkBindImageMemory` on `VK_NULL_HANDLE`). Found by trapping a cross-thread `Image::destroyFromHardware()` during a creation under gdb; WAD-refusal repro 3 hits / 27 runs before, 0 / 18 after |
| **Async capture safety** | CRITICAL | No `this` capture in getOrCreateResource lambdas |
| **Cycle detection** | HIGH | `addDependency()` takes the dependency-GRAPH lock first, then an iterative O(V+E) walk (visited set) copies each node's list under that node's lock alone, then the pair lock (triad 2026-09-30: it used to read sub-dependency lists without their lock, and revisit shared dependencies exponentially) |
| **Data paths are confined** | CRITICAL | A LocalData `data` (and any filename from data) is joined through `Base::IO::confinedPath()` in `FileSystem::getFilepathFromDataDirectories()`: absolute or `..`-escaping = refused (triad 2026-09-30; `path::append()` replaced the data directory with an absolute path). 10 019 real index entries all stay inside their store |
| **Directory scans never throw** | CRITICAL | `Base::IO::forEachDirectoryEntry()` only — a range-for over `directory_iterator` terminates on the first filesystem error under `-fno-exceptions`. The dynamic scan finds the same 14 396 resources as before |
| **Memory management** | HIGH | `shared_ptr` for reference counting |
| **Status tracking** | MEDIUM | State machine: Unloaded → Loading → Loaded/Failed |
| **Cache efficiency** | MEDIUM | Key by resource name for reuse |
