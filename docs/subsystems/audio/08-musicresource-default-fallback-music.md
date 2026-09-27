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
