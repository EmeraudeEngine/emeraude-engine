---
id: recorded-mix-holds-exact-zero-stretches
title: The mix written to a file holds stretches of exact zero while a sound plays
status: open
priority: high
scope: Audio (Manager, Source, the output device), measurement method
opened: 2026-11-26
tags: [audio, defect-unconfirmed, measurement]
---

# The mix written to a file holds stretches of exact zero while a sound plays

## Why

Found on 2026-11-26 while measuring the sound under a sea (audio doc 15) with OpenAL Soft's wave writer
(`ALSOFT_DRIVERS=wave`): the float mix holds stretches of EXACT zero while a sound emitter plays.

- AMD machine (RADV), five launches of projet-alpha `water-world`: 11 to 20 blocks of 50 ms of exact zero a file,
  while playing and after the stop.
- Linux (RTX 3070 Ti), an air launch, twice: 41 % of the samples of the 12.5 s of play are exact zeros on both
  channels, in runs of 1024, 2048, 3072, 4096, 5120 and 8192 samples (91 runs) which all start on the file's
  1024-sample grid; 142 of the 586 blocks of 1024 samples are entirely zero. Around a run the samples are a few steps
  of 1 / 32768.
- The sound's file (`modem-dialup`, 16 bits, stereo, 48 kHz) holds 0.4 % of zero samples over the same duration. It
  has quiet passages: 220 of its 585 blocks of 1024 samples are under 40 steps of 16 bits (a hiss of 5 to 35 steps).
- Before the play the mix alternates the same way between zero and a constant level of one step of 16 bits.

Ranked high by caution (owner, 2026-11-26): it is NOT established that it is a defect. Two readings:
1. the asset's quiet passages fall to zero somewhere in the chain (a conversion to 16 bits at the played level) —
   harmless, but a float mix quantized to 16-bit steps is then to explain;
2. sound is lost a mixing period at a time (a starved or restarted source) — a defect, audible on a real device.

The owner's listening on a real output device (2026-11-26, a wind loop) noticed nothing.

## What remains

- Decide between the two readings with a CONTINUOUS sound that has no quiet passage (a sine at a known level):
  any zero stretch in its mix is lost sound. Then the same with a sound at a low level, for the quantization.
- If sound is lost: whether it happens on a real output device too (the source's state and processed-buffer count
  over a play, read through the console), and where (the source's feeding, the pool, the manager's update).
- If it is the wave writer alone: say so in audio doc 15 and in projet-alpha's `test-instance` skill, with the way
  to measure around it.

## ⚠️ Traps

- An attempt to align the envelope of the mix on the asset's (10 ms windows, shifts of ±0.1 s) found no correlation:
  not a proof of lost sound — the start of the play in the mix is found by a threshold, and the asset begins with a
  quiet passage.
- The band ratios of audio doc 15 compare launches that all carry these stretches; their absolute levels are about
  2 dB under what a mix without them would give, if the stretches are lost sound.

## References

- `docs/subsystems/audio/15-the-listeners-medium.md` § Limits.
