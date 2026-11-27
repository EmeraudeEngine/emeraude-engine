## SoundEmitter one-shot voices and the clean source pool (2026-10-23)

### Why

A `Scenes::Component::SoundEmitter` held ONE OpenAL source: `play()` replaced whatever it played. An actor shares one
emitter between its voice, its weapons, its landings and its pick-ups, so every new sound cut the previous one (a
groan cut the sword's swoosh; projet-alpha's player death cry, played with `replaceSound = false`, was simply dropped
when anything was still playing). The engine had no equivalent of Unity's `AudioSource.PlayOneShot()`.

### The contract (`SoundEmitter.hpp`)

- `playOneShot(sound, gain = 1, pitch = 1)` plays a sound ONCE on an extra voice layered over the emitter's own sound.
  It does not touch the own sound: `isPlaying()`, `setGain()`, `replay()`, `rewind()`, `attachedSound()` keep
  speaking of the own sound only.
- At most `MaxOneShotVoices` (4) voices per emitter. A voice whose sound ended is reused, else an empty slot asks the
  pool, else the next busy voice in turn is CUT for the new sound (the latest event is the one to hear).
- The voices follow the entity (`move()`), are paused / resumed / stopped with the emitter (`pause()`, `resume()`,
  `stop()`, `clear()`), released at a scene suspension (`onSuspend()`: a one-shot is not resumed at the wakeup) and in
  the destructor; `processLogics()` gives a finished voice back to the pool.
- **A one-shot is an event of NOW**: a sound not loaded yet is refused with a warning, never played later (that is
  `play()`'s behaviour, which waits for the load). Preload the sounds an actor fires.
- Refused (traced): a null sound, a non-finite gain or pitch. A pool with no free source drops the play (Debug trace):
  a saturation, not an error. The pitch is clamped by `Source::setPitch()` to [0.5, 2].
- Logic thread only (the emitter's own thread: `move()`, `processLogics()`).
- Console: `SoundEmitter.getState` answers `oneShots`, the voices still playing — the way an AI verifies a one-shot
  fired without hearing it.

### The pool hands out clean sources (`Manager::releaseSource()` → `Source::resetToDefaults()`)

A pooled source is shared by every user in turn and each user configures only what it relies on. `Ambience` leaves
its channels relative to the listener (the loop channel), pitched, with short reference / maximum distances and a
direct filter; a `SoundEmitter` that later drew one of those sources set only position, direction, velocity and gain —
it played relative, pitched and filtered. Since 2026-10-23 the pool's deleter (`releaseSource()`) stops the source,
removes its sound and puts back every OpenAL default before the source is available again (pitch 1, gain 1 and its
bounds, position / velocity / direction 0, absolute, reference distance 1, rolloff 1, maximum distance FLT_MAX, cone
360° / 360° / 0, no direct filter, no effect slot; with EFX air absorption 0 and the cone's outer HF gain 1). The
one-shots set a pitch per play: without the reset they would have spread that defect to every later user.

### Measured (projet-alpha, Linux, 2026-10-23, Release, test ports 7780)

- `citadel`, the car dropped 4 m (`setNodePosition`, 7.7 m/s at the ground): its emitter's `oneShots` went 0 → 1 → 0
  over ~0.25 s (one crash sound; 4 voices before the car aggregated its impacts per cycle, see projet-alpha
  `docs/subsystems/actor/08-8-actor-sounds.md`). Driving to 27 m/s: 0 voice (the wheels report no contact).
- `animation-debug`, the player among the four hunting paladins: the player's emitter layered 2-4 one-shots (the
  blades) with its groan as own sound, then the death cry once; a paladin struck by another's swing groaned (own
  sound) with its armour hit as a one-shot.
- 0 `VUID-`, 0 warning from `playOneShot`, a clean audio shutdown (no `SourceRequest` held at the termination).

### World sounds — `Scene::playSoundAt()` (2026-10-24)

A sound OWNED BY THE SCENE, played once at a world position: it outlives the entity that caused it (UE5's
`UGameplayStatics::PlaySoundAtLocation()`). projet-alpha's explosion kept its whole actor alive for its sound's tail;
its smoke grenade lost its ignition hiss at once (the grenade, and its emitter, went away the same cycle).
- `playSoundAt(position, sound, gain, pitch)`: at most `MaxWorldSounds` (16) voices (`m_worldSounds`, a
  `Base::Mutex`, any caller thread): a finished voice is reused, an empty slot asks the pool, else the next in turn is
  cut. Refused (traced) like a one-shot: null, not loaded (an event of now), non-finite.
- `processLogics()` gives the finished voices back each cycle; a suspension (`suspendAllEntities()`) and the
  destructor give them all back (never resumed: events of the moment).
- `Audio::Source::isFinished()` (neither playing nor paused) is the release test of both the emitter's one-shots and
  the world sounds.
- Console: `getWorldSoundCount()` (read-only); `playSoundAt(sound, x, y, z [, gain, pitch])` (2026-11-27) plays a
  sound of the Sounds store as a world sound — the name checked (an unknown one is refused, never the default
  sound), the sound loaded BEFORE the scene is entered (the scene does not play a sound still loading), a gain
  outside [0, 1], a pitch outside ]0, 4] or a coordinate that is not finite refused; it answers a warning when no
  world voice or no audio source was free.

