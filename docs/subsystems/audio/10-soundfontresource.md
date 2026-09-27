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
- See: [`Base/WaveFactory/AGENTS.md`](../../../dependencies/emeraude-base/src/WaveFactory/AGENTS.md) for full MIDI feature list
