## Critical Points

- **Device detection**: OpenAL-Soft latest can be finicky on laptops
- **Sound format**: MONO mandatory for 3D spatial positioning
- **Music format**: STEREO supported (no spatialization)
- **Thread safety**: OpenAL handles threading internally
- **Coordinate system**: Always Y-up in Audio API
- **`AL_ORIENTATION` takes the engine vectors VERBATIM.** OpenAL follows the OpenGL convention — right-handed, `+Y` up, `-Z` forward — which is exactly the engine world convention. `HardwareOutput::updateDeviceFromCoordinates()` therefore passes `forwardVector()` and `upwardVector()` unmodified. **Any negation added there is a bug, not an OpenAL quirk.**
  ⚠️ It passed `downwardVector()` until Aug 2026 — a Y-down compensation the flip should have removed, leaving the vertical axis of the audio field INVERTED. It survived that long because it is **invisible to every visual check**: stereo/HRTF panning disagrees with the picture with nothing on screen to show for it. To re-verify, play a positional sound ABOVE the listener and confirm which way it comes from.
- **Resource integration**: Never load directly, go through Resources
- **Every OpenAL object is released while its context and device exist** (Ave Robustus II, 2026-10-16). An OpenAL
  source belongs to its context, a buffer to its device; the recorder's game context (on its loopback device) is the
  current one when the recorder is up. `Audio::Manager::onTerminate()` therefore releases, in order: the default source
  and the source pool (`releaseSources()`), the other sub-services (the track mixer deletes its two sources), the
  buffers of every sound and music resource (`releaseResourceBuffers()` → `PlayableInterface::releaseBuffers()` →
  `Buffer::destroy()`: the resource manager, which holds them, terminates AFTER the audio manager), then the recorder,
  then its own context and device.
  - Before: `AL_INVALID_NAME` at `GlobalReleaseFlush` at every shutdown; with `ALSOFT_LOGLEVEL=3`
    `ALSOFT_LOGFILE=<file>`, OpenAL Soft reported "254 Sources not deleted" (the fault-injection tool) and "187 Buffers
    not deleted" (projet-alpha with music). After: none of the three. Use that log to re-verify.
  - A `SourceRequest` still held at the audio shutdown is a contract fault (owner decision 2026-10-16): traced, aborted
    in Debug; in Release the pool is kept so no source is destroyed under its holder.
  - `Buffer` and `Source` own their OpenAL name: neither copyable nor movable (a copy deleted the name twice).
  - So do `Effects::Abstract`, `Filters::Abstract` and `EffectSlot` (and every effect / filter): neither copyable nor
    movable since Ave Robustus II P5 (2026-10-16) — the defaulted move did not clear the name either.
- **A source detaches its buffers BEFORE it lets its sound go** (2026-11-26). OpenAL refuses to delete a buffer still
  attached to a source (`AL_INVALID_OPERATION`, OpenAL Soft: "Deleting in-use buffer"): the buffer then stays allocated
  until the context dies. The invariant: a source holds its sound's resource (`m_currentPlayableInterface`) as long as
  one of that sound's buffers is attached. `Source::detachBuffers()` (stops the source, then `AL_BUFFER` = 0, which
  releases a static buffer and a queue alike) runs in `removeSound()` — so in `resetToDefaults()`, the return to the
  pool — and in `play()` when another sound replaces the current one.
  - Before: `removeSound()` unqueued a STREAMED sound's buffers only; a static sound's buffer stayed attached to the
    pooled source while the resource was released. Seen with the recorder off (`Core/RushMaker/EnableAudio` false,
    `ALSOFT_DRIVERS=null`), a console `SoundEmitter.play` of `wind1`, a `stop`, a shutdown: one "Deleting in-use buffer"
    in OpenAL Soft's log (`ALSOFT_LOGLEVEL=2`, `ALSOFT_LOGFILE=<file>`) and `AL_INVALID_OPERATION` at
    `GlobalReleaseFlush`. With the recorder on, the flush reads no error: the game context is already gone.
  - After (Linux): play, replace by another sound, pause, stop, replay, shutdown — 0 error in OpenAL Soft's log, with
    the recorder off and on; the music through play, three `next`, `previous`, `stop`, `play` with the crossfade on —
    0 error, no "clearing streams" warning.