### The scene's ambience from the console (2026-11-27)

`Audio::Ambience` (a scene's loop and its random sound effects) had no command: its loop's gain could not be tuned
while listening, and a measurement of another sound had to switch it off with a temporary probe.

- `getAmbience()` (read-only): `exists`, `state` (Stopped / Playing / Paused), `loop` (the resource name, or null),
  `loopGain`, `channelCount`, `soundCount`, `radius`.
- `setAmbienceLoop(sound, gain)`: the SAME sound as the current loop changes its gain alone, on the source that plays
  it (`Ambience::setLoopGain()`, new) — no cut; another sound stops the ambience, changes the loop and starts it
  again when it was playing (a paused ambience is left stopped); an empty name removes the loop
  (`Ambience::removeLoopSound()`, new). An unknown name and a gain outside [0, 1] are refused.
- `startAmbience()` / `stopAmbience()`: `startAmbience` answers a warning when the ambience has nothing to play.
- Every command runs under the scene's exclusive access (the logic thread updates the ambience).
- A loop whose sound is still loading when the ambience starts (or wakes up) is AWAITED: `Ambience::update()`, on
  the logic thread, tests the resource's status each cycle and starts it (`startAwaitedLoop()`); `stop()` and
  `suspend()` cancel the wait with the sources; a sound that failed to load is dropped with a warning. Until
  2026-11-27 the ambience observed the sound and its `LoadFinished` handler played from the LOADING thread, on a
  source `stop()` could have released (a null pointer) — read, never observed; and a loop still loading at a wake-up
  was never started. The ambience observes nothing any more.
  Proven (ThreadSanitizer, Linux, Debug, `default`, the process on two cores, a temporary trace in
  `startAwaitedLoop()`): `setAmbienceLoop("bigroom", 0.5)` then `startAmbience()` — the loop starts on the cycle
  after "bigroom is loaded"; a loop replaced or an ambience stopped before its sound ends loading starts nothing
  (`foudre`, `fx1`, `fx2` loaded after the fact); 0 report in five runs, four of them with ten sky switches loading
  at the same time.
- Measured (Linux, `water-world`, the mix written whole): the loop `wind1` over ONE whole period of it (6.84 s, so
  the phase does not matter) at the gains 1.0 / 0.5 / 0.25: −30.07 / −36.09 / −42.11 dBFS, −6.02 and −12.04 dB; the
  longest run of exact zeros around each change 0.02 ms (one sample): no cut. The answers of `getAmbience` follow
  every command; `tools/mcp-conformance.py`: 2140 checks passed, 0 failed.
  ⚠️ A loop whose level moves (the surf of `water-world`: 2.3 dB between two 5 s windows) cannot prove a gain change
  on two different stretches: measure whole periods, or the same stretch.
- macOS (Apple M6, MoltenVK, Apple clang 21) and Windows (RTX 3500 Ada, MSVC `/W4 /WX`), engine f06619d7,
  2026-11-27: Release and Debug without a warning, every answer and every refusal identical to Linux's,
  `tools/mcp-conformance.py` 2140 checks passed and 0 failed on both, 0 `VUID-`. The world sound's path measured
  there (doc 15 § the path of a world sound): listener in the air, source under against source in the air
  −10.7 / −12.1 / −23.2 dB on macOS (125-500 Hz, 500-2k, 2k-8k) and −10.7 / −12.1 / −23.1 / −40.0 dB on Windows,
  within 0.2 dB of Linux.

Measured (projet-alpha, Linux Release, test ports, 2026-10-24): citadel, 5 rockets in 0.4 s → 5 world blasts, released
as they end; 8 smoke grenades → their hisses (2-3 at once) now heard to the end; lighten-marbles, 14 marble explosions
as world sounds in a 6-pass sample. 0 `VUID-`, 0 refusal, no `SourceRequest` held at the audio shutdown.

### The emitter's own pitch (2026-10-28)

`SoundEmitter::setPitch(pitch)` / `pitch()`: the pitch of the emitter's OWN sound (an engine loop following its RPM),
applied at once when it plays and kept for its next play (`playAttachedSound()` sets it with the gain); clamped by the
source to [0.5, 2], a non-finite value refused (traced); `clear()` resets it to 1. The one-shot voices keep their own
pitch (`playOneShot()`). A pooled source comes back to its defaults when released (`Audio::Manager::releaseSource()`):
no pitch leaks from a former user. First user: a downstream car's engine loop.
