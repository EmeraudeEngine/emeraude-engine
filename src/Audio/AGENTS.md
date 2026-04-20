# Audio System

Context for developing the Emeraude Engine 3D spatial audio system.

## Module Overview

3D audio system based on OpenAL-Soft with full support for spatial effects, music streaming, and seamless integration with the scene graph via SoundEmitter components.

## Audio-Specific Rules

### Coordinate Convention
- **Y-DOWN mandatory** in Audio abstraction
- Internal conversions to OpenAL if necessary
- Total consistency with the rest of the engine (Physics, Graphics, Scenes)

### Sounds vs Music Philosophy
- **Sounds**: Fully loaded in RAM, MONO format, for short sound effects
- **Music**: Streaming from RAM to OpenAL, STEREO format supported, for long tracks
- Supported formats: Delegated to `libsndfile` (WAV, OGG, FLAC, etc.)

### Component Architecture
- **SoundEmitter**: Component attachable to Entity/Nodes in the scene graph
- Emitter count: Unlimited at scene graph level
- **Pooling**: Emitters require OpenAL sources from a pool during playback
- Automatic 3D positioning via scene graph

### Resources Integration
- **MANDATORY**: Use Resources system for loading
- `SoundResource`: Sound loading management
- `MusicResource`: Music loading management
- `PlaylistResource`: Playlist manifest (JSON, `MusicPlaylists/` store) listing MusicResource names in order
- `SoundfontResource`: SoundFont 2 (SF2) sample banks for MIDI rendering
- Fail-safe pattern: Neutral audio resources on failure
- **Default/Fallback sound**: Retro double-beep generated via `WaveFactory::Synthesizer`

## Development Commands

```bash
# Audio tests
ctest -R Audio
./test --filter="*Audio*"
```

## Important Files

- `Manager.cpp/.hpp` - Main manager (devices, activation, capture). Writes available playback devices to settings on init.
- `SoundResource.cpp/.hpp` - Sound loading and management (mono)
- `MusicResource.cpp/.hpp` - Music loading and management (stereo + streaming)
- `PlaylistResource.cpp/.hpp` - Playlist manifest (JSON) bound to the `MusicPlaylists` store
- `SoundfontResource.cpp/.hpp` - SoundFont 2 (SF2) sample bank loading
- `Source.cpp/.hpp` - OpenAL source abstraction (pool)
- `Speaker.cpp/.hpp` - Listen point abstraction (non-OpenAL, API consistency)
- `TrackMixer.cpp/.hpp` - Jukebox for playlist management (see TrackMixer section below)
- `Ambience.cpp/.hpp` - Ambient sounds (loop channel + random effects, State enum)
- `AmbienceChannel.hpp` - Individual channel for ambient sound effects
- `AmbienceSound.hpp` - Ambient sound configuration
- `Recorder.cpp/.hpp` - Audio recording via OpenAL Soft loopback passthrough (game audio capture, not microphone)
- `ExternalInput.cpp/.hpp` - Microphone capture (dual mode: memory for real-time, streaming for rush voice-over)
- `@docs/coordinate-system.md` - Y-down convention (CRITICAL)

## Development Patterns

### Adding a SoundEmitter to an Entity
1. Create the SoundEmitter component
2. Attach it to the scene graph Node
3. Load the SoundResource via the Resources system
4. 3D positioning automatically follows the Node

### Audio Effects Configuration
- All OpenAL-Soft effects available via filter system
- 3D effects: Distance attenuation, Doppler, directivity
- Environment effects: Reverb, occlusion, etc.
- Applied via configurable filters

### Source Pooling Management
- OpenAL source pool managed by Manager
- Automatic request on emitter `play()`
- Automatic release at playback end
- Priorities manageable if pool saturated

### Ambient Sounds with Ambience
1. Configure a continuous background track (loop channel)
2. Add random sound effects (probabilities, intervals)
3. System automatically manages the mix

