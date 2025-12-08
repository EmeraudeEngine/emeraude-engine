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
// Via FileIO (automatic dispatch)
Wave<int16_t> wave;
WaveFactory::FileIO::read("music.mid", wave, Frequency::PCM48000Hz);

// Direct usage
FileFormatMIDI<int16_t> midiFormat{Frequency::PCM48000Hz};
midiFormat.readFile("music.mid", wave);
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
- **CC#1 (Modulation)**: Vibrato effect
- **CC#7 (Volume)**: Channel volume control
- **CC#10 (Pan)**: Stereo positioning per MIDI channel (0=left, 64=center, 127=right)
- **CC#11 (Expression)**: Dynamic volume control
- **CC#64 (Sustain)**: Sustain pedal
- **CC#74 (Filter Cutoff)**: Brightness control
- **CC#92 (Tremolo)**: Amplitude modulation
- **Program Change**: Instrument selection per channel (General MIDI mapping)

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

**Limitations:**
- No SMPTE time division
- Basic waveforms (no wavetable/sampling)
- No SoundFont (SF2) support yet (architecture is ready for future integration)

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
| `FileFormatMIDI.hpp` | MIDI file parser |

## External Dependencies

- **libsndfile**: Audio format loading (WAV, FLAC, OGG, etc.)
- **libsamplerate**: High-quality resampling (SRC_SINC_BEST_QUALITY)

## Critical Attention Points

- **Synthesizer is mono-only**: Do not add channel loops to Synthesizer methods
- **Multi-channel via SFXScript**: Use JSON tracks for stereo/multi-channel
- **Region system**: Use `setRegion()`/`resetRegion()` for sub-buffer operations
- **Type conversion**: Use `dataConversion<>()` for int16_t <-> float conversion
- **Performance**: Synthesizer methods are optimized for single-channel processing
- **MIDI End of Track**: Always handle meta event 0x2F to properly terminate parsing. See: `FileFormatMIDI.hpp:parseTrack()`

## Future Extensibility

**SoundFont (SF2) Integration:**
The MIDI rendering architecture is modular and ready for SF2 sample-based playback:
- Parsing and rendering are separated (`parseTrack()` → `renderToWave()`)
- Sample generation is isolated in `Synthesizer::generateWaveformSample()`
- To add SF2: replace waveform generation with sample lookup from SoundFont bank
- Recommended library: TinySoundFont (header-only, MIT license)
