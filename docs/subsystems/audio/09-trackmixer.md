## TrackMixer (Jukebox)

Music playlist manager with full playback control. Observable for state change notifications.

### Key Features
- Playlist management: `addToPlaylist()`, `clearPlaylist()`, `playlist()`
- Playback control: `play()`, `pause()`, `resume()`, `stop()`
- Navigation: `next()`, `previous()`
- Seek: `seek(float position)` - Jump to position in seconds
- Progress: `currentPosition()`, `currentDuration()` - For UI progress bars
- Shuffle mode: `enableShuffle(bool)`, `isShuffleEnabled()`
- Loop mode: `setLoopMode(bool)`, `isLoopEnabled()`
- Cross-fade: `enableCrossFader(bool)` - Smooth transitions between tracks
- Volume: `setVolume(float)`, `volume()`

### State Management (UserState enum)
See `TrackMixer.hpp:UserState`
- `Stopped` - No playback
- `Playing` - Active playback
- `Paused` - Paused (source retained)

### Shuffle Implementation
- Uses `std::shuffle` with `std::mt19937` for randomization
- Maintains `m_shuffleOrder` vector of indices
- `generateShuffleOrder()` regenerates on enable or playlist change
- `next()`/`previous()` respect shuffle order when enabled

### Observable Notifications
TrackMixer notifies observers on state changes:
- `MusicPlaying` - Playback started
- `MusicPaused` - Playback paused
- `MusicStopped` - Playback stopped
- `MusicFinished` - Track ended (auto-advances if more in playlist)

### Code References
- `TrackMixer.hpp:previous()` - Navigate to previous track
- `TrackMixer.hpp:seek()` - Seek to position (uses `Source::setPlaybackPosition()`)
- `TrackMixer.hpp:currentPosition()` - Get current position via `Source::playbackPosition()`
- `TrackMixer.hpp:currentDuration()` - Get duration via `MusicResource::duration()`
- `TrackMixer.hpp:enableShuffle()` - Toggle shuffle mode
- `Source.hpp:playbackPosition()` - OpenAL `AL_SEC_OFFSET` getter
- `Source.hpp:setPlaybackPosition()` - OpenAL `AL_SEC_OFFSET` setter
- `MusicResource.hpp:duration()` - Returns `Wave::seconds()`

### Console Commands (Remote Console, TCP 7777)

Bound in `TrackMixer.console.cpp`. Prefix with the engine service path, e.g.:
`Core.AudioManagerService.TrackMixerService.<command>(<args>)`.

| Command | Arguments | Purpose |
|---------|-----------|---------|
| `play` | `[trackName]` | Resume if paused, else play the given track (exact resource name) or start the playlist. |
| `pause` | — | Pause current playback. |
| `stop` | — | Stop current playback. |
| `next` / `previous`,`prev` | — | Playlist navigation. |
| `volume`,`vol` | `[0..100]` | Get or set mixer volume. |
| `shuffle` | `[on\|off]` | Get or toggle shuffle. |
| `loop` | `[on\|off]` | Get or toggle loop mode. |
| `crossfade` | `[on\|off]` | Get or toggle crossfade transitions. |
| `seek` | `[seconds]` | Get or set playback position. |
| `status` | — | Overview of mixer state (volume, modes, index, position). |
| `nowPlaying`,`np` | — | **Atomic current-track query.** Returns title (filename fallback for MIDI/untagged files), artist, position, index. |
| `playlist`,`pl` | `[clear\|add <name>\|play <index>]` | List playlist or modify it. |

**`nowPlaying` rationale:** `status` only shows the playlist index (`N/32`), not the track identity. `nowPlaying` returns it atomically so scripts/AI agents don't race against shuffle-driven track changes (short MIDI + crossfade can switch tracks in ≈1 s). Output format is stable and one-block parseable.

### m_musicIndex Invariant (play(track) sync)

`play(const std::shared_ptr<MusicResource> &)` synchronizes `m_musicIndex` with the track's position in the playlist on every invocation. Before the fix, only `playIndex()`/`next()`/`previous()` updated the index, so a direct console call `play("Kyrandia2/Zanthia's Hut")` would leave `m_musicIndex` stale — producing wrong output from `nowPlaying`, `status`, `currentPosition`, and `currentDuration` (all read `m_playlist[m_musicIndex]`).

- If `track` is present in `m_playlist` → `m_musicIndex` = that entry's index.
- If `track` is not in the playlist (played ad-hoc) → `m_musicIndex = SIZE_MAX` as sentinel. Read paths guard via `m_musicIndex >= m_playlist.size()`.
- Linear scan is intentional: `m_playlist` is typically small (< 100 entries) and shared_ptr equality is O(1) per compare.
- Code reference: `TrackMixer.cpp:play(track)`.

