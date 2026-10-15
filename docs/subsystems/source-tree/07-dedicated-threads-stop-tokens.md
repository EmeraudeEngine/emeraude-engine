## Dedicated threads stop through a std::stop_token (Ave Robustus II, Oct 2026)

Every dedicated thread of the engine is a `Base::Thread` (emeraude-base, the `std::jthread` model: a body taking a
`std::stop_token` receives it; `requestStop()`; the destructor requests the stop, then joins; no `detach()`; a
self-join aborts). Since 2026-10-15 no owner stops its thread with a hand-made flag (item `jobs-owned-by-their-starter`,
closed 2026-10-16 after the three OS; plan Ave Robustus II § 4.2, P2).

### The rules

- The body takes the thread's `std::stop_token` and checks it with a bounded latency.
- A wait is `std::condition_variable_any::wait(lock, stopToken, predicate)`: a stop request wakes it, no notification of
  the owner's own is needed. It returns `false` only when the stop is requested AND the predicate is still false, so a
  queue that must be drained before the end (an encoder, the log) keeps being drained.
- The `Base::Thread` member is declared LAST: it uses every member above it, so it is destroyed (stopped and joined)
  first.
- A flag that is also a STATE (`RemoteListener::isRunning()`, `ExternalInput::isRecording()`) stays, but no longer
  stops the thread.

### The owners

| Owner | Thread | Stop | Notes |
|---|---|---|---|
| `Audio::TrackMixer` | `m_eventThread` | its token; `m_fadeCv` is `_any` | the fade loop checks it between 16 ms steps |
| `Audio::Recorder` | `m_renderThread` | its token, once per chunk (5 ms pace) | |
| `Audio::ExternalInput` | `m_process` | its token | ⚠️ fixed: `m_isRecording` (a plain `bool`) was read by the thread while the main thread wrote it; the loop polled without a pause (a core at 100 %, now 5 ms, the capture buffer holds 21 ms at least); `stop()` now drains what the device holds after `alcCaptureStop()`; `onTerminate()` ends a running recording first |
| `Console::RemoteListener` | `m_networkThread` | a `std::stop_callback` on its token stops the `io_context` | ⚠️ fixed: the destructor returned early when a fatal accept error had lowered `m_running`, the context still running the clients' handlers on members destroyed before the thread |
| `Graphics::Recorder` | each session's `encodingThread` (VP9, hardware H.265) | its token; the queue is drained, then the session finalized | ⚠️ fixed: `threadRunning` / `finished` declared after the thread; the VP9 session's destructor joined without requesting the stop |
| `TracerLogger` | `m_thread` | its token; every entry pushed before the stop is written, then the footer | ⚠️ fixed: the loop ended right after a pass (an entry pushed meanwhile was lost); `m_condition` was declared after the thread |
| `Core` logic and rendering loops | locals of `Core::run()` | a MEMBER `std::stop_source m_loopsStop` (owner decision 2026-10-15) | a member, not the threads' own tokens: `stop()` may run before `run()` starts them, which then end at once; the on-demand redraw wait (`m_redrawCondition`, `_any`) is woken by the token |

Not in scope, already self-contained: `Graphics::Renderer::m_pipelineCacheRestore` (an uninterruptible driver call,
joined), `PlatformSpecific/Helpers.linux.cpp` (a local worker joined before return). `Core::m_isMainLoopRunning` is the
main thread's own loop, not another thread's stop.

### Found on the way

`Notifier::push()` read the base class's `usable()` flag, which `ServiceInterface::terminate()` writes unlocked: the
track mixer's event thread still notified (through `Core::onNotification()`) while the main thread terminated the
notifier, a data race TSan reported in `EmeraudeTrackMixerStress` once the loops stopped faster. The notifier now keeps
its own `m_accepting` under its list lock (true from the end of `onInitialize()` to the start of `onTerminate()`).
⚠️ Any `usable()` read from another thread than the one that terminates the service is the same race.

### Verified (Linux, 2026-10-15)

- Build: GCC Release 0 warning; clang `-fsyntax-only -Wthread-safety -Wdocumentation` clean on the touched TUs;
  clang-tidy 21.1.6, changed lines: 1 on purpose (the ledger).
- TSan: `EmeraudeTrackMixerStress` 3 runs (~28 500 calls each), `EmeraudeResourceJobsStress`,
  `EmeraudeOverlayResizeStress` (285 resizes): 0 report.
- projet-alpha, validation layer, a 6 s rush on the `default` demo, with the voice-over and the log file on:
  - hardware H.265: 181 pictures, 0 decode error;
  - forced VP9: 182 pictures, drained in the background after the stop;
  - the game and voice-over WAVs: 6.0 s each, valid;
  - the log file ends with "Log file closed properly" after the last trace;
  - the remote console thread stopped; the shutdown took 0.37 s; 0 VUID, exit 0.
- `OnDemand` rendering (test-only build, the mode a downstream application uses): the menu drawn, a screenshot taken,
  the shutdown from the idle wait in 0.74 s.

### Verified on the peers (2026-10-15/16)

- macOS (M6, AppleClang PARANOID, 0 warning):
  - TSan, 0 report: `EmeraudeTrackMixerStress` 2 runs (~19 650 calls), `EmeraudeResourceJobsStress`;
  - a VP9 rush with the voice-over: 174 pictures, 0 decode error; the WAVs valid; the journal closes properly; the
    shutdown 0.32 s;
  - `sponza` quit while loading: 0.59-0.69 s;
  - 0 validation message.
- Windows (RTX 3500 Ada, MSVC, 0 warning; base suites 2531/2531):
  - the H.265 and VP9 rushes with the voice-over: 181 pictures each, the WAVs valid, the journals closed properly,
    the shutdown 0.63-0.73 s;
  - a VP9 rush with `Core.shutdown()` sent 0.01 s after the stop: the background drain completes inside the shutdown
    (181/181 pictures, exit in 0.76 s);
  - the downstream application (OnDemand rendering, Vulkan 1.1 target): shutdown from the idle wait in 0.87-1.08 s;
  - 0 validation message everywhere.
- ⚠️ The game-audio WAV of the `default` demo is silent on every OS: the demo plays no sound during a rush. With
  music started (`Core.AudioManagerService.TrackMixerService.play()`) the loopback WAV carries the signal (Linux).

