## ExternalInput (Microphone Capture)

Audio capture service for recording from an external device (microphone). Uses OpenAL `ALC_EXT_CAPTURE` extension.

### Privacy by Design

**Microphone access is gated by `Core/Audio/Capture/Enable` (default: `false`).** The service does not initialize unless this setting is explicitly enabled. This ensures users are never recorded without consent.

### Dual Recording Modes

ExternalInput supports two recording modes for different use cases:

| Mode | Method | Use Case | Data Flow |
|------|--------|----------|-----------|
| **Memory** | `start()` | Multiplayer voice chat, real-time processing | Samples accumulate in `m_samples` vector |
| **Streaming** | `start(path)` | RushMaker voice-over, long recordings | Samples stream directly to WAV file |

#### Memory Mode
- `start()` — Begin capture, samples stored in RAM
- `stop()` — Stop capture, join thread
- `saveRecord(path)` — Write accumulated samples to WAV via `WaveFactory`
- Suitable for short captures or when samples need real-time processing

#### Streaming Mode
- `start(path)` — Open WAV file, write placeholder header, begin capture
- `stop()` — Join thread, patch WAV header sizes, close file
- Crash-safe: data is on disk even if stop() is never called
- No RAM accumulation — suitable for arbitrarily long recordings

### Thread Safety

The recording thread (`recordingTask()`) polls `alcCaptureSamples()` in a loop. On `stop()`:
1. `alcCaptureStop()` is called
2. `m_isRecording` flag set to `false` (thread exit condition)
3. Thread is joined immediately — ensures all data is flushed before `stop()` returns

### Available Devices in Settings

On initialization, ExternalInput writes the list of available capture devices to `Core/Audio/Capture/AvailableDevices` in settings. This mirrors the pattern used by Vulkan for GPU enumeration, allowing users to see available microphones when editing the settings file and copy the desired device name into `Core/Audio/Capture/DeviceName`.

### Code References
- `ExternalInput.hpp` — Class declaration with dual-mode API
- `ExternalInput.cpp:start()` — Memory mode start
- `ExternalInput.cpp:start(path)` — Streaming mode start (opens WAV, writes header)
- `ExternalInput.cpp:stop()` — Joins thread, patches WAV header in streaming mode
- `ExternalInput.cpp:recordingTask()` — Capture loop (branches on `m_streamingMode`)
- `ExternalInput.cpp:writeWAVHeader()` — Standard 44-byte PCM mono WAV header
- `ExternalInput.cpp:saveRecord()` — Memory mode WAV export via WaveFactory
- `SettingKeys.hpp:AudioCaptureEnableKey` — Master capture enable gate
- `SettingKeys.hpp:AudioCaptureAvailableDevicesKey` — Available devices array
