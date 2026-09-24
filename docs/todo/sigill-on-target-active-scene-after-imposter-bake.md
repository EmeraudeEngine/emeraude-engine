---
id: sigill-on-target-active-scene-after-imposter-bake
title: SIGILL while executing the console targetActiveScene() right after the imposter bake
status: open
priority: unranked
scope: Console / Scenes (terrain, imposter bake)
opened: 2026-09-24
tags: [crash, console, imposters, terrain]
---

# SIGILL while executing the console targetActiveScene() right after the imposter bake

## Why

Observed ONCE on 2026-09-24 (Release, `.claude-build-release`, engine `3a270818` + the terrain CSM
test): `projet-alpha --load-demo terrain --demo-options 100000,25,0,0,0,0,0` died with SIGILL
("Instruction non permise") while the Remote Console executed
`Core.SceneManagerService.targetActiveScene()`. The log line announcing the command is CUT in the
middle (`Executing command: Core.SceneManagerService.targetActiveScen`), and the line just before is
the last imposter atlas (`Imposter/Colonized2`) being baked — the command landed within seconds of
the bake completing. No core dump was kept (systemd-coredump absent on the machine).

## What remains

1. Reproduce: same command line, fire `targetActiveScene()` in a loop starting as soon as the port
   opens, under `gdb -batch -ex run -ex "thread apply all bt"` (a second run under gdb, with the
   command sent 10 s AFTER the bake, did NOT crash).
2. A SIGILL in a `-fno-exceptions` Release build is typically a compiler-emitted `ud2`: a non-void
   function falling off its end, `__builtin_unreachable()`, or `std::terminate` — read the frame, do
   not guess.
3. Suspect list to check first: the render-target list mutation at the end of the bake
   (`Scene::snapshotRenderTargets()`, fixed 2026-09-23 for a deadlock) racing the console thread.

## References

- `src/Scenes/Manager.console.cpp` (`targetActiveScene`), `src/Graphics/RenderTarget/ImposterBake.cpp`.