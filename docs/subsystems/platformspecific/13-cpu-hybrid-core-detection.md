## CPU Hybrid Core Detection (SystemInfo Enhancement)

**Files**: `SystemInfo.cpp`, `Types.hpp`

Enhanced CPU information with efficiency/performance core counts for hybrid architectures (Intel 12th gen+, Apple M1/M2/M3).

**New fields** in `PlatformSpecific::CPU` struct (`Types.hpp`):
```cpp
uint32_t efficiencyCores{0};   // E-cores (via hwloc cpukinds API)
uint32_t performanceCores{0};  // P-cores (via hwloc cpukinds API)
```

**Detection**: Uses hwloc >= 2.4 `cpukinds` API. Kind 0 = lowest performance (E-cores), Kind N-1 = highest (P-cores). On non-hybrid CPUs (`numKinds < 2`), both fields remain 0.

---
