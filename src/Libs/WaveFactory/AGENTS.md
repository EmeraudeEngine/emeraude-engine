# WaveFactory - Audio Synthesis and Processing

Context for developing audio manipulation in Emeraude Engine.

## Module Overview

**Audio foundation** - Provides audio loading, procedural generation, effects processing, and format conversion. All audio operations in the engine use WaveFactory.

## Architecture (Separation of Concerns)

### Core Classes

**Wave<T>** - Audio data container (template)
- Stores samples in `std::vector<precision_t>`
- Supports any arithmetic type (int16_t default, float for processing)
- File I/O via libsndfile (int16_t specialization)
- See: `Wave.hpp`

**Synthesizer<T>** - Sound generation (creates new sounds)
- Takes a `Wave<T>&` reference, generates directly into it
- **Mono-only**: Works exclusively with mono audio for performance
- **Region system**: `setRegion(offset, length)` / `resetRegion()` for targeting specific portions
- Noise: `whiteNoise()`, `pinkNoise()`, `brownNoise()`, `blueNoise()`
- Waveforms: `sineWave()`, `squareWave()`, `triangleWave()`, `sawtoothWave()`
- Special: `pitchSweep()`, `noiseBurst()`
- Envelopes: `applyADSR()`, `applyFadeIn/Out()`
- Modulation: `applyVibrato()`, `applyTremolo()`, `applyRingModulation()`
- Filters: `applyLowPass()`, `applyHighPass()`
- Guitar effects: `applyDistortion()`, `applyOverdrive()`, `applyFuzz()`
- Modulation FX: `applyChorus()`, `applyFlanger()`, `applyPhaser()`
- Delay FX: `applyDelay()`, `applyReverb()`
- Dynamic FX: `applyWahWah()`, `applyAutoWah()`, `applyCompressor()`, `applyNoiseGate()`
- Lo-fi FX: `applyBitCrush()`, `applyPitchShift()`, `applySampleRateReduce()`
- Utilities: `mix()`, `reverse()`, `normalize()`
- See: `Synthesizer.hpp`

**SFXScript<T>** - JSON-based procedural sound effects
- Parses JSON definitions for multi-track audio generation
- Creates one mono `Synthesizer` per track, then interleaves for stereo output
- **Processing order per track**: `preInstructions` → `regions` → `instructions`
- `preInstructions`: Applied first on full track (generators like noise, waveforms)
- `regions`: Applied to specific portions (modifiers, localized effects)
- `instructions`: Applied last on full track (post-processing)
- `finalInstructions`: Applied to all tracks uniformly before interleaving
- Multi-channel via track interleaving, not per-sample channel loops
- See: `SFXScript.hpp:processTrack()`

**Processor** - Sound transformation (modifies existing sounds)
- Works internally in float precision via `dataConversion()`
- Structural: `trim()`, `crop()`, `pad()`, `concat()`, `split()`
- Channels: `mixDown()`, `toStereo()`, `extractChannel()`, `swapChannels()`
- Resampling: `resample()` (via libsamplerate, SRC_SINC_BEST_QUALITY)
- Analysis: `getPeakLevel()`, `getRMSLevel()`, `getDuration()`, `detectSilence()`
- Quality: `normalize()`, `convertBitDepth()`
- See: `Processor.hpp`, `Processor.cpp`

**dataConversion<In,Out>()** - Type conversion helper
- Converts Wave between precision types (int16_t <-> float)
- Handles normalization automatically
- See: `Wave.hpp:dataConversion()`

### File I/O Classes

**FileIO** - Dispatcher by extension
- Delegates to appropriate FileFormat class based on file extension
- See: `FileIO.hpp`

**FileFormatInterface** - Abstract base for audio formats
- See: `FileFormatInterface.hpp`

**FileFormatSNDFile** - libsndfile wrapper
- Supports WAV, FLAC, OGG, and other formats via libsndfile
- See: `FileFormatSNDFile.hpp`

**FileFormatJSON** - Procedural audio via SFXScript
- See: `FileFormatJSON.hpp`

**FileFormatMIDI** - MIDI file parser with synthesized audio output
- See: `FileFormatMIDI.hpp`

## Usage Patterns

### Generate a Mono Sound

```cpp
Wave<int16_t> wave;
Synthesizer synth{wave, sampleCount, Frequency::F44100};  // Always mono
synth.sineWave(440.0F, 0.5F);
synth.applyADSR(0.01F, 0.1F, 0.7F, 0.2F);
```

### Multi-Track Stereo via JSON

```json
{
    "duration": 1000,
    "channels": 2,
    "tracks": [
        {
            "instructions": [
                { "type": "sineWave", "frequency": 440.0, "amplitude": 0.5 },
                { "type": "applyADSR", "attack": 0.01, "decay": 0.1, "sustain": 0.7, "release": 0.2 }
            ]
        },
        {
            "instructions": [
                { "type": "sineWave", "frequency": 554.37, "amplitude": 0.5 },
                { "type": "applyADSR", "attack": 0.01, "decay": 0.1, "sustain": 0.7, "release": 0.2 }
            ]
        }
    ],
    "finalInstructions": [
        { "type": "normalize" }
    ]
}
```

