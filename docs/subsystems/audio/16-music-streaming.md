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
- Seen in that stress run: 29 `[Error][WaveFactory::FileFormatMIDI] processIStream(), failed to render audio !`
  lines. They are MIDI renderings CANCELLED by the shutdown, logged as errors (the testbed, the playlist `Doom`
  loaded and the application shut down two seconds later: 31 lines; left to load for 60 s: none, and the tracks
  play) — a fault of emeraude-base, fixed there the same day (its wavefactory doc 07): the same shutdown then logs
  none.
- macOS (engine 6a033c23, Apple M6, MoltenVK, Apple clang 21 with `-Werror -Wthread-safety`, 2026-11-27): Release
  and Debug without a warning; the console scenario on `default` (play `AirWolf8`, `seek(40)`, the wrap in Loop
  mode at 5.51 s, the next track in Once mode at 3.48 s, pause, a seek while paused, resume, cross-fade, `next` ×3,
  `previous`, stop, play) — 0 underrun line, 0 `VUID-`, 0 error in OpenAL Soft's log. ThreadSanitizer, the mixer's
  stress: 20 250 calls, 347 positions read on a streaming track, 0 report.
- Windows (engine 6a033c23, RTX 3500 Ada, MSVC 19.44 `/W4 /WX`, the same day): Release and Debug without a
  warning; the same scenario with the same answers — 0 underrun line, 0 `VUID-`, 0 error in OpenAL Soft's log; the
  mix of 14 s of `AirWolf8`: no exact-zero stretch in any of the 22 windows of 0.5 s (the longest run of zeros:
  0.00 ms). The correlation with the track was not measured there.
- ⚠️ A position is counted from the END OF THE LOAD: `play()` of a track not loaded yet answers at once and the
  track starts when it is decoded (1.0 s for `AirWolf8` on Linux, 1.8 s on Windows, resampled from 44.1 kHz) — 14 s
  after the command the position reads 13.0 s on Linux, 13.3 s on macOS, 12.2 s on Windows.
- ⚠️ NOT measured: the memory OpenAL gives back (no build of before was kept for an A/B); a starved event thread
  (no underrun was provoked).

### The playlist is a list of names — two tracks decoded, not the whole playlist (2026-11-27)

