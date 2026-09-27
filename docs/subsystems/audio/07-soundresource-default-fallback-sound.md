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
