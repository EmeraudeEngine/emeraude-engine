---
id: random-resource-pick-follows-an-unordered-map
title: Resources::Container::getRandomResource() — the pick is the same at every launch and differs from one OS to another
status: open
priority: unranked
scope: Resources/Container (getRandomResource()), emeraude-base Utility::quickRandom() (its only source of randomness)
opened: 2026-11-15
tags: [portability, randomness, cross-os, peers]
---

# Resources::Container::getRandomResource() — the pick is the same at every launch and differs from one OS to another

## Why

Seen by the Windows peer (rounds 8 to 11, 2026-11-15) and read in the code: a testbed scene that asks for a random sky
box drew the same NIGHT sky at every launch on Windows and a DAY sky on Linux. The scene is therefore not the same on
every OS — on Windows it has no main directional light, so the contact shadows sit on their fallback there — and a
cross-OS comparison of that scene's lanes or pixels compares two different scenes (scenes doc 14, the peers' round 11).

The mechanism, two implementation-defined steps:

- `Base::Utility::quickRandom< size_t >(0, size - 1)` is the C `rand()`, which nothing seeds (no `srand()` in the
  cascade): the same sequence at every launch, and a sequence that belongs to the C library (glibc, the MSVC runtime
  and Apple's libc do not give the same numbers; `RAND_MAX` is 32767 on Windows). `rand()` is not thread-safe either.
- The index is then walked through `m_localStore`, an `std::unordered_map`: its iteration order belongs to the standard
  library (hash function, bucket count, insertion history).

So "random" is neither random from one launch to the next, nor the same world on the three OSes — the situation the
cascade's rule on seeded randomness forbids for a seeded result (`Base::PortableRandom` / `Randomizer`).

## What remains

- An owner decision on the INTENT: a different pick at every launch (a real random source), or a reproducible pick that
  is the same on the three OSes (a `PortableRandom` draw over the resource names in a defined order — sorted). The
  second makes the peers' comparisons valid; the first needs the testbed to pin its sky when it compares.
- Whatever the intent: the draw goes through the cascade's generator, not `rand()`; the walk goes over an ORDERED list
  of names (`resourceNames()` sorted), not the map's own order.
- The other users of `Utility::quickRandom()` (a `/usr/bin/grep` over the three repositories): each one is either a
  cosmetic variation or a result somebody compares — list them, decide per family.

## ⚠️ Traps

- A testbed scene that relies on the pick being "always the day sky on Linux" changes its look the day this is fixed:
  pin its sky by name.
- `getRandomResource()` holds no lock while it walks the map beyond the one `getResource()` takes: read the function's
  locking before reordering it.

## References

- `src/Resources/Container.hpp` — `getRandomResource()`.
- emeraude-base `src/BaseUtility.hpp` — `quickRandom()`; `docs/subsystems/source-tree/23-portable-random.md`.
