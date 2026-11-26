---
id: effect-slot-ignores-its-send-channel
title: A source's own effect slot writes fixed auxiliary sends, whatever channel was asked
status: open
priority: high
scope: Audio (Source, EffectSlot)
opened: 2026-11-26
tags: [audio, efx, defect]
---

# A source's own effect slot writes fixed auxiliary sends, whatever channel was asked

## Why

Found by reading, on 2026-11-26, while the listener's medium took the LAST auxiliary send of every pooled source
(audio doc 15). `Source::addEffect(effect, channel)` keeps one `EffectSlot` per `channel`, but
`EffectSlot::enable(source)` never receives that channel:

- without a filter it writes `AL_AUXILIARY_SEND_FILTER` on send **0**;
- with a filter it writes it on send **1**;
- `EffectSlot::disable(source)` clears send **0** only.

So two effects on channels 0 and 1 both land on send 0 (the second replaces the first), an effect with a filter
lands on send 1 whatever its channel — on a device with two sends (OpenAL Soft's default) that is the send of the
listener's reverberation, which it overwrites until the next change of medium — and an effect with a filter is
never disconnected by `disable()`.

No user in the cascade today (`addEffect()` is called by nothing in the engine nor in projet-alpha): not reproduced
at run time, read in the code only.

## What remains

- The send index is the channel: `enable(source, channel)` / `disable(source, channel)`, the filter optional on any
  send.
- The channels a user may take stop before the listener's send (`OpenAL::g_maxAuxiliarySends - 1`), refused beyond;
  or more sends asked for at the context's creation (`ALC_MAX_AUXILIARY_SENDS`, the recorder's game context too) —
  the owner's choice.
- A runtime proof: two effects on one source, both heard, on the mix written to a file (`ALSOFT_DRIVERS=wave`).

## References

- `src/Audio/EffectSlot.cpp` (`enable()`, `disable()`), `src/Audio/Source.cpp` (`prepareEffectSlot()`, `addEffect()`,
  `refreshListenerSend()`).
- `docs/subsystems/audio/15-the-listeners-medium.md`.