Creates one Synthesizer per track, interleaves tracks for stereo output.

### Process Existing Sound

```cpp
Wave<int16_t> loaded;
loaded.readFile("sound.wav");
Processor processor{loaded};
processor.mixDown();      // Stereo -> Mono
processor.resample(Frequency::F44100);
processor.toWave(loaded); // Write back
```

### Load MIDI File

```cpp
// Via FileIO (automatic dispatch) - additive synthesis
Wave<int16_t> wave;
WaveFactory::FileIO::read("music.mid", wave, Frequency::PCM48000Hz);

// Direct usage - additive synthesis
FileFormatMIDI<int16_t> midiFormat{Frequency::PCM48000Hz};
midiFormat.readFile("music.mid", wave);
```

### Load MIDI with SoundFont (SF2)

```cpp
// High-quality rendering with SF2 sample bank
// Note: tsf* handle typically comes from Audio::SoundfontResource

Wave<int16_t> wave;
FileFormatMIDI<int16_t> midiFormat{Frequency::PCM48000Hz};
midiFormat.setSoundfont(tsfHandle);  // tsf* from SoundfontResource::handle()
midiFormat.readFile("music.mid", wave);

// Via FileIO with SF2
WaveFactory::FileIO::read("music.mid", wave, Frequency::PCM48000Hz, tsfHandle);
```

## Technical Details

### Noise Types (Spectral Characteristics)
- **White**: Flat spectrum, equal energy per frequency
- **Pink**: -3 dB/octave (Voss-McCartney algorithm), natural sounds
- **Brown**: -6 dB/octave (random walk), deep rumbles
- **Blue**: +3 dB/octave (high-pass filtered white), dithering

### Mono-Only Synthesizer Design

The `Synthesizer` class works exclusively with mono audio:

**Rationale:**
- Simpler, more performant code (no channel loops)
- Direct sample indexing (`data[sampleIndex]` vs `data[sampleIndex * channels + channel]`)
- Multi-channel handled at higher level by `SFXScript`

**Multi-channel workflow:**
1. JSON defines `"channels": 2` and multiple `"tracks"`
2. `SFXScript` creates one mono `Synthesizer` per track
3. Each track generates into its own mono buffer
4. Tracks are interleaved into final multi-channel output
5. `finalInstructions` applied to mixed result

### FileFormatMIDI Details

**Supported MIDI features:**
- Format 0 and 1 (single/multi-track)
- Note On/Off events with velocity
- Tempo changes (meta event 0x51)
- End of Track (meta event 0x2F) - proper parsing termination
- Variable-length delta times
- Running status compression
- **Pitch Bend (0xE0)**: Real-time pitch modification per channel
- **CC#1 (Modulation)**: Vibrato effect (LFO-based in TSF mode)
- **CC#7 (Volume)**: Channel volume control
- **CC#10 (Pan)**: Stereo positioning per MIDI channel (0=left, 64=center, 127=right)
- **CC#11 (Expression)**: Dynamic volume control
- **CC#64 (Sustain)**: Sustain pedal
- **CC#74 (Filter Cutoff)**: Brightness control
- **CC#92 (Tremolo)**: Amplitude modulation
- **Program Change (0xC0)**: Dynamic instrument selection per channel
- **Channel Pressure (0xD0)**: Aftertouch affecting whole channel
- **Polyphonic Key Pressure (0xA0)**: Per-note aftertouch (captured, limited TSF support)

**Advanced controllers (SF2 mode):**
- **CC#0, CC#32**: Bank Select MSB/LSB
- **CC#6, CC#38**: Data Entry MSB/LSB (for RPN)
- **CC#39, CC#42, CC#43**: Fine LSB controllers (Volume, Pan, Expression)
- **CC#98, CC#99**: NRPN LSB/MSB
- **CC#100, CC#101**: RPN LSB/MSB (Pitch Bend Range)
- **CC#120**: All Sound Off
- **CC#121**: Reset All Controllers
- **CC#123**: All Notes Off

**Output:**
- **Stereo output** with per-channel pan positioning
- Constant-power panning for natural sound

**Instrument families (waveform mapping):**
| Family | Programs | Waveform | ADSR |
|--------|----------|----------|------|
| Piano, Strings, Ensemble, Pads, Pipe | 0-7, 40-55, 72-79, 88-95 | Sine | Varied |
| Organ, Synth Lead, Chromatic | 8-23, 80-87 | Square | Sustained |
| Guitar, Bass | 24-39 | Sawtooth | Plucked |
| Brass, Reed | 56-71 | Triangle | Wind |
| Percussive, SFX | 112-127 | Noise burst | Short |
| **Channel 10** | Any | Noise burst | Percussion |