### Ambience State Management
Ambience uses a `State` enum for playback state. See `Ambience.hpp:State`

**Playback states (State enum):**
- `Stopped` - Not started or stopped
- `Playing` - Active playback
- `Paused` - Gameplay paused (sources kept)

**Two orthogonal control levels:**

1. **Gameplay level** - `pause()`/`resume()`:
   - Direct OpenAL control, sources kept in memory
   - Usage: game pause in an active scene
   - See `Ambience.cpp:pause()`, `Ambience.cpp:resume()`

2. **Scene Manager level** - `suspend()`/`wakeup()`:
   - Releases/reacquires sources to/from pool
   - Usage: called by `Scene::disable()`/`enable()` on scene change
   - `m_suspended` flag orthogonal to `m_state`
   - See `Ambience.cpp:suspend()`, `Ambience.cpp:wakeup()`

**Behavior:**
- If paused then suspended → on wakeup, restored to paused state
- `update()` only processes `Playing` state

## Critical Points

- **Device detection**: OpenAL-Soft latest can be finicky on laptops
- **Sound format**: MONO mandatory for 3D spatial positioning
- **Music format**: STEREO supported (no spatialization)
- **Thread safety**: OpenAL handles threading internally
- **Coordinate system**: Always Y-down in Audio API
- **Resource integration**: Never load directly, go through Resources
- **Microphone privacy**: `Core/Audio/Capture/Enable` must be explicitly `true` before any microphone access. Default is `false`. Voice-over (`Core/RushMaker/EnableVoiceOver`) depends on this gate.
- **Available devices in settings**: Both playback (`Core/Audio/AvailableDevices`) and capture (`Core/Audio/Capture/AvailableDevices`) device lists are written to settings on each startup for easy device switching via settings file editing

## SoundResource Default/Fallback Sound

When `SoundResource::load()` is called without a file path, a procedurally generated fallback sound is created. This serves as a recognizable placeholder when a sound file is missing.

**Generated sound characteristics:**
- Duration: ~250ms total
- Two short beeps with silence gap (100ms beep + 50ms silence + 100ms beep)
- First beep: Descending pitch sweep 880Hz → 440Hz
- Second beep: Ascending pitch sweep 440Hz → 660Hz
- Effects: Punchy ADSR envelope, subtle bit-crush (12-bit) for retro feel
- Normalized for consistent volume

**Code reference:** `SoundResource.cpp:load()` (no filepath overload)

**Why this design:**
- Recognizable as placeholder (classic alert pattern)
- Short and non-intrusive
- Harmonious frequencies (A5 → A4 → E5)
- Uses `WaveFactory::Synthesizer` for generation

## MusicResource Default/Fallback Music

When `MusicResource::load()` is called without a file path, a procedurally generated placeholder melody is created. This provides a pleasant looping background track when no music file is available.

**Generated music characteristics:**
- Duration: ~42 seconds (64 measures at 90 BPM)
- Seamless loop design (no fade in/out, matching start/end)
- Key: A minor with related progressions

**Musical structure (AABA form with variations):**
1. **Section A** (Sparse, Minimal) - Gentle intro
2. **Section A'** (Straight→Syncopated, Pad) - Building
3. **Section B** (Syncopated, Layered) - Development with counter-melody
4. **Section A** (Arpeggiated→Straight, Pad) - Return
5. **Section C** (Sparse, Layered) - Bridge (new color: F-G-Am-Em)
6. **Section A'** (Straight→Syncopated, Layered) - Rebuilding
7. **Section B'** (Syncopated, Layered) - Climax (turnaround: Dm-E-Am-Am)
8. **Section A** (Sparse, Minimal) - Loop point (matches intro)

**Chord progressions:**
- Section A: Am - F - C - G (classic pop)
- Section A': Am - F - C - E (tension variant)
- Section B: Dm - G - C - Am (ii-V-I-vi)
- Section B': Dm - E - Am - Am (turnaround)
- Section C: F - G - Am - Em (bridge)

