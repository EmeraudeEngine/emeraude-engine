# The shutdown sequence (Oct 2026)

How a run ends, from the stop request to the last primary service, as the code does it (`src/Core.cpp`:
`stop()`, `proceedStop()`, the end of `run()`, `terminate()`, `~Core()`). Written for Ave Robustus II P6
(projet-alpha `docs/plans/ave-robustus-ii.md` § 6: "no document describes the whole shutdown sequence").

## 1. The stop request

Three ways in, one way through (`proceedStop()`):

| Request | The application's say | Then |
|---|---|---|
| `stop(exitCode)` — the user (window close, `Core.shutdown()`, a key) | `onBeforeCoreStop()` may VETO; after `MaxStopVetoCount` vetoes a "Force Quit / Wait More" dialog (one at a time; "Wait More" grants a new grace period) | `proceedStop()` |
| `requestSystemStop(reason)` — a termination signal caught by the main loop (SIGTERM / SIGHUP / SIGINT) | asked as for `stop()`, but a veto only lasts until `SystemStopGracePeriod`; no veto counting, no dialog | `checkSystemStopDeadline()` forces `proceedStop()` at the deadline (exit code 0: the system ended the run) — unless the application cancels it first (`cancelStopRequest()`: its user said Cancel; macOS's "quit" Apple Event is answered "user canceled") |
| `stopOnDeviceLoss()` — the graphics device is lost | told (`onBeforeCoreStop()`, it may save its data), cannot refuse | `proceedStop()` with `DeviceLostExitCode` |

`proceedStop()`: notifies `ExecutionStopping`; stops a running audio / video rush (`stopAudioVideoRecording()`);
requests the stop of the two loops (`m_loopsStop.request_stop()` — the stop token also wakes a rendering thread asleep
in on-demand mode, so it exits at once); lowers the main loop flag; notifies `ExecutionStopped`.

## 2. The end of `run()`

1. The main loop ends; the previous termination-signal handlers come back (an application's own teardown, e.g.
   projet-alpha's `CefShutdown()`, runs with them).
2. `ExitingMainLoop` is notified.
3. The rendering thread is joined, then the logic thread (`Base::Thread`: stop requested in step 1, joined here;
   `source-tree/07`).
4. `terminate()` — below. Nothing else touches the thread pool: every pool job that uses an object is OWNED by that
   object (`Base::TaskHandle` / `Base::TaskGroup`, Ave Robustus II P2, 2026-10-15) and stopped and waited by it when it
   goes. Core has no global cancel flag and no pool drain any more.

## 3. `Core::terminate()`

In this order (each step traced, a failing service counted, the exit code `EXIT_FAILURE` if any failed):

1. `Resources::Manager::stopBackgroundWork()` — the resource manager's OWN jobs (loads, reloads, the release pass) stop
   first, before the services they touch go. The one ordering Core states for background jobs.
2. The user services, in reverse order of their start; then `unloadUnusedResources()` (what they held).
3. The secondary services, in reverse order, with `unloadUnusedResources()` after EACH: the Vulkan resources (VMA
   allocations) must be freed before the device goes with the instance service.
4. Vulkan debug builds (`VulkanTrackingDebugEnabled`): every Vulkan object still alive is listed and counted as an error.

`terminate()` is idempotent; the services' own `onTerminate()` stop and join what they started (examples: the recorder
joins the rushes still encoding — announced, a rush is never cut, graphics `11-10-video-recording.md`; the console
controller detaches every registered object, console `11-critical-points.md`).

## 4. `~Core()`

1. A safety net (decision D3-a): a path that left user or secondary services registered calls `terminate()` here,
   before the primary services they depend on. A normal run has emptied both lists: nothing happens.
2. The primary services, in reverse order of their start, then `PrimaryServices::terminate()`.

## `ThreadPool::wait()` is not a shutdown tool

It waits until the WHOLE pool is idle — every owner's tasks, not the caller's — and its own contract (base
`ThreadPool.hpp`) lists its limits: it may return while other threads keep enqueuing, and called from a pool task it
deadlocks once the pool is saturated. That is why the shutdown no longer calls it: an object waits for ITS work through
its `TaskHandle` / `TaskGroup` (`waitForAll()`), which neither sees nor waits for anyone else's.

## Measured

Shutdown request → "Core level terminated", Release, 2026-10-16/17: 0.25-0.65 s on the bench demos (Linux, macOS),
0.8-1.6 s on Windows (CEF's own `CefShutdown()` included); `terrain` 1.7 s (its scene destruction); a run in on-demand
mode stopped while the rendering thread sleeps: 0.74 s (Linux). A quit right after a software rush waits for its
encode (12.7 s measured on an Apple M6).
