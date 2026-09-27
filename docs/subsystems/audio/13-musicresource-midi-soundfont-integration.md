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
