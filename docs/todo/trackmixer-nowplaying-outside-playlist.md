---
id: trackmixer-nowplaying-outside-playlist
title: TrackMixer's nowPlaying refuses a track played outside the playlist
status: open
priority: high
scope: src/Audio/TrackMixer.cpp, src/Audio/TrackMixer.hpp, src/Audio/TrackMixer.console.cpp
opened: 2026-10-18
tags: [audio, console, defect]
---

# TrackMixer's nowPlaying refuses a track played outside the playlist

## Why
`TrackMixer.play("BaroqueSonata")` (a store music not in the playlist) plays it, then `nowPlaying()` answers "Playing
state is inconsistent (no playlist entry at current index) !". The state is legal: `play(track)` sets `m_musicIndex` to
`SIZE_MAX` on purpose ("out of playlist", its own comment), and `play()` without argument restarts the playlist "after
a track played outside it". But `status()` builds `currentTrack` from the playlist only, so the playing resource is
unknown, and `nowPlaying()` reads the out-of-playlist index as an error. Seen 2026-10-18 while checking the script
musics.

## What remains
- The mixer keeps the resource it plays (set by `startTrack()`, cleared by `stop()`), and `status()` reports it whatever
  the index.
- `nowPlaying()` shows the track with "outside the playlist" instead of an index; `currentPosition()` /
  `currentDuration()` already follow the playing source (check).
- Verify: play a store music by name, `nowPlaying()` shows its title; then `play()` restarts the playlist.

## References
- `src/Audio/TrackMixer.cpp` `play(const std::shared_ptr< MusicResource > &)`, `status()`;
  `TrackMixer.console.cpp` `nowPlaying`.