- **A source's effect channel IS its auxiliary send** (2026-11-26). Both contexts — the manager's and the recorder's
  loopback "game" context — ask for `OpenAL::RequestedAuxiliarySends` = 4 sends a source (OpenAL Soft: 2 by default,
  6 at most); the count is read from the device the GAME is mixed on, after the recorder started
  (`OpenAL::readAuxiliarySendCount()`, traced: "Auxiliary sends per source"). The LAST send is the listener's medium's
  (doc 15); `Source::addEffect(…, channel)` takes a channel in `[0, OpenAL::userAuxiliarySendCount())` — 3 channels —
  and refuses another one (traced). `EffectSlot::enable(source, send)` / `disable(source, send)` write that send, with
  the slot's filter or without.
  - Before: `enable()` wrote send 0 without a filter and send 1 with one, whatever the channel, and `disable()` cleared
    send 0 only — two effects of one source replaced each other, and an effect with a filter took the listener's send
    on a device with two sends. Read in the code; `addEffect()` has no caller in the cascade.
  - Proven (Linux, a temporary probe on a played source, `wind1`, the mix written whole — recorder off — power from
    1.0 to 3.5 s of play): no effect 1.019e-3 (twice, to the digit), an echo on channel 0 3.904e-3, a reverb on
    channel 1 2.378e-3, both 5.450e-3 — the sum of the two additions predicts 5.263e-3 (+0.15 dB). Channel 2 accepted,
    3 and −1 refused, 4 sends reported with the recorder on and off. Under water with both effects, the listener's
    reverberation still adds its level (4.721e-3 without it, 5.200e-3 with it).
  - ⚠️ Limits: a source's own effects are NOT muffled by the listener's medium — the medium filters the direct path
    and feeds its own send; a user's send carries the sound unfiltered (under water, both effects: −23.3 dB against
    −36.1 dB for the muffled sound alone). When a source is released its effect slots go with it: an effect's tail
    is cut at the stop.
- **Sample rates (2026-11-06, base `WaveFactory::Frequency` became a strong type)**: a sound or music file at any rate in
  [1 kHz, 384 kHz] loads (22 255 Hz, a classic Mac rate, was refused before) and is resampled ONCE to the playback rate
  by `SoundResource` / `MusicResource`. The settings `Core/Audio/PlaybackFrequency` and the capture frequency go through
  the same `Frequency::fromHz()`: any rate in that range is now accepted (only the ten standard rates were before),
  outside it the 48 kHz default is used with a warning. `Manager` keeps the rate the OpenAL context actually reports
  (`ALC_FREQUENCY`); a reported rate outside the range is ignored with a warning (it used to become Invalid silently).
- **Microphone privacy**: `Core/Audio/Capture/Enable` must be explicitly `true` before any microphone access. Default is `false`. Voice-over (`Core/RushMaker/EnableVoiceOver`) depends on this gate.
- **Available devices in settings**: Both playback (`Core/Audio/AvailableDevices`) and capture (`Core/Audio/Capture/AvailableDevices`) device lists are written to settings on each startup for easy device switching via settings file editing

### MusicResource reads audio tags from audio containers only (2026-10-18)

`MusicResource::load(filepath)` reads the title and artist with TagLib (`readMetaData()`) — never from a procedural
music (a JSON SFX script: it carries no tag; TagLib was asked anyway and every script music, BaroqueSonata,
IndustrialNoise, logged `[Error] Unable to read file … for audio tag extraction`, Windows peer). The tags are optional
metadata: a file TagLib does not read is an Info line, no error. A MIDI music has no tags either (its title is the
resource name, the artist "Unknown"). The traces print their paths through `Base::IO::toU8String()`: streaming the
`std::filesystem::path` itself goes through `BlobTrait`'s `std::quoted` (UTF-8, safe) and doubles the quotes of a
message that adds its own — cosmetic, about 125 engine traces still do it.
Validated 2026-10-18 on macOS and Windows (no tag error for BaroqueSonata, the tags of `Kyrandia/Medley.mp3` read,
0 VUID).