**Rhythm styles (enum RhythmStyle):**
- `Straight`: Quarter notes
- `Syncopated`: Off-beat eighth note accents
- `Arpeggiated`: Broken chord patterns with passing tones
- `Sparse`: Half notes with ghost notes

**Texture styles (enum TextureStyle):**
- `Minimal`: Simple, clean (intro/outro)
- `Pad`: Sustained chord tones
- `Layered`: Rich harmonics with shimmer effects
- `Plucked`: Short attacks (unused currently)

**Dynamic variations:**
- Beat accents: {1.0, 0.7, 0.85, 0.75} per measure
- Pass intensity: 0.95 (first) → 1.05 (second)
- Counter-melodies on beats 2 and 4 (certain sections)
- Octave harmonies on beats 1 and 3

**Effects applied:**
- Chorus (0.7 rate, 6.0 depth, 0.25 mix)
- Reverb (0.35 room, 0.55 damping, 0.2 mix)
- Final normalization

**Code reference:** `MusicResource.cpp:load()` (no filepath overload, lines 97-550)

**Why this design:**
- Long enough to not feel repetitive (~42s loop)
- Musically interesting with varied textures and rhythms
- Seamless loop (end matches beginning)
- Uses full `WaveFactory::Synthesizer` capabilities

## TrackMixer (Jukebox)

Music playlist manager with full playback control. Observable for state change notifications.

### Key Features
- Playlist management: `addToPlaylist()`, `clearPlaylist()`, `playlist()`
- Playback control: `play()`, `pause()`, `resume()`, `stop()`
- Navigation: `next()`, `previous()`
- Seek: `seek(float position)` - Jump to position in seconds
- Progress: `currentPosition()`, `currentDuration()` - For UI progress bars
- Shuffle mode: `enableShuffle(bool)`, `isShuffleEnabled()`
- Loop mode: `setLoopMode(bool)`, `isLoopEnabled()`
- Cross-fade: `enableCrossFader(bool)` - Smooth transitions between tracks
- Volume: `setVolume(float)`, `volume()`

### State Management (UserState enum)
See `TrackMixer.hpp:UserState`
- `Stopped` - No playback
- `Playing` - Active playback
- `Paused` - Paused (source retained)

### Shuffle Implementation
- Uses `std::shuffle` with `std::mt19937` for randomization
- Maintains `m_shuffleOrder` vector of indices
- `generateShuffleOrder()` regenerates on enable or playlist change
- `next()`/`previous()` respect shuffle order when enabled

### Observable Notifications
TrackMixer notifies observers on state changes:
- `MusicPlaying` - Playback started
- `MusicPaused` - Playback paused
- `MusicStopped` - Playback stopped
- `MusicFinished` - Track ended (auto-advances if more in playlist)

### Code References
- `TrackMixer.hpp:previous()` - Navigate to previous track
- `TrackMixer.hpp:seek()` - Seek to position (uses `Source::setPlaybackPosition()`)
- `TrackMixer.hpp:currentPosition()` - Get current position via `Source::playbackPosition()`
- `TrackMixer.hpp:currentDuration()` - Get duration via `MusicResource::duration()`
- `TrackMixer.hpp:enableShuffle()` - Toggle shuffle mode
- `Source.hpp:playbackPosition()` - OpenAL `AL_SEC_OFFSET` getter
- `Source.hpp:setPlaybackPosition()` - OpenAL `AL_SEC_OFFSET` setter
- `MusicResource.hpp:duration()` - Returns `Wave::seconds()`

### Console Commands (Remote Console, TCP 7777)

Bound in `TrackMixer.console.cpp`. Prefix with the engine service path, e.g.:
`Core.AudioManagerService.TrackMixerService.<command>(<args>)`.

