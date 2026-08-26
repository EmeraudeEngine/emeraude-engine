---
id: compressed-gltf-sigill-at-idle
title: SIGILL at idle after a successful compressed-glTF load — unattributed
status: open
priority: medium
scope: Scenes/SceneLoaders/GLTF
opened: 2026-08-11
tags: [crash, ktx2, meshopt]
---

# SIGILL at idle after a successful compressed-glTF load

## Why

The `sponza` demo (asset `Sponza.ktx2.glb`, KTX2/Basis + meshopt) died in **SIGILL** (illegal
instruction, exit 132) **after a successful load**, at idle, with no command running, 1-3 min
after the last capture. No core dump was collected ⇒ no post-mortem on that run. **Neither
cleared nor incriminated.**

## Facts for whoever resumes

- A **different** run, with a wrong demo id (`Sponza` instead of `sponza`, so **no glTF parsed at
  all**), died in `*** stack smashing detected ***` / SIGABRT on the "demo not found" path, in the
  main process AND the CEF helper ⇒ **the binary has at least one pre-existing fault**.
- SIGILL typically signs a jump into corrupted code (overwritten vtable / function pointer), which
  fits memory corruption — consistent with the other run's stack smashing.
- Checked and found clean: the 10 `iterateAccessor` sites are all in
  `loadMeshes`/`loadSkins`/`loadAnimations`, therefore BEFORE `m_bufferCache.reset()`, and
  `loadLights`/`loadCameras`/`buildNodeDescriptors` read no accessor ⇒ no use-after-free through
  `MeshoptBufferAdapter`'s raw pointer.

## What remains

- [ ] **Decided next step**: relaunch under `gdb` with core dumps enabled, leave it idle, capture
  the stack.
- [ ] A/B against the uncompressed `Sponza.glb` (repoint `Sponza.cpp:91`, rebuild).

## ⚠️ Traps

- The demo's ClassId is **`sponza` in lowercase** (`Sponza.hpp:39`); `Sponza` matches nothing and
  the engine answers `No built-in scene available` without suggesting anything.
