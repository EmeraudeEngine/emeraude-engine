## The listener's medium — the sound under a sea (2026-11-26)

### Why

Owner (2026-11-23, with the view from under the sea): under the water the sound is muffled. His decisions of
2026-11-26: every sound of the WORLD is muffled and reverberates, the music is left alone; the change follows the
listener's DEPTH (a continuous function, never a switch: a listener bobbing at the surface must not chatter); the
reference is what one hears with the head under water (almost no treble, a little less level, a short dense
reverberation), not a film's.

### The contract

- **`Audio::ListenerMedium`** (`Audio/ListenerMedium.hpp`): what the medium around the listener does to every sound —
  the gain of the direct path, its gain at high frequencies, how much of the medium's reverberation is heard, the
  speed of sound. The default is the open air. The audio side knows no water: it applies a medium already blended.
- **`Audio::Manager::setListenerMedium()`** (any thread) applies it to every source of the POOL — the scene's
  emitters and their one-shot voices, the world sounds, the ambience. The track mixer's two sources (the music) and
  the manager's default source (interface sounds) are not pooled: untouched by construction. A medium equal to the
  last one costs a comparison; an invalid one is refused (traced). `listenerMedium()` reads it back.
  **A direct call, on purpose** (owner decision 2026-11-26, the Observer pattern weighed): the manager owns its
  sources and sets them itself. A notification the sources would listen to gives the same sound, costs a
  notification per source and per cycle while the listener crosses the surface (the medium is a continuous state,
  not an "entered / left" event), and must be emitted outside the pool's lock. It becomes the right choice the day
  sources OUTSIDE the pool have to follow the medium.
- **A source has ONE direct filter in OpenAL.** A user's own (`Source::enableDirectFilter()`: the ambience) and the
  medium's are multiplied: `Filters::Abstract::response()` answers a filter's three gains (whole, low shelf at
  250 Hz, high shelf at 5 kHz), the product goes into a band-pass the source owns (`Source::refreshDirectFilter()`).
  OpenAL copies a filter's values when it is attached: the same filter object is attached again at each change.
  The source's filter state has its own lock (a user sets its filter while the manager applies the medium).
- **The reverberation** is one EAX reverb in one shared effect slot of the manager (`createListenerReverb()`), set
  from Creative Labs' "underwater" preset (`EFX_REVERB_PRESET_UNDERWATER`, `AL/efx-presets.h`, shipped with OpenAL
  Soft): the one medium the engine has so far. Every pooled source is sent to it on its LAST auxiliary send, only
  while the medium asks for some (a slot nothing is sent to costs nothing); the level is the slot's gain.
  A device without an auxiliary send has no reverberation, the gains still apply.
- **What the medium does to a source outlives `Source::resetToDefaults()`**: it is the listener's, not a user's; a
  source handed out by the pool already carries it.
- **Lifetime**: the slot and the effect are released after the sources (`Manager::releaseSources()`): an effect slot
  is deleted once nothing is sent to it.

### Who decides — `Scene::updateSeaSound()` (logic thread, every cycle)

- The depth of the scene's primary audio device under the sea (`SeaLevelInterface::getDepthAt()`) gives an immersion:
  `clamp(depth / transitionDepth + 0.5, 0, 1)` — centred on the level, 0 in the air, 1 fully under. The scene's
  `listenerImmersion()` answers it.