| Command | Arguments | Purpose |
|---------|-----------|---------|
| `play` | `[trackName]` | Resume if paused, else play the given track (exact resource name) or start the playlist. |
| `pause` | — | Pause current playback. |
| `stop` | — | Stop current playback. |
| `next` / `previous`,`prev` | — | Playlist navigation. |
| `volume`,`vol` | `[0..100]` | Get or set mixer volume. |
| `shuffle` | `[on\|off]` | Get or toggle shuffle. |
| `loop` | `[on\|off]` | Get or toggle loop mode. |
| `crossfade` | `[on\|off]` | Get or toggle crossfade transitions. |
| `seek` | `[seconds]` | Get or set playback position. |
| `status` | — | Overview of mixer state (volume, modes, index, position). |
| `nowPlaying`,`np` | — | **Atomic current-track query.** Returns title (filename fallback for MIDI/untagged files), artist, position, index. |
| `playlist`,`pl` | `[clear\|add <name>\|play <index>]` | List playlist or modify it. |

**`nowPlaying` rationale:** `status` only shows the playlist index (`N/32`), not the track identity. `nowPlaying` returns it atomically so scripts/AI agents don't race against shuffle-driven track changes (short MIDI + crossfade can switch tracks in ≈1 s). Output format is stable and one-block parseable.

### m_musicIndex Invariant (play(track) sync)

`play(const std::shared_ptr<MusicResource> &)` synchronizes `m_musicIndex` with the track's position in the playlist on every invocation. Before the fix, only `playIndex()`/`next()`/`previous()` updated the index, so a direct console call `play("Kyrandia2/Zanthia's Hut")` would leave `m_musicIndex` stale — producing wrong output from `nowPlaying`, `status`, `currentPosition`, and `currentDuration` (all read `m_playlist[m_musicIndex]`).

- If `track` is present in `m_playlist` → `m_musicIndex` = that entry's index.
- If `track` is not in the playlist (played ad-hoc) → `m_musicIndex = SIZE_MAX` as sentinel. Read paths guard via `m_musicIndex >= m_playlist.size()`.
- Linear scan is intentional: `m_playlist` is typically small (< 100 entries) and shared_ptr equality is O(1) per compare.
- Code reference: `TrackMixer.cpp:play(track)`.

### PlaylistResource & Playlist Swapping

Playlists are first-class resources backed by JSON manifests in the dedicated `MusicPlaylists/` store (separate from the audio store `Musics/` to avoid any confusion with the JSON synthesis recipes living in `Musics/`).

**Manifest schema** (minimal, intentional):
```json
{
	"tracks": [
		"Kyrandia2/Ferry",
		"Kyrandia2/Swamp"
	]
}
```
The playlist name is derived from the filename stem (`Kyrandia2.json` → `Kyrandia2`). Track entries are `MusicResource` names resolvable via the `Musics` container.

**Engine API** (`TrackMixer::loadPlaylist(const std::shared_ptr<PlaylistResource> &)`):
1. Captures persistent intent: `wasPlaying = (userState() == UserState::Playing)`. This is deliberately NOT `isPlaying()` — MIDI rendering is async, so a source may not yet be emitting when the swap is requested. `userState()` captures "the user wants music", which is the correct trigger for auto-resume.
2. Resolves every track name against the `MusicResource` container BEFORE mutating state. Unknown entries are skipped with a warning.
3. If resolution yields zero tracks (manifest empty or all entries invalid), the call returns `false` and the previous playlist is left intact.
4. Otherwise: `stop()` → `clearPlaylist()` → repopulate → if `wasPlaying`, `playIndex(0)` on the new playlist.
5. MIDI tracks may take a few seconds to render; `nowPlaying` will read "No track playing" until the async render completes and the `onNotification` callback triggers actual playback. This is expected behavior, not a bug.

