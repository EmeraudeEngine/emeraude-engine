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

- `Manager.cpp/.hpp` - Main manager (devices, activation, capture)
- `SoundResource.cpp/.hpp` - Sound loading and management (mono)
- `MusicResource.cpp/.hpp` - Music loading and management (stereo + streaming)
- `SoundfontResource.cpp/.hpp` - SoundFont 2 (SF2) sample bank loading
- `Source.cpp/.hpp` - OpenAL source abstraction (pool)
- `Speaker.cpp/.hpp` - Listen point abstraction (non-OpenAL, API consistency)
- `TrackMixer.cpp/.hpp` - Jukebox for playlist management (see TrackMixer section below)
- `Ambience.cpp/.hpp` - Ambient sounds (loop channel + random effects, State enum)
- `AmbienceChannel.hpp` - Individual channel for ambient sound effects
- `AmbienceSound.hpp` - Ambient sound configuration
- `Recorder.cpp/.hpp` - Audio recording from microphone
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
auto soundfonts = resources->container<S<oundfontResource>();
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
- See: `Libs/WaveFactory/AGENTS.md` for full MIDI feature list
