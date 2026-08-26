---
id: remove-invalid-noexcept
title: Remove every invalid noexcept keyword
status: in-progress
priority: unranked
scope: cascade-wide
opened: unknown
tags: [cpp, hygiene]
---

# Remove every invalid noexcept keyword

## What remains

- [ ] Sweep the engine for `noexcept` on functions that can in fact terminate — allocation,
  container growth, anything reaching a call that is not itself `noexcept`. A wrong `noexcept` is
  not a hint, it is a promise the compiler enforces with `std::terminate`.

## Origin

Inherited from the historical root `TODO.md` (marked WIP there, no date). The sweep was started
and never finished; nothing records how far it got, so it must be re-measured before being called
done.