**Console commands**:
- `listPlaylists` / `lpl` — enumerate all playlist manifests discovered in `MusicPlaylists/`.
- `loadPlaylist <query>` / `lp <query>` — swap the playlist. Resolution: (1) exact name via `isResourceExists`, (2) fuzzy case-insensitive substring on available playlist names. First match wins.

**Autoload** (projet-alpha): `App/Music/AutoloadPlaylist` + `App/Music/DefaultPlaylist` settings. Replaces the previous hardcoded prefix scan in `Application.cpp`.

**Adding a new playlist**: drop a new `<Name>.json` file in `data-stores/MusicPlaylists/`. On next startup (or via `listPlaylists` after an engine-side rescan) it appears immediately — no code changes needed.

**Code references**:
- `PlaylistResource.hpp/.cpp` — resource class, JSON parsing
- `TrackMixer.cpp:loadPlaylist()` — swap logic with atomic semantics
- `TrackMixer.console.cpp` — `listPlaylists` and `loadPlaylist` command bindings
- `Resources/Manager.cpp` — registers `Playlists` container bound to `MusicPlaylists` store

### Play Command Fuzzy Fallback (console)

`Core.AudioManagerService.TrackMixerService.play(<query>)` resolves `<query>` via two steps:

1. **Exact resource lookup** — uses `Container::isResourceExists(query)` to guard against `getResource()`'s silent `Default` fallback, then retrieves the resource if it actually exists. This preserves the ability to play tracks that live in the store but aren't in the current playlist.
2. **Playlist substring fallback** — if step 1 misses, calls `TrackMixer::findPlaylistTrack(query)` which performs a case-insensitive substring match on each playlist entry's `name()`. First match wins; no fuzzy algorithm beyond `String::toLower` + `std::string::find`.

If both steps fail, the command returns a descriptive error. Rationale for two steps: AI/remote console workflows pass approximate titles ("Snowy" → "Kyrandia2/Snowy Bridge"), while scripts or the UI still benefit from exact-name paths when possible.

- Code reference: `TrackMixer.console.cpp:play` command binding, `TrackMixer.cpp:findPlaylistTrack`.

### MusicResource.title() Fallback Behavior

`MusicResource::title()` returns the resource name (filename-derived, e.g. `Kyrandia2/Ferry`) when no ID3/metadata title has been set. Applies to formats without tag support — notably **MIDI** (TagLib cannot read MIDI metadata) and any audio file whose title tag is empty or missing.

- Sentinel: `MusicResource::DefaultInfo` = `"Unknown"` (private constant).
- Check order: `m_title.empty()` → `name()`; `m_title == DefaultInfo` → `name()`; else `m_title`.
- Artist behavior is **unchanged** — returns `"Unknown"` when tag absent.
- Code reference: `MusicResource.hpp:title()`.
- Downstream impact: `TrackMixer.cpp:play()` notifications (`MusicPlaying`, `MusicSwitching`) now show meaningful names for MIDI tracks instead of `"Unknown"`.

## SoundfontResource (SF2 Sample Banks)

Resource class for loading SoundFont 2 (SF2) files used for high-quality MIDI rendering.

### Overview
- Loads SF2 files via TinySoundFont library
- Provides `tsf*` handle for use with `FileFormatMIDI`
- Integrated with Resources system (store: "SoundBanks")
- Fail-safe: neutral resource returns nullptr handle (falls back to additive synthesis)

### Usage

```cpp
// Load a SoundFont via Resources system
auto soundfonts = resources->container<SoundfontResource>();
auto sf2 = soundfonts->getResource("8realgs20");

// Check if valid
if (sf2->isValid()) {
    // Use with MIDI rendering
    FileFormatMIDI<int16_t> midi{Frequency::PCM48000Hz};
    midi.setSoundfont(sf2->handle());
    midi.readFile("song.mid", wave);
}

// Query presets
int count = sf2->presetCount();
std::string name = sf2->presetName(0);
```

