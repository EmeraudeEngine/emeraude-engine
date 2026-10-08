---
id: deferred-work-without-owner
title: Five deferred-work sites with no lifetime protection
status: open
priority: high
scope: src/Graphics/Renderable/SpriteResource.cpp, src/Net/HTTPServer.cpp, PeerStore.cpp, SharingServer.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, concurrency, defect]
---

# Five deferred-work sites with no lifetime protection

## Why
Class D of the census:
- `SpriteResource.cpp:91` — a `[&, data]` pool factory implicitly captures `this` (against `Container.hpp:1380`);
- `HTTPServer.cpp:995` — `stop()` posts `[this, &locals]`; its 3 s timeout can return while the handler is still
  queued on dead stack locals;
- `PeerStore.cpp:178`, `SharingServer.cpp:421` — `notify_all()` after the unlock: the destructor can return between
  the two.

## What remains
- One local fix per site (P0), each with a reasoning written in the commit; TSan run on the sharing demo.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 3.2.
