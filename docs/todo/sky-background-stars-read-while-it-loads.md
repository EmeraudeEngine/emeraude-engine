---
id: sky-background-stars-read-while-it-loads
title: A background's stars are read by the scene while its loading thread still adds them (use after free)
status: open
priority: high
scope: Graphics/Renderable (AbstractBackground, SkyBoxResource), Scenes (Scene.lighting), Graphics/Renderer
opened: 2026-11-26
tags: [defect, concurrency, sky, crash, threadsanitizer]
---

# A background's stars are read by the scene while its loading thread still adds them (use after free)

## Why

ThreadSanitizer, 2026-11-26, the testbed's `default` demo on the software device (window-less, the run meant for the
audio recorder): in two runs of two, a data race on the background's list of stars, then heap-use-after-free; the
first run died on it (`SEGV` in `Scene::environmentStarMask()`, `Scene.lighting.cpp`), the second went on with seven
reports.

The two sides, from the reports:

- **The writer, a thread of the pool** — the sky box is still LOADING: `Resources::Container< SkyBoxResource >::
  loadingTask()` → `SkyBoxResource::load(const Json::Value &)` → `AbstractBackground::parsePhotometry()` →
  `AbstractBackground::addStar()`, an `emplace_back` on a `std::vector< Graphics::CelestialBody >` which reallocates.
- **The readers** — the scene already holds that resource as its background (the demo took it with
  `Container::getRandomResource()` and set it before the load ended):
  - the logic thread, each cycle: `Scene::processLogics()` → `Scene::updateEnvironmentIBL()` →
    `Scene::environmentStarMask()` walks the stars (`CelestialBody::isInTexture()`, `illuminance()`,
    `angularDiameter()`, the direction) — under the logic-cycle lock and the SHARED scene access, which the loading
    thread does not take;
  - the rendering thread: `AbstractBackground::luminance()` read from `Renderer::recordIrradianceProbeUpdate()`.

A resource read while it loads: the vector's storage is freed under the reader. A crash in Release is possible
whenever a sky with stars in its photometry finishes loading while its scene already runs; not observed outside
ThreadSanitizer.

## What remains

- The protocol is the owner's choice. The family already has answers to weigh (scenes doc 12 § *Plain state shared
  by the logic and rendering threads*, and the item `sky-material-atmosphere-read-while-its-creation-sets-it`, the
  same fault on the sky's material): the photometry parsed into locals and PUBLISHED once (a flag stored last, or
  the scene reading the stars only from a resource that reports itself loaded), versus a lock on the list.
- What else `parsePhotometry()` writes that the scene or the renderer reads before the load ends (`luminance()` is
  in the reports; the rest was not read).
- Proof: the same ThreadSanitizer run (projet-alpha `docs/unit-testing.md` § ThreadSanitizer; `default`, a settings
  copy forcing the software device, several launches — the sky is a random pick): 0 report naming
  `AbstractBackground`, `CelestialBody` or `environmentStarMask`.

## References

- `src/Graphics/Renderable/AbstractBackground.hpp` (`addStar()`, `luminance()`),
  `src/Graphics/Renderable/AbstractBackground.cpp` (`parsePhotometry()`),
  `src/Graphics/Renderable/SkyBoxResource.cpp` (`load()`), `src/Scenes/Scene.lighting.cpp`
  (`environmentStarMask()`, `updateEnvironmentIBL()`).
- `docs/subsystems/scenes/12-*` § *Plain state shared by the logic and rendering threads*.