**SoundFont (SF2) Support:**
- Optional SF2 sample-based rendering via TinySoundFont
- `setSoundfont(tsf*)` to enable high-quality rendering
- Falls back to additive synthesis if no SF2 provided
- See: `Audio/SoundfontResource` for SF2 loading

**Dynamic Control Events (TSF mode):**
- Pitch Bend, Volume, Expression, Sustain, Pan update in real-time during playback
- Events processed via unified timeline (`TimelineEvent` struct)
- **Modulation/Vibrato**: LFO at 5.5Hz, ±50 cents max depth, scaled by CC#1 value
- **Adaptive chunked rendering**: 4096-sample chunks normally, 64-sample when vibrato active
- **Program Change**: Dynamic instrument switching during playback
- **Bank Select + RPN**: Full MIDI bank/preset selection support
- Pre-allocated 256 TSF voices for complex MIDI files
- See: `FileFormatMIDI.hpp:renderWithSoundfont()`

**Tempo Map:**
- Multiple tempo changes during playback properly handled
- Tempo events (`TempoEvent`) stored and sorted by tick
- Time-to-sample conversion via `ticksToSamplesWithTempoMap()`
- See: `FileFormatMIDI.hpp:TempoEvent`, `FileFormatMIDI.hpp:ticksToSamplesWithTempoMap()`

**Rendering Pipeline (SF2 mode):**
- Float accumulator buffer for lossless rendering before normalization
- Post-rendering normalization with 5% headroom (target peak = 0.95)
- Adaptive chunk sizes: 4096 samples normally, 64 samples when vibrato active
- Direct rendering into accumulator (no intermediate buffer allocation)
- Maximum duration limit: 30 minutes to prevent memory exhaustion
- See: `FileFormatMIDI.hpp:renderWithSoundfont()`

**Robustness:**
- **EOF protection**: `readVariableLength()` guards against truncated files to prevent infinite loops
- **NaN guards**: All output samples checked with `std::isfinite()` before writing
- **Invalid frequency handling**: `Wave::seconds()`/`milliseconds()` return 0.0F when frequency is Invalid
- See: `FileFormatMIDI.hpp:readVariableLength()`, `Wave.hpp:seconds()`

**Limitations:**
- No SMPTE time division
- No Reverb/Chorus effects (CC#91/93 - TSF limitation)
- Polyphonic Key Pressure captured but not applied (TSF limitation)
- No streaming mode (full pre-rendering to buffer)
- Truncated MIDI files may produce incomplete audio (graceful degradation)

**Note rendering:**
- Waveform selected by instrument family
- ADSR envelope adapted to instrument type
- Polyphony: additive mixing with saturation protection
- Stereo panning: constant-power pan law based on CC#10 per channel
- Auto-normalization to prevent clipping
- Note 69 (A4) = 440 Hz reference

## Code References

| File | Description |
|------|-------------|
| `Wave.hpp` | Audio data container, file I/O |
| `Synthesizer.hpp` | All generation/effects (header-only template) |
| `SFXScript.hpp` | JSON parser for procedural sound effects |
| `Processor.hpp/.cpp` | Transformation operations |
| `Types.hpp` | Channels, Frequency enums |
| `FileIO.hpp` | File format dispatcher |
| `FileFormatInterface.hpp` | Abstract base class |
| `FileFormatSNDFile.hpp` | libsndfile wrapper |
| `FileFormatJSON.hpp` | JSON procedural audio |
| `FileFormatMIDI.hpp` | MIDI file parser with SF2 support |

## Critical Attention Points

- **Synthesizer is mono-only**: Do not add channel loops to Synthesizer methods
- **Multi-channel via SFXScript**: Use JSON tracks for stereo/multi-channel
- **Region system**: Use `setRegion()`/`resetRegion()` for sub-buffer operations
- **Type conversion**: Use `dataConversion<>()` for int16_t <-> float conversion
- **Performance**: Synthesizer methods are optimized for single-channel processing
- **MIDI End of Track**: Always handle meta event 0x2F to properly terminate parsing. See: `FileFormatMIDI.hpp:parseTrack()`
- **MIDI Tempo Map**: Use `ticksToSamplesWithTempoMap()` for accurate timing with tempo changes. See: `FileFormatMIDI.hpp:ticksToSamplesWithTempoMap()`
- **MIDI EOF Safety**: `readVariableLength()` must check for EOF to prevent infinite loops on truncated files. See: `FileFormatMIDI.hpp:readVariableLength()`
- **Wave Invalid Frequency**: Always check frequency validity before computing durations. `seconds()`/`milliseconds()` return 0.0F for invalid frequency.

## External Dependencies

- **libsndfile**: Audio format loading (WAV, FLAC, OGG, etc.)
- **libsamplerate**: High-quality resampling (SRC_SINC_BEST_QUALITY)
- **TinySoundFont**: SoundFont 2 (SF2) sample-based MIDI rendering (header-only, MIT license)
