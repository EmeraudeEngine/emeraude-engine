## CPU Hybrid Core Detection (SystemInfo Enhancement)

**Files**: `SystemInfo.cpp`, `Types.hpp`

Enhanced CPU information with efficiency/performance core counts for hybrid architectures (Intel 12th gen+, Apple M1 and later).

**New fields** in `PlatformSpecific::CPU` struct (`Types.hpp`):
```cpp
uint32_t efficiencyCores{0};   // E-cores (via hwloc cpukinds API)
uint32_t performanceCores{0};  // P-cores (via hwloc cpukinds API)
```

**Detection**: Uses the hwloc `cpukinds` API. Kind 0 = lowest performance (E-cores), Kind N-1 = highest (P-cores).
On non-hybrid CPUs (`numKinds < 2`), both fields remain 0. With three or more kinds, only kind N-1 is counted as
"performance"; every lower kind is added to `efficiencyCores` (an Apple M5 Pro-class chip reports Super ×2,
Performance ×4, Efficiency ×6 → `performanceCores = 2`, `efficiencyCores = 10`). The two fields are only printed by
`operator<<` today.

**hwloc 3.0 API (2026-10-10).** The external dependencies archive pins hwloc on upstream master (3.0.0a1, see the
generator's `libraries/hwloc.yaml`). 3.0 changes `hwloc_cpukinds_get_info()`: the info attributes come as one
`struct hwloc_infos_s **infosp` instead of `unsigned *nr_infos, struct hwloc_info_s **infos` — `SystemInfo` passes
`nullptr`. **This code does not compile against a 2.x archive** (v017 and older).

**Robustness.** The topology and the cpukind bitmap are owned by `std::unique_ptr` with `hwloc_topology_destroy` /
`hwloc_bitmap_free` as deleters, so every exit path releases them. `hwloc_topology_init()` and
`hwloc_topology_load()` are checked: a failure is logged (`TraceError`) and `fetchCPUInformation()` returns `false`
(the CPU fields filled by cpu_features stay valid). A failed bitmap allocation skips the hybrid detection, logged.

### Trap — hwloc <= 2.15 hangs forever on three-core-type Apple silicon

Symptom: the application freezes at 100 % CPU at start-up, the last log line being the `SettingsService` one; a
`sample <pid>` shows `SystemInfo::fetchCPUInformation → hwloc_topology_load → hwloc_look_darwin →
hwloc__darwin_build_perflevel_cache_level`. Measured on a Mac18,5 (2026-10-10): `hw.nperflevels = 3`
(Super / Performance / Efficiency), IOKit cluster types E (cpu 0-5), P (cpu 6-7) and **M** (cpu 8-11).

Cause, in hwloc's darwin backend:
1. `hwloc__darwin_look_iokit_cpukinds()` only knows the cluster types E (→ perflevel 1) and P (→ perflevel 0), and
   tests `kinds[1]` instead of `kinds[i]`, so M is silently filed as perflevel 0 and the mapping counts as matched.
2. The E kind (6 CPUs) therefore receives perflevel 1's caches, `cpusperl2 = 4`.
3. `hwloc__darwin_build_perflevel_cache_level()` cuts the cpuset in groups of `width`; after a partial last group
   `next` is `-1`, and `hwloc_bitmap_next(cpuset, -1)` restarts at the first CPU — forever.

Fix: upstream master (Apple, 1aa822adf / bf9797df6 / 3a72815b4 / ead59f4ae — the mapping by perflevel name and CPU
count), plus the generator's `patches/hwloc.patch` that stops the loop itself (still present on master). Workaround
on an old archive: `HWLOC_DARWIN_CPUKINDS_FROM_SYSCTL=1` in the environment (sysctl kinds are contiguous ranges whose
weights equal `cpusperl2` here, so no partial group; the CPU-to-kind assignment is then a guess, harmless on macOS
which cannot bind threads).

Verification (2026-10-10, Mac18,5, macOS 27.0, Release): a standalone probe on the 2.14 archive hangs (killed after
5 s); on the 3.0.0a1 archive it loads in < 10 ms with 12 PUs, 3 cpukinds (Efficiency cpus 0-5, Performance 8-11,
Super 6-7 — matches IOKit) and 3 L2 caches. A downstream application relinked on that archive (`--show-core-infos`,
settings copy) starts past `SystemInfoService` in the main process and in its CEF helper processes and prints
`Physical cores : 12`, `Logical cores : 12`, `Efficiency cores : 10`, `Performance cores : 2`; 18 s run, 0 `[Error]`
/ `[Fatal]`, terminated cleanly. Cascade build: 0 warning under the paranoid set; emeraude-base suite 2499/2499.

---