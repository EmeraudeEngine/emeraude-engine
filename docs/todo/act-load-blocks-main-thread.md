---
id: act-load-blocks-main-thread
title: A built-in act is loaded on the main thread, which stops answering the window system for its whole duration
status: in-progress
priority: high
scope: projet-alpha Stage/Loader, engine Core (main loop)
opened: 2026-09-25
tags: [wayland, loading, responsiveness]
---

# A built-in act is loaded on the main thread, which stops answering the window system for its whole duration

## Why

`Application::onCoreStarted()` → `Stage::loadBuiltInScene()` → `Loader::load()` builds the whole scene on the
main thread, before the main loop: 9 s for `terrain` (750 000 trees, 3.2 s of tree growth alone). During that time
nothing polls the window. It is what let GNOME drop the Wayland connection (fixed the other way: the render thread
now reads the display, `Window::drainDisplayConnection()`, `docs/caution-points.md` § *GNOME dropped the Wayland
connection*), and it still leaves the compositor's `xdg_wm_base.ping` unanswered: GNOME may dim the window as
"not responding" during a long load, and no input is taken.

## Fixed downstream (2026-11-13, owner decision: the loader on a worker, the Stage pumps)

- projet-alpha `Stage::buildPumpingWindow(build)`, the one generic loading behaviour (owner, 2026-11-13: no demo acts
  differently): the build step of every act (`Builtin::Loader::load()`, `Scenes::Manager::loadScene()`) runs on a
  dedicated `Base::Thread` (never the resource pool) while the main thread waits in `Window::waitEvents(0.016)`; a
  load requested meanwhile (the UI's callbacks run during the pump) is refused with a warning; `loadActiveAct()` stays
  on the main thread. `AbstractDemo::pumpEventsUntil()` is gone: WorldLobby and JungleRuins call the USD loader
  synchronously like any demo (they used to spawn their own thread and pump GLFW from `onBuilding()`).
- Engine side: `Window::waitEvents()` is the wait (the ping is answered by GLFW's dispatch at every pass); the
  measurement that opened this item is in projet-alpha's `docs/caution-points.md` § Core Application.
- Symptom that confirmed the mechanism (owner, Ubuntu 26.04, GNOME 50.1): the "Wait / Force Quit" dialog on every
  interaction during terrain's load; mutter's `check-alive-timeout` is 5000 ms and it pings on each click / key / focus.
- Measured after (2026-11-13, same machine, `terrain`, validation ON): 61.3 s of build with the owner clicking, typing
  and moving the pointer, no dialog, 0 VUID, 0 loss, clean shutdown.

## What remains

- [x] TSan on `default` (2026-11-13, Debug + TSan, `--disable-cef`, the RADV device forced: the NVIDIA driver dies under
  TSan at device creation, item `tsan-logic-render-shared-state-races` § Traps): 132 reports, 127 in uninstrumented
  third-party code (glycin, glib), 5 in the engine between the LOGIC and RENDERING threads only (filed as that item);
  the build thread is party to none (it only allocated the objects). Clean start and `Core.shutdown()`.
- [x] TSan on `terrain` (2026-11-13, RADV forced, 10.5 GB, 415 s to the start, the owner closed it with the window's
  close button): 145 reports, the build thread an ACCESS party in none (the allocator of the racing objects in 10),
  the rest the glycin / glib noise and the logic ↔ rendering family of `tsan-logic-render-shared-state-races` (the
  ready flag also from `MultiLayerMeshResource`). ⚠️ The Debug teardown after that close printed VMA assertions
  ("Some allocations were not freed before destruction of this memory block", "Unfreed dedicated allocations",
  then a null allocator and TSan's DEADLYSIGNAL) with `CloudShape… is still used 3 times` just before: to check on
  the Linux workstation in Debug WITHOUT TSan, terrain closed by the close button, on this commit and on its
  parent (a Release close is silent: VMA asserts are compiled out).
- [ ] `world-lobby` and `jungle-ruins` (their USD loads are now synchronous on the Stage's worker) on a machine that has
  the USD assets: the Ubuntu laptop's data checkout has no `data-stores/USD/`, so both failed there for the missing
  file, cleanly (2026-11-13: exit 0, 0 loss, 0 VUID, the failure traced by the worker).
- [ ] Peers (macOS-PA, Windows-PA): a built-in load with the window answering (move the pointer during terrain's load).
- [ ] Then delete this item (its knowledge lives in projet-alpha's caution-points and `src/AGENTS.md`).

## Former plan

- Load the act on a worker while the main loop runs (a loading state the stage shows), then enable the scene;
  or pump the window events between the loader's steps — the second is simpler but runs input callbacks in the
  middle of a load (re-entrancy to check).
- Still synchronous on 2026-10-13. The choice of form has a precedent (owner, 2026-08-09, USD stage load): the
  least invasive form was chosen and is in use — the loader runs on the resource thread pool
  (`Scenes::Loaders::Interface::loadAsync()`) while the caller waits pumping the window
  (`AbstractDemo::pumpEventsUntil()` → `Window::pumpEvents()`, used by projet-alpha's `WorldLobby` and
  `JungleRuins` demos). The pumping lives on the caller side, never in the loader (no loader ↔ windowing
  coupling); rewriting the demo lifecycle (`AbstractDemo::create()`) as asynchronous was rejected as too broad.
  Applying the same split here is the default; confirm with the owner which repository carries the change (the
  downstream loader, or an engine loading contract it calls).

## References

- Downstream call chain (projet-alpha): `src/Application.cpp:385` (`onCoreStarted()`, `--load-demo`) and
  `:1011`, `:1053` (the UI's act-load notifications) → `src/Stage.cpp:311-331` (`Stage::loadBuiltInScene()`, the
  synchronous `m_loader.load()` at `:323`).
- Engine: `src/Core.cpp:350-351` (the render thread drains the display connection,
  `Window::drainDisplayConnection()`); `docs/caution-points.md` § *Fixed: GNOME dropped the Wayland connection during
  a long load*.

## Evidence 2026-10-01 (Windows peer, RTX 3060 Laptop)

`Core.shutdown()` sent to `terrain` 40 s after launch, while its act was still being built on the main thread (no
imposter baked yet): "[UIManagerService] No default page found !", "[UIManagerService] Timeout waiting for 1
web-view(s) to complete CEF teardown — abandoning.", then "[WebView] Web-view 'ApplicationMenuCEF' destroyed while its
CEF browser is still alive — forcing CloseBrowser(true)". The same shutdown at 100 s (load finished): clean. Also:
`Stage.loadDemo(terrain)` kept the console blocked > 60 s on that machine.

## A console command waits for the whole load (2026-10-07, Windows peer, RTX 3060, `terrain`)

`Core.shutdown()` sent 20 s after launch was only EXECUTED when the synchronous load ended ("The application
successfully started" at the same instant): 73-77 s from the send to the exit, the console silent for > 30 s in the
meantime; the teardown itself took ~4 s. This was the slow-shutdown half of the closed item
`shutdown-hangs-after-act-removal` (its other half, the scene re-created after the act removal on `relief`, is gone:
10 / 10 shutdowns under 1 s). The console and MCP are unusable for the duration of a built-in act's load.

