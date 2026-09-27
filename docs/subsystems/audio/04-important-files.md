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
- `@docs/coordinate-system.md` - Y-up convention (CRITICAL)
