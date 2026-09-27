## System Memory Query (`SystemInfo::getSystemMemoryInformation()`)

**Files**: `SystemInfo.hpp` (static entry point), `SystemInfo.{linux,windows,mac}.cpp`

Live, whole-machine memory probe — every call re-queries the OS (no caching, no service
state; usable from any process including sandboxed CEF renderers). Returns a
`SystemMemory{total, free, used, currentUsage, peakUsage}` (bytes; `currentUsage`/`peakUsage`
are this-process RSS).

| Field | Linux | Windows | macOS |
|-------|-------|---------|-------|
| `total` | `sysconf(_SC_PHYS_PAGES)` | `GlobalMemoryStatusEx().ullTotalPhys` | `sysconf(_SC_PHYS_PAGES)` |
| `free` | `MemAvailable` from `/proc/meminfo` (kernel ≥ 3.14; fallback `_SC_AVPHYS_PAGES`) — 2026-07: `_SC_AVPHYS_PAGES` alone counts the reclaimable page cache as "used", overestimating the memory load by tens of percents | `GlobalMemoryStatusEx().ullAvailPhys` | `host_statistics64(HOST_VM_INFO64)` — free + inactive pages (2026-07, replaces the historical stub that returned 0) |

**Contract**: `free == 0` (or `total == 0`) means "probe unavailable" — consumers (e.g.
the consumer's `ArrayBufferManager` memory-pressure purge) must treat it as *no pressure*,
never as a full machine.

---
