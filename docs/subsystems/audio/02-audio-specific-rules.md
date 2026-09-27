## Audio-Specific Rules

### Coordinate Convention
- **Y-UP mandatory** in Audio abstraction (`+Y` up, `-Z` forward — which is also OpenAL's own convention, so `AL_ORIENTATION` takes the engine vectors verbatim)
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
