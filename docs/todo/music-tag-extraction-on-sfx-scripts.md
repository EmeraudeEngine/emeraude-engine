---
id: music-tag-extraction-on-sfx-scripts
title: A music made from an SFX script logs a false error from the audio tag extraction
status: open
priority: high
scope: src/Audio/MusicResource.cpp
opened: 2026-10-18
tags: [audio, logging, defect]
---

# A music made from an SFX script logs a false error from the audio tag extraction

## Why
Windows peer, 2026-10-18: playing `BaroqueSonata` (a `.json` SFX script in the Musics store) logs
`[Error][MusicResource] Unable to read file '"…\Musics\BaroqueSonata.json"' for audio tag extraction.` — the music
loads and plays. `MusicResource::load(filepath)` decodes the script through `WaveFactory::FileIO::read()`, then calls
`readMetaData(filepath)` on every non-MIDI file: TagLib cannot read a JSON, and the miss is logged as an error. The
quotes are doubled: the trace streams the `std::filesystem::path` itself (`operator<<` quotes it) inside its own quotes;
the same pattern is in the other `MusicResource` traces.

## What remains
- Extract tags only from a format that carries them (the audio containers TagLib reads), never from a script; a file
  without tags is no error (Info at most).
- Every `MusicResource` trace prints its path through `Base::IO::toU8String()`.
- Verify: BaroqueSonata plays with 0 `[Error]`; a tagged OGG / MP3 still shows its title and artist.

## References
- `src/Audio/MusicResource.cpp` `load(const std::filesystem::path &)`, `readMetaData()`.
