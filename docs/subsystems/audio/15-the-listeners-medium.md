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
only `move()` did, and `move()` is not called for every displacement: `setLocalCoordinates()` (a node's and a static
entity's) writes the frame WITHOUT notifying the components nor the sub-nodes (engine item
`node-setlocalcoordinates-notifies-nobody`). Measured: a player repositioned from y = 60 to y = −8 by projet-alpha's
`Act.setPosition` kept its listener at 60 until the head turned. ⚠️ `Component::SoundEmitter` still follows through
`move()` only: after such a displacement its source stays behind (18 m behind in the first measurement: −25 dB by
distance alone, which looked like the filter).

### Console

- `setSeaSound(gain, gainHF, reverb, transitionDepth)` on the scene service; `getSea()` answers them as `sound`, with
  `listenerImmersion`.
- `SoundEmitter.play(entity, component, sound, gain, loop)`: a sound of the Sounds store on an emitter (an unknown
  name is refused); `getState` gains `awaitingLoad`.

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

- Heard by the owner on Linux only; macOS and Windows not run; AMD untested.
- One medium for every source: a source that is itself under water, heard from the air, is not muffled; a source in
  the air heard from under water is muffled like one beside the listener (owner decision 2026-11-26: the listener's
  state first).
- ⚠️ `EffectSlot::enable()` writes fixed send numbers (0 without a filter, 1 with one) whatever channel
  `Source::addEffect()` was given: with a filter it would overwrite the reverberation's send on a device with two
  sends (engine item `effect-slot-ignores-its-send-channel`). No user in the cascade today.