Owner decision the same day (the alternative weighed: the music resource releasing and reloading its samples through
the CPU-copy mechanism of `docs/subsystems/resources/11-cpu-copy-release.md`, the playlist's API unchanged).

- **A playlist entry is a NAME** (`TrackMixer::PlaylistEntry`). `loadPlaylist()` only checks each name against the
  music store (`isResourceExists()`): nothing is decoded. It used to ask the container for every track, which
  started every load — 367 MiB for 32 tracks, whatever was played.
- **The mixer holds a loaded music for three things only**: the track it plays (`m_playingMusic` and its stream),
  the track fading out (its stream) and the NEXT entry (`m_preloadedTrack`, asked from the store by
  `preloadNextTrack()` as soon as a track starts — `steppedIndex()` gives the entry `next()` would move to, the
  shuffle order included — so it is decoded when its turn comes). Plus a track it waits for (`m_loadingTrack`).
- **What nothing holds any more is forgotten by the container**: `closeStream()`, a change of the preloaded track,
  `clearPlaylist()` and `loadPlaylist()` raise `m_releaseRequested`; the event thread then calls
  `Container< MusicResource >::unloadUnusedResources()` OUTSIDE `m_stateAccess` (`releaseUnusedMusics()`). The
  store is asked outside that lock too (`resolveTrack()`): it takes its own lock and may start a load.
  ⚠️ It empties the container of EVERY music nothing holds, not only the mixer's: whoever wants a music kept decoded
  holds its `shared_ptr`.
- **A music the store does not hold** (a file dropped on the window: `Core` builds the resource itself) cannot be
  asked again: its entry keeps the resource (`PlaylistEntry::held`), decoded, for as long as the entry lives.
- API: `playlist()` and `Status::playlist` are the names (`std::vector< std::string >`); `Status::currentTrack` is
  gone (nothing read it); `addToPlaylist()` still takes a resource and keeps its name when the store holds it;
  `findPlaylistTrack()` asks the store for the entry found. `currentDuration()` is 0 while no source holds a track:
  the duration of an entry is not known before it is loaded. The console and the testbed's UI only list names.
- **A track asked for while the playing one ends is no longer lost.** `play(track)` of a track not loaded records it
  (`m_loadingTrack`); if the playing track reached its end before the load finished, the playlist advanced and the
  next entry superseded the request. The event thread now leaves the playlist where it is while a track is awaited.
  Reproduced before the fix: `play("Kyrandia2/Ferry")`, `seek(10.4)` (0.3 s before its end), `play("AirWolf8")` —
  five seconds later `nowPlaying` answered "Kyrandia2/Animated Scarecrow … Index: 1/32"; after: "Main Theme … 4 s /
  60.4981 s … outside the playlist". If that awaited track fails to load, nothing plays (as when nothing played).

Measured (Linux, Release, `default`, window-less, the playlist `Kyrandia2`, 32 MIDI tracks;
`Core.ResourcesManagerService.memoryCensus()`, the `MusicResource` row):

| Moment | Musics loaded | Bytes | Unused |
|---|---|---|---|
| Playlist loaded, stopped | none | — | — |
| Playing track 1 | 2 | 14.9 MB | 0 |
| After `next` | 2 | 15.1 MB | 0 |
| After `next` ×3 in two seconds | 2 | 12.2 MB | 0 |
| `playlistPlay(20)`, a far jump (the track starts about 1 s later) | 2 | 33.8 MB | 0 |
| Shuffle on, after two `next` | 2 | 52.8 MB | 0 |
| After a cross-faded `next` (the fade over) | 2 | 32.2 MB | 0 |
| `play("AirWolf8")`, a store music outside the playlist | 2 | 14.7 MB | 0 |
| Its natural end, back to the playlist | 2 | 14.9 MB | 0 |
| `stop` | 1 (the preloaded one) | 11.8 MB | 0 |
| `playlistClear` | none | — | — |

0 underrun line, 0 error in OpenAL Soft's log, 0 `VUID-`, 0 MIDI error. Gate `commit` clean. ThreadSanitizer, the
mixer's stress on this code: 29 335 calls, 0 report — ⚠️ only 9 positions read on a streaming track: with tracks
loaded on demand and four drivers calling `next` / `stop` at once, a track rarely gets to play; the run exercises the
loads, the preloads and the unloads far more than the streams (step 1's run: 781).
⚠️ NOT proven by a run: a file dropped on the window (`PlaylistEntry::held`) — no console command drops a file.

macOS (engine 9fdce41a, Apple M6, MoltenVK, Apple clang 21 with `-Werror -Wthread-safety`, 2026-11-27) and Windows
(RTX 3500 Ada, MSVC 19.44 `/W4 /WX`, the same day): Release and Debug without a warning; the same scenario with the
census at every step — 2 musics loaded while a track plays (the same byte counts as Linux for the same tracks, within
208 bytes on Windows), 1 after `stop`, 0 after `playlistClear` and after `loadPlaylist`, never an unused byte; the
lost request fixed ("Main Theme … outside the playlist"); 0 underrun line, 0 MIDI error, 0 `VUID-`, 0 error in
OpenAL Soft's log. macOS, ThreadSanitizer, the mixer's stress: 22 460 calls, 17 positions read on a streaming track,
0 report. (With no music loaded the census keeps a `MusicResource` row holding zeros.)

### Limits

- Two or three tracks stay decoded in full (the longest of this playlist: 30 MB). Decoding on the fly from the file
  would bring that down to the ring alone; it was weighed and NOT decided (incremental decoders in emeraude-base, file
  reading off the mixer's thread, no answer for MIDI and script musics). No item is open for it.
- A track is decoded again each time its turn comes (a MIDI is rendered again): the preload hides it in a normal
  playback, a far jump waits for it.
- A track that finishes loading after the mixer let it go stays in the container until the next release request.
- A seek on a paused track starts the source and pauses it at once: a few milliseconds may be heard.
- A music can no longer be played as a positioned sound by a scene source. Nothing did.
