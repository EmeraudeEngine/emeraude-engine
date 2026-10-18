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

