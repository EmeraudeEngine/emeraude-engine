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
- Console: `getWorldSoundCount()` (read-only).

Measured (projet-alpha, Linux Release, test ports, 2026-10-24): citadel, 5 rockets in 0.4 s → 5 world blasts, released
as they end; 8 smoke grenades → their hisses (2-3 at once) now heard to the end; lighten-marbles, 14 marble explosions
as world sounds in a 6-pass sample. 0 `VUID-`, 0 refusal, no `SourceRequest` held at the audio shutdown.

### The emitter's own pitch (2026-10-28)

`SoundEmitter::setPitch(pitch)` / `pitch()`: the pitch of the emitter's OWN sound (an engine loop following its RPM),
applied at once when it plays and kept for its next play (`playAttachedSound()` sets it with the gain); clamped by the
source to [0.5, 2], a non-finite value refused (traced); `clear()` resets it to 1. The one-shot voices keep their own
pitch (`playOneShot()`). A pooled source comes back to its defaults when released (`Audio::Manager::releaseSource()`):
no pitch leaks from a former user. First user: a downstream car's engine loop.
