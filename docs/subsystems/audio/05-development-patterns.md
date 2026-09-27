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
