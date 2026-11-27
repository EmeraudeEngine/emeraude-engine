## Music is streamed by the track mixer (2026-11-27)

### Why

A loaded track was held TWICE: decoded in the music resource (`MusicResource::m_localData`, 16-bit PCM) and copied
whole into OpenAL, one buffer per chunk, when its load ended (`onDependenciesLoaded()`); `Source::play()` then queued
every buffer of the track on the source. With a playlist of 32 tracks loaded up front, the memory census counted
367 MiB of PCM (2026-10-03) and OpenAL held about as much again (not measured). One track plays at a time.

### The contract

- **A music resource holds samples and metadata, no OpenAL object.** Its load no longer calls OpenAL at all.
- **The track mixer owns the playback of a music**: `TrackMixer::TrackStream`, one per source (A and B, for the
  cross-fade) — a ring of `StreamRingSize` = 8 OpenAL buffers created by `onInitialize()` and deleted by
  `onTerminate()`, and the cursor in the music they are filled from. With the default chunk
  (`Core/Audio/MusicChunkSize` = 8192 SAMPLES, so 4096 stereo frames) the ring holds about 0.68 s at 48 kHz, 256 KiB.
  A chunk is rounded down to whole frames.
- **The mixer's event thread feeds the rings** (`eventLoop()` → `serviceStream()`, every `EventPeriod` = 16 ms while a
  track is held or a fade runs; it sleeps otherwise): the buffers the source has played come back
  (`Source::unqueueProcessedBuffers()`), are refilled with the next chunks (`Buffer::feedData()`, a copy from memory —
  no file is read on that thread) and queued again (`Source::queueBuffers()`). Everything under `m_stateAccess`.
- **`Audio::PlayableInterface` is gone** (owner, 2026-11-27): with the music out of it, it had one implementer.
  `Source::play()` and `Audio::Manager::play()` take a `SoundResource`; a sound has one buffer
  (`SoundResource::buffer()`). A queue is a SERVICE of `Source` (`queueBuffers()`, `unqueueProcessedBuffers()`,
  `playQueue()`, `queuedBufferCount()`, `sampleOffset()` / `setSampleOffset()`, `detachBuffers()`), whose owner owns
  the buffers. If another kind of thing must play on a source one day, a contract is drawn again from two real cases.

### What the mixer does itself, since OpenAL only knows 0.7 s of the track

| | Before | Now |
|---|---|---|
| Position | `AL_SEC_OFFSET` over the whole queue | `streamPosition()`: the frame the stream was opened at + the frames of the buffers taken back + `AL_SAMPLE_OFFSET`, modulo the track |
| Seek | `AL_SEC_OFFSET` | `openStream(track, music, frame)`: the ring is emptied, filled from the chunk holding the frame, the source's cursor set to the remainder, the source started — and paused again when it was paused |
| Loop | `AL_LOOPING` | `fillStream()` wraps to the first chunk; the mode is read WHEN the end of the samples is reached, so `setPlayMode()` applies to the track being played. `AL_LOOPING` is off on a queue (a looping source never marks a buffer as processed) |
| End of a track | OpenAL Soft's source event (`AL_SOFT_events`) on OpenAL's thread | `serviceStream()`: the last chunk queued in Once mode (`exhausted`) and the source stopped with an empty queue. The track is freed (`m_playingTrack` = `None`) and the next one requested |
| Underrun | impossible | the source stopped while buffers remain: traced ("ran dry (underrun), restarted") and restarted |

- The OpenAL Soft event callback is no longer registered: the mixer has three threads, not four, and a playlist
  advances whatever the OpenAL implementation (without the extension it used to loop one track).
- `stop()` closes BOTH streams: a track fading out when the music was stopped went on playing at the gain the fade
  had left it.
- A cross-fade ends when the new track is at its gain AND the old stream is closed.

### Measured (Linux, Release, `default`, window-less, the mix written whole to a file, recorder off)

- The console scenario on the playlist `Kyrandia2` (MIDI tracks): `play`, the position read 3 s apart (+3.03 s), the
  natural end of track 1 (15.9 s) → track 2 at 3.52 s four seconds later; `pause` (position still), `seek(30)` while
  paused (30 s, still paused), resume (32.03 s two seconds later); cross-fade on, `next` ×3, `previous`, `stop`,
  `play`. 0 underrun line, 0 error in OpenAL Soft's log (`ALSOFT_LOGLEVEL=2`), 0 `VUID-`.
- The sound itself, `play("AirWolf8")` (an Ogg at 44.1 kHz, 60.5 s) against the same file decoded by ffmpeg to
  48 kHz, by normalized correlation of every 0.5 s window of the mix AT the position the continuity predicts:
  1.000 on the 25 windows after the start; `seek(40)` — 1.000 on 15 windows, the track at 39.98 s when the command
  was sent; Loop mode, `seek(58)` — 1.000 before the end and on the 10 windows after the wrap, the position shifted
  by 60.498 s, the track's length; Once mode, `seek(58)` — 1.000 to the end, then the first track of the playlist.
  ⚠️ Locate a window by the continuity, not by the correlation's maximum: the music repeats itself (a 5.05 s
  pattern) and the maximum lands on a louder repetition.
- ThreadSanitizer, the mixer's stress (`tools/stress/TrackMixerStress.cpp`, Debug, lavapipe, the playlist `Doom`,
  four threads calling every public method for 30 s plus the logic thread): 27 023 calls, 1 800 logic-thread reads,
  **781 positions read on a track that was streaming**, 0 report, 0 underrun line. The tool now counts those reads
  (`m_playingReads`): a run whose tracks never finish loading exercises no stream and proves nothing — its summary
  line must show a count above zero. (It also lacked `Audio/PlaylistResource.hpp`, which it used through another
  header.)
- The gate's `commit` profile: GCC Release and clang builds without a warning, clang-tidy, cppcheck and
  include-what-you-use clean on the touched files.
- ⚠️ Seen in that stress run and not looked into: 29 `[Error][WaveFactory::FileFormatMIDI] processIStream(), failed
  to render audio !` lines for tracks of the playlist `Doom` (a scratch configuration, no soundfont set).
- ⚠️ NOT measured: the memory OpenAL gives back (no build of before was kept for an A/B); a starved event thread
  (no underrun was provoked).

### Limits

- The decoded samples of every loaded track still stay in memory: the playlist loads every track up front (engine
  item `decoded-music-stays-resident`, the second step).
- A seek on a paused track starts the source and pauses it at once: a few milliseconds may be heard.
- A music can no longer be played as a positioned sound by a scene source. Nothing did.
