## Recorder (Loopback Audio Capture)

Audio recording service that captures game audio output using OpenAL Soft loopback passthrough. This is **not** microphone recording — it captures what the game renders to the audio device.

### Architecture

The loopback pipeline operates in three stages:
1. **Loopback device** renders game audio into a buffer (no hardware output)
2. **Dedicated render thread** continuously pulls samples via `alcRenderSamplesSOFT()`
3. Samples are **forwarded to a real playback device** for speaker output and optionally **streamed to WAV file**

```
Game Audio Sources → Loopback Device → Render Thread ─┬─→ Speaker Playback
                                                       └─→ WAV Streaming (optional)
```

### Symmetric API

Both `Audio::Recorder` and `Graphics::Recorder` share the same recording API pattern:

| Method | Purpose |
|--------|---------|
| `startRecording(path)` | Begin streaming recording to WAV file at given path |
| `stopRecording()` | Stop recording and finalize WAV header |
| `isRecording()` | Check if recording is active |

Path generation and coordinated start/stop of all recorders is owned by `Core::startAudioVideoRecording()` / `Core::stopAudioVideoRecording()`. See `src/AGENTS.md` Core section.

### The render thread owns the recording file (2026-11-26)

- The file, its path and its byte count are a LOCAL of `renderThreadFunc()` (`Recorder::RecordingFile`): opened,
  written, completed (the two size fields of the header) and closed by that thread alone.
- `startRecording()` / `stopRecording()` post a request and wait for the answer: a state
  (`Idle → StartRequested → Starting → Recording → StopRequested → Stopping → Idle`), the requested path, the answer
  and the saved byte count, all four behind `m_recordingAccess`; the thread publishes each state it reaches and
  notifies `m_recordingChanged` with the mutex held. The thread takes the mutex once per loop turn (5 ms) to read a
  request and once to answer it — never around file work.
- The wait is bounded (`RequestTimeout`, 5 s). A start request the thread had not taken is withdrawn; one it is
  serving stays its own, and `isRecording()` (any state but `Idle`) tells what became of it. A stop that times out
  gives nothing up: the thread still completes the file.
- The thread completes a recording still running when it is asked to stop (the shutdown), and answers a request it
  will not serve: `onTerminate()` no longer calls `stopRecording()`.
- A write failure, or a file reaching the 4 GiB a WAV header can state, ends the recording: the header is completed
  with the bytes that reached the file.
- Before: the stream and its count were members written by both threads behind one relaxed atomic flag — 4 data
  races under ThreadSanitizer (2026-11-15), and `stopRecording()` patched and closed the stream while the thread could
  still be inside `write()`.
- Proven (Linux, Release, recorder on, video off in a settings copy, `Core.toggleRecording()`): three recordings
  started and stopped, a fourth left running into the shutdown — four files whose RIFF size is the file's length − 8
  and whose data size is its length − 44, 2 channels, 48 kHz, the played sound in each.
- ThreadSanitizer (Linux, Debug, the software device, window-less, `default`, the same scenario — three recordings
  and a fourth left running into the shutdown): 0 report naming `Audio::Recorder`, the four files complete. The run's
  seven reports are another fault's (engine item `sky-background-stars-read-while-it-loads`).
- `Audio::ExternalInput` (the microphone's capture) was read for the same fault and does not have it: its file is
  opened before its thread is started and completed after that thread is joined. `Graphics::Recorder` (its encoder
  thread and its file) was NOT read: engine item `graphics-recorder-file-sharing-not-read`.

### Required Extensions
- `ALC_SOFT_loopback` — loopback device rendering
- `ALC_EXT_thread_local_context` — per-thread OpenAL context

### Key Implementation Details
- Render thread uses 1024-sample chunks with 4-buffer streaming for smooth playback
- **Streaming WAV**: Samples written directly to file during capture, header patched on stop — by the render thread
- Crash-safe: data is on disk even if header isn't patched (recoverable by ffmpeg)
- Setup creates: loopback device, game context, playback device/context, render thread
- Shutdown joins the render thread (which completes a running recording), releases all OpenAL resources
- ⚠️ The recorder owns the context every other OpenAL object of the engine is created on (its game context is made
  current BEFORE the audio manager creates its sources): `Audio::Manager::onTerminate()` terminates it LAST, after the
  default source and the pool, the other sub-services and the buffers of the sound and music resources (2026-10-16,
  [`06-critical-points.md`](06-critical-points.md))

### Code References
- `Recorder.hpp` — Class declaration with full Doxygen documentation
- `Recorder.cpp:setup()` — Loopback pipeline creation (4-step)
- `Recorder.cpp:renderThreadFunc()` — Render thread entry point
- `Recorder.cpp:startRecording()` / `stopRecording()` — Recording control
- `Recorder.cpp:writeWAVHeader()` — WAV header generation
- `Manager.cpp:startAudioRecording()` / `stopAudioRecording()` — Manager-level wrappers
- `Core.cpp:startAudioVideoRecording()` — Coordinated audio+video+voice-over recording start
