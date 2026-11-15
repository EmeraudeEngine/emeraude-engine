---
id: console-commands-wait-for-an-os-event-while-paused
title: While the engine is paused, a remote-console or MCP command waits for the next OS event
status: open
priority: high
scope: Console/RemoteListener, Console/MCP, Core
opened: 2026-11-15
tags: [console, mcp, main-loop, verification]
---

# While the engine is paused, a remote-console or MCP command waits for the next OS event

## Why

Found while drawing `docs/lifecycle-map.html` (synchronization S5 / S12). The network threads only QUEUE a command
(`src/Console/RemoteListener.cpp:466` `m_commandsQueue.push`, `src/Console/MCP/Server.cpp:491`
`m_pendingRequests.emplace_back`); the main thread executes it in `Controller::poll()` from
`Core::executeMainLoopCycle()` (`src/Core.cpp:1814`). Nothing on the network side wakes the main loop:
`Input::Manager::wakeUpEventsLoop()` is called only by the shader-failure path and `scheduleMainLoopCycle()`.

- Running: the wait is bounded by 1 / `DefaultMainLoopFrequencyHz` (100 Hz → 10 ms). Harmless.
- PAUSED: the inner loop calls `waitSystemEvents(mainLoopWaitTimeout(0.0))` (`src/Core.cpp:795`), and a fallback of
  0.0 means "wait indefinitely" unless a cycle was scheduled. A command sent while paused (the Pause key) is then
  executed only when some OS event arrives — including `Core.resume()` itself.

Read from the code only: not reproduced at runtime.

## What remains

1. Reproduce: a test instance, the Pause key, then a console / MCP command; measure the delay with and without moving
   the mouse over the window.
2. If confirmed: the network side wakes the main loop after queuing (`Input::Manager::wakeUpEventsLoop()`), or the
   paused wait gets a bound. The owner chooses.
3. Update the lifecycle map (S5, S12) and `docs/ai-runtime-control.md`.

## ⚠️ Traps

- `Core::m_pausable` is never set to true anywhere in the engine: the focus / visibility pause path
  (`Core::onNotification`, `OSNotifiesWindowLostFocus`) is dead today; only the Pause key (and `pause()` callers) pause.

## References

- `docs/lifecycle-map.html` → Threads & synchronization, S5 and S12.
