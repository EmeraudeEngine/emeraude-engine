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

### Required Extensions
- `ALC_SOFT_loopback` — loopback device rendering
- `ALC_EXT_thread_local_context` — per-thread OpenAL context

### Key Implementation Details
- Render thread uses 1024-sample chunks with 4-buffer streaming for smooth playback
- **Streaming WAV**: Samples written directly to file during capture, header patched on stop
- Crash-safe: data is on disk even if header isn't patched (recoverable by ffmpeg)
- Setup creates: loopback device, game context, playback device/context, render thread
- Shutdown stops recording, joins render thread, releases all OpenAL resources

### Code References
- `Recorder.hpp` — Class declaration with full Doxygen documentation
- `Recorder.cpp:setup()` — Loopback pipeline creation (4-step)
- `Recorder.cpp:renderThreadFunc()` — Render thread entry point
- `Recorder.cpp:startRecording()` / `stopRecording()` — Recording control
- `Recorder.cpp:writeWAVHeader()` — WAV header generation
- `Manager.cpp:startAudioRecording()` / `stopAudioRecording()` — Manager-level wrappers
- `Core.cpp:startAudioVideoRecording()` — Coordinated audio+video+voice-over recording start