- The sea's `SeaSoundSettings` (the family level, `Renderable::AbstractSea`: every sea) are the values fully under:
  `gain` 0.6, `gainHF` 0.02, `reverb` 0.5, `transitionDepth` 0.25 m. The owner heard them on 2026-11-26 (projet-alpha
  `water-world` on Linux, a looped wind on the player's head, diving and surfacing in free flight) and kept them as
  they were. The gains are blended in decibels (`value ^ immersion`, floored at −60 dB on the way), the reverberation and
  the speed of sound (the environment's toward 1480 m/s) linearly.
- A scene without a sea states the open air each cycle; a scene suspended or destroyed gives the listener back to
  the air if it had put it under its sea (`releaseSeaSound()`).
- ⚠️ **The level is the sea's level AT REST**: `OceanResource::getLevelAt()` does not answer the waves yet (engine
  item ocean-fft-surface, step 6). In a swell the sound changes up to a wave's height away from what the picture
  shows. The sound follows by itself the day the sea answers its waves (owner decision 2026-11-26: the level at
  rest first).
- ⚠️ `Audio::Manager::setEnvironmentSoundProperties()` no longer owns the speed of sound alone: it keeps the medium's
  in step, and the scene states it again each cycle.

### The microphone states the listener every cycle

`Component::Microphone::processLogics()` now updates its outputs every cycle, like `Camera::processLogics()`. Before,
only `move()` did, and until 2026-11-26 `move()` was not called for every displacement: `setLocalCoordinates()` (a
node's and a static entity's) wrote the frame WITHOUT notifying the components nor the sub-nodes. Measured then: a
player repositioned from y = 60 to y = −8 by projet-alpha's `Act.setPosition` kept its listener at 60 until the head
turned, and its sound emitter stayed 18 m behind (−25 dB by distance alone, which looked like the filter). The setter
notifies since (scenes doc 11 § *Every displacement notifies*): `Component::SoundEmitter`, which follows through
`move()` only, is at 0.4 dB of the same placement followed by a head turn.

### Console

- `setSeaSound(gain, gainHF, reverb, transitionDepth)` on the scene service; `getSea()` answers them as `sound`, with
  `listenerImmersion`.
- `SoundEmitter.play(entity, component, sound, gain, loop)`: a sound of the Sounds store on an emitter (an unknown
  name is refused); `getState` gains `awaitingLoad`.
  Proven on a sound never requested (2026-11-26, `psycho`, 224 s, loaded in 3.6 s): `play` answers `awaitingLoad`
  true; `stop` 0.04 s later cancels the wait; two seconds after the journal says the sound is loaded the emitter is
  not playing, has no source and still names the sound; `replay` then plays it.

### Measured (projet-alpha `water-world`, Linux, Release, window-less, 2026-11-26)

The mix written to a file (`ALSOFT_DRIVERS=wave`), `modem-dialup` played on the player's own emitter, one launch per
condition, the same ten seconds of the sound compared band by band with a launch in the air. Two launches in the air
differ by up to 1.7 dB a band (the noise floor of the method). The sound holds almost nothing above 3.5 kHz: those
bands are not read.

| Condition | 250-500 Hz | 500-1000 Hz | 1-2 kHz | 2-3.5 kHz |
|---|---|---|---|---|
| Fully under (immersion 1), no reverberation | −2.7 dB | −7.7 dB | −14.5 dB | −21.9 dB |
| At the level (immersion 0.5), no reverberation | −0.7 dB | −1.0 dB | −1.9 dB | −7.0 dB |
| Fully under, reverberation 0.5 | +4.4 dB | −3.2 dB | −13.6 dB | −17.6 dB |

- Whole-band level: −38.9 dB in the air, −46.8 dB under without reverberation, about −40 dB with it.
- After `stop`: the air and the dry water fall to the file's floor (−100 dB) within 0.1 s; with the reverberation
  the level goes −60, −70, −80, −82, −85, −91, −99 dB over 0.7 s — the tail.
- `listenerImmersion`: 0 at y = +11.6 m, 0.5 at the level, 1 at −6.4 m. 0 `VUID-`; the only `[Error]` is the wave
  writer's own (no audio INPUT device in that mode).

### Limits

- Heard by the owner on Linux only. macOS verified on 2026-11-26 (Apple M6, MoltenVK, engine 04c5d417): the same
  five launches, immersions 0 / 0 / 0.5 / 1 / 1, 0 VUID, the tail about 0.6 to 0.7 s; fully under without
  reverberation −5.6 / −8.7 / −15.2 / −20.9 dB, mid-band −2.6 / −3.3 / −4.0 / −6.2, with reverberation
  +2.4 / −5.4 / −15.2 / −15.7 (two air launches within 0.4 dB). Above 2 kHz within about 2 dB of Linux; under
  1 kHz macOS attenuates 2 to 3 dB more, not explained. Windows verified the same day (RTX 3500 Ada, MSVC 14.44
  Release and Debug 0 warning): the same immersions, 0 VUID, the tail about 0.6 s; fully under without reverberation
  −5.0 / −7.7 / −14.6 / −21.8 dB, with it +3.6 / −3.3 / −14.4 / −17.1, mid-band −2.6 / −2.4 / −3.4 / −6.0. ⚠️ The
  mid-band reading agrees between macOS and Windows and sits about 2 dB under Linux's in the three lower bands: the
  Linux reading of that one condition is the odd one, not explained. AMD verified the same day (Radeon integrated
  graphics, RADV RENOIR, Mesa 26.0.8, the device forced through `ForceGPU` in a settings copy, engine fab5e37c): the
  same immersions, 0 VUID; fully under without reverberation −5.4 / −9.3 / −16.1 / −24.0 dB, with it
  +2.7 / −4.9 / −15.5 / −18.9, mid-band −2.7 / −1.8 / −2.9 / −7.5; the second air launch within 0.7 dB in the three
  lower bands and at −2.5 dB in the highest. The tail there falls 30 dB in about 0.45 s and then meets the file's
  floor: the 60 dB point is not reachable under water at this sound's level.
- ⚠️ **The mix written by the wave writer has holes — a trap of the method, not lost sound** (2026-11-26). The
  engine opens TWO output devices on the selected output: the audio manager's own, and the recorder's passthrough
  (`Audio::Recorder`: the game is mixed on a loopback device in 16 bits, a thread plays that mix on a second device).
  With `ALSOFT_DRIVERS=wave` both are wave writers on the SAME file (two descriptors on it, read in `/proc`), each
  writing at its own position: the manager's silent blocks overwrite a part of the passthrough's. Measured on Linux:
  32 to 41 % of the samples of a played sound are exact zeros, in runs of 1024 to 9216 samples on the file's
  1024-sample grid, for a continuous sound as well (`fx5`, `wind1`) while the emitter reports `playing`; the blocks
  that remain sit at their exact time in the sound (`wind1`: correlation 1.00, offset 0.000 s all along). A real
  output device mixes the two instead: nothing is lost there. Consequences: the band RATIOS above compare
  launches that all carry the holes (their spread from a launch to the next, up to 2.5 dB, comes from there);
  an absolute level read on such a mix is too low, and a correlation must use the blocks that hold sound.
  **A mix written WHOLE**: a settings copy with `Core/RushMaker/EnableAudio` false — the recorder is not started,
  the game is mixed on the manager's device, one descriptor on the file. Proven the same day (`wind1`, 6.84 s of
  play): no run of zeros, 0.02 % of zero samples. The measurements above were NOT taken that way.
- One medium for every source: a source that is itself under water, heard from the air, is not muffled; a source in
  the air heard from under water is muffled like one beside the listener (owner decision 2026-11-26: the listener's
  state first).
- A source's own effects (`Source::addEffect()`) keep to their channels, the sends before the last one (four sends
  asked a source since 2026-11-26: `06-critical-points.md`, *A source's effect channel IS its auxiliary send*). They
  are NOT muffled by the medium: a user's send carries the sound unfiltered. No user in the cascade today.