### Resource Store
- Store name: `SoundBanks`
- File extension: `.sf2`
- Location: `data-stores/SoundBanks/`

### API
- `handle()` - Returns `tsf*` pointer (or nullptr if no SF2 loaded)
- `isValid()` - Returns true if a SoundFont is loaded
- `presetCount()` - Number of presets/instruments in the bank
- `presetName(int)` - Name of a preset by index

### Code References
- `SoundfontResource.hpp` - Resource class declaration
- `SoundfontResource.cpp` - Implementation with TSF_IMPLEMENTATION
- `FileFormatMIDI.hpp:setSoundfont()` - Pass SF2 handle for rendering
- `FileFormatMIDI.hpp:renderWithSoundfont()` - SF2-based MIDI rendering

### SF2 Rendering Features
- **Dynamic control events**: Pan (CC#10), Expression (CC#11), Sustain (CC#64), Volume (CC#7), Pitch Bend update in real-time
- **Modulation/Vibrato (CC#1)**: LFO at 5.5Hz, ±50 cents max depth, rendered in 64-sample chunks
- **Tempo map support**: Multiple tempo changes during playback handled correctly
- **Program changes**: Dynamic instrument switching during playback
- **Bank selection**: CC#0/CC#32 for Bank Select, 0 for melodic, 128 for drums (channel 10)
- **RPN support**: Pitch Bend Range (CC#100/101 + CC#6/38)
- **Aftertouch**: Channel Pressure (0xD0) simulated via expression
- **Voice allocation**: Pre-allocated 256 TSF voices for complex MIDI files
- See: [`Libs/WaveFactory/AGENTS.md`](../Libs/WaveFactory/AGENTS.md) for full MIDI feature list

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

## MusicResource MIDI/Soundfont Integration

MusicResource automatically loads soundfonts as dependencies for MIDI rendering.

### Architecture (v0.8.35+)

**Dependency-based approach** (no static globals):
1. `MusicResource::load()` queries `Core/Audio/MusicSoundfont` from settings
2. If configured and file is MIDI, loads `SoundfontResource` as dependency
3. MIDI rendering occurs in `onDependenciesLoaded()` once soundfont is ready
4. Fallback to additive synthesis if no soundfont configured/available

### Configuration

Setting key: `Core/Audio/MusicSoundfont` (default: empty)
```cpp
// Example: Configure soundfont in settings
settings.set<std::string>("Core/Audio/MusicSoundfont", "8realgs20");
```

### Loading Flow

```
MusicResource::load(filepath)
    ├─ Is MIDI file? (.mid/.midi)
    │   ├─ Yes → Query soundfont name from settings
    │   │   ├─ Soundfont configured → Add as dependency, store pending path
    │   │   │   └─ Return success (wait for dependency)
    │   │   └─ No soundfont → Load with additive synthesis
    │   └─ No → Standard audio loading (WAV, MP3, OGG)
    └─ onDependenciesLoaded()
        ├─ Pending MIDI? → renderPendingMidi() with soundfont
        └─ Create audio buffers
```

### Key Implementation Details

- **Complexity**: `DepComplexity::One` (single soundfont dependency)
- **Thread safety**: TSF mutex protects concurrent MIDI rendering
- **Fallback**: Additive synthesis if soundfont unavailable
- **No global state**: Each MusicResource manages its own soundfont reference

### Code References
- `MusicResource.cpp:load()` - Dependency setup for MIDI files
- `MusicResource.cpp:renderPendingMidi()` - SF2-based rendering
- `MusicResource.cpp:onDependenciesLoaded()` - Finalization with soundfont
- `MusicResource.hpp:m_soundfontDependency` - Soundfont reference
- `MusicResource.hpp:m_pendingMidiPath` - MIDI path awaiting dependency
- `SettingKeys.hpp:AudioMusicSoundfontKey` - Setting key constant
