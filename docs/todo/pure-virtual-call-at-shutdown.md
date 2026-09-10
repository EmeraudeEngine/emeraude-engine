---
id: pure-virtual-call-at-shutdown
title: "'pure virtual method called' at shutdown, once in ~12 runs — not reproduced, not attributed"
status: open
priority: unranked
scope: Scenes / Core shutdown
opened: 2026-09-10
tags: [shutdown, lifetime, crash, intermittent]
---

# 'pure virtual method called' at shutdown, once in ~12 runs

## Why

Observed **once**, on `--load-demo sponza` (Release, Linux, RTX-class device, validation layers
ON, zero VUID in that run). Every other launch of the same session — a dozen or so, including
three back-to-back reproduction cycles run immediately afterwards — shut down with exit code 0 and
the full graceful-termination log.

Recorded rather than dropped because the signature names a **lifetime** defect, and this codebase
has already paid for one of that family (a snapshot keeping a *component* alive while its *entity*
died — see the game-logic cycle work). An intermittent abort at teardown is exactly the kind of
thing that gets rediscovered from scratch in six months.

## The evidence

Tail of the run, verbatim:

```
[Info][Stage] Cleaning the active act 'sponza' ...
[Info][ActorPlayer] Disabling control for player 'ActorPlayer' ...
[Success][Act] Player control unregistered !
[Info][Stage] Removing the act 'sponza' ...
[Success][AVConsoleManager] Virtual video device 'Eyes_55b433a2e0e8' removed !
[Success][AVConsoleManager] Virtual audio device 'Ears_55b433a2dae8' removed !
[Success][RendererService] Scene render target created (2880x1620, format: R16G16B16A16_SFLOAT).
[Success][Scene] Scene will use environment cubemap 'Forrest' !
logicsTask : 361.631 ms
logicsTask : 42.0196 ms
pure virtual method called
terminate called without an active exception
```

⚠️ **The interesting part is not the abort, it is the two lines before it.** A scene render target
is created and a background cubemap bound **after** the act was removed — something built a scene
while the shutdown was already tearing one down. `pure virtual method called` is then the textbook
consequence: a virtual dispatch on an object whose derived part is already destroyed.

## What remains

- Reproduce. 3 clean launch/shutdown cycles right after did **not** (`exit=0`, no occurrence), so
  it needs either a loop of many cycles or a specific trigger. Suspect a race between
  `Core::shutdown()` (console → main thread) and the logic thread, given the `logicsTask` lines
  interleaved with the teardown.
- Identify what creates a scene during shutdown — that is the actual defect; the abort is
  downstream of it.
- ⚠️ Do NOT "fix" it by ordering the destructors differently until the creation-during-shutdown is
  explained: silencing a pure-virtual call by changing teardown order hides the race instead of
  removing it.

## ⚠️ Traps

- **Not attributed.** It appeared during the post-processing settings work of 2026-09-10, but
  nothing in that work touches scene or act lifetime, and it did not reproduce. Do not assume it
  is a regression from that change, and do not assume it is not.
- `Core.shutdown()` over the console runs on the **main** thread while the logic and rendering
  threads are still live (`Core.cpp` spawns both) — the timing window is real, not theoretical.

## References

- `src/Core.cpp` — the logic/rendering thread pair and the shutdown sequence.
- `docs/todo/jungle-ruins-fence-timeout-abort.md` — the other open teardown-time abort; check
  whether they share a cause before treating them as two.
