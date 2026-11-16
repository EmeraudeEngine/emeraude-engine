---
id: sky-material-atmosphere-read-while-its-creation-sets-it
title: A dynamic sky reads its material's atmosphere while the material's creation function sets it on a pool worker
status: open
priority: high
scope: Graphics/Renderable/DynamicSkyResource (build()), Graphics/Material/SkyAtmosphereResource (setAtmosphere(), atmosphere()), Resources::Container (getOrCreateResource())
opened: 2026-11-16
tags: [tsan, concurrency, defect, measured]
---

# A dynamic sky reads its material's atmosphere while the material's creation function sets it on a pool worker

## Why

ThreadSanitizer, 2026-11-16, the sweep of the testbed's scenes (Debug, the software device): `sky-lab` (2 reports) and
`city` (4) report a data race on the `std::shared_ptr< SkyAtmosphere >` of a `Material::SkyAtmosphereResource`:

| Written by | Read by |
|---|---|
| a pool worker running the creation function given to `Resources::Container::getOrCreateResource()`: `SkyAtmosphereResource::setAtmosphere()` (`DynamicSkyResource.cpp`, the lambda of `build()`) | the thread loading the sky: `DynamicSkyResource::build()`, a few lines below — `material->atmosphere()`, tested then copied — ← `DynamicSkyResource::load()` |

`getOrCreateResource()` "returns immediately; loading occurs asynchronously in thread pool": `build()` receives the
material while its creation function may still be running, and reads the member that function writes. A shared
pointer copied while it is assigned is undefined (the control block of one object, the pointer of another). The read
exists to follow a material of the same name that ALREADY existed ("it draws ITS atmosphere: the sky follows it"); for
a new material both branches end on the same object, which is why the picture is right.

## What remains

- Decide how `build()` learns which atmosphere the material draws without reading the material while it is created:
  e.g. ask the container whether the material existed BEFORE the call and read `atmosphere()` only then (a material
  that existed is loaded, or its creation is someone else's and the same question arises), or have the creation
  function publish the answer, or wait for the material's load (a dependency of the sky resource). An owner decision:
  the contract of `getOrCreateResource()` for a caller that needs the created state at once is the same question as
  scenes doc 19 § *onMeshLoaded hands out resources still being created*.
- Proof: `sky-lab` and `city` under ThreadSanitizer, 0 report on these two functions.

## ⚠️ Traps

- Not a logic / rendering race: both threads are loading threads. The protocols of scenes doc 12 do not apply.
- The recipe of a TSan run on the software device: `docs/caution-points.md` § *The software device gives a ray-query
  hit the identity of another instance*.

## References

- Scenes doc 19 § *onMeshLoaded hands out resources still being created* (the same family, another caller).
- Scenes doc 12, the sweep of 2026-11-16.
