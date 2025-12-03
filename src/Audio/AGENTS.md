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
- Fail-safe pattern: Neutral audio resources on failure

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
- `Source.cpp/.hpp` - OpenAL source abstraction (pool)
- `Speaker.cpp/.hpp` - Listen point abstraction (non-OpenAL, API consistency)
- `TrackMixer.cpp/.hpp` - Jukebox for playlist management
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