### PlaylistResource & Playlist Swapping

Playlists are first-class resources backed by JSON manifests in the dedicated `MusicPlaylists/` store (separate from the audio store `Musics/` to avoid any confusion with the JSON synthesis recipes living in `Musics/`).

**Manifest schema** (minimal, intentional):
```json
{
	"tracks": [
		"Kyrandia2/Ferry",
		"Kyrandia2/Swamp"
	]
}
```
The playlist name is derived from the filename stem (`Kyrandia2.json` → `Kyrandia2`). Track entries are `MusicResource` names resolvable via the `Musics` container.

**Engine API** (`TrackMixer::loadPlaylist(const std::shared_ptr<PlaylistResource> &)`):
1. Captures persistent intent: `wasPlaying = (userState() == UserState::Playing)`. This is deliberately NOT `isPlaying()` — MIDI rendering is async, so a source may not yet be emitting when the swap is requested. `userState()` captures "the user wants music", which is the correct trigger for auto-resume.
2. Resolves every track name against the `MusicResource` container BEFORE mutating state. Unknown entries are skipped with a warning.
3. If resolution yields zero tracks (manifest empty or all entries invalid), the call returns `false` and the previous playlist is left intact.
4. Otherwise: `stop()` → `clearPlaylist()` → repopulate → if `wasPlaying`, `playIndex(0)` on the new playlist.
5. MIDI tracks may take a few seconds to render; `nowPlaying` will read "No track playing" until the async render completes and the `onNotification` callback triggers actual playback. This is expected behavior, not a bug.

**Console commands**:
- `listPlaylists` / `lpl` — enumerate all playlist manifests discovered in `MusicPlaylists/`.
- `loadPlaylist <query>` / `lp <query>` — swap the playlist. Resolution: (1) exact name via `isResourceExists`, (2) fuzzy case-insensitive substring on available playlist names. First match wins.

**Autoload** (projet-alpha): `App/Music/AutoloadPlaylist` + `App/Music/DefaultPlaylist` settings. Replaces the previous hardcoded prefix scan in `Application.cpp`.

**Adding a new playlist**: drop a new `<Name>.json` file in `data-stores/MusicPlaylists/`. On next startup (or via `listPlaylists` after an engine-side rescan) it appears immediately — no code changes needed.

**Code references**:
- `PlaylistResource.hpp/.cpp` — resource class, JSON parsing
- `TrackMixer.cpp:loadPlaylist()` — swap logic with atomic semantics
- `TrackMixer.console.cpp` — `listPlaylists` and `loadPlaylist` command bindings
- `Resources/Manager.cpp` — registers `Playlists` container bound to `MusicPlaylists` store

### Play Command Fuzzy Fallback (console)

`Core.AudioManagerService.TrackMixerService.play(<query>)` resolves `<query>` via two steps:

1. **Exact resource lookup** — uses `Container::isResourceExists(query)` to guard against `getResource()`'s silent `Default` fallback, then retrieves the resource if it actually exists. This preserves the ability to play tracks that live in the store but aren't in the current playlist.
2. **Playlist substring fallback** — if step 1 misses, calls `TrackMixer::findPlaylistTrack(query)` which performs a case-insensitive substring match on each playlist entry's `name()`. First match wins; no fuzzy algorithm beyond `String::toLower` + `std::string::find`.

If both steps fail, the command returns a descriptive error. Rationale for two steps: AI/remote console workflows pass approximate titles ("Snowy" → "Kyrandia2/Snowy Bridge"), while scripts or the UI still benefit from exact-name paths when possible.

- Code reference: `TrackMixer.console.cpp:play` command binding, `TrackMixer.cpp:findPlaylistTrack`.

### MusicResource.title() Fallback Behavior

`MusicResource::title()` returns the resource name (filename-derived, e.g. `Kyrandia2/Ferry`) when no ID3/metadata title has been set. Applies to formats without tag support — notably **MIDI** (TagLib cannot read MIDI metadata) and any audio file whose title tag is empty or missing.

- Sentinel: `MusicResource::DefaultInfo` = `"Unknown"` (private constant).
- Check order: `m_title.empty()` → `name()`; `m_title == DefaultInfo` → `name()`; else `m_title`.
- Artist behavior is **unchanged** — returns `"Unknown"` when tag absent.
- Code reference: `MusicResource.hpp:title()`.
- Downstream impact: `TrackMixer.cpp:play()` notifications (`MusicPlaying`, `MusicSwitching`) now show meaningful names for MIDI tracks instead of `"Unknown"`.
