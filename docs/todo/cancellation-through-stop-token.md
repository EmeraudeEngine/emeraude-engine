---
id: cancellation-through-stop-token
title: Two engine waits still cancel through a hand-made atomic flag
status: open
priority: high
scope: src/Resources/SharingServer.cpp, src/Resources/PeerStore.cpp, src/Net/UDPClient.cpp
opened: 2026-10-18
tags: [ave-robustus-ii, concurrency, defect]
---

# Two engine waits still cancel through a hand-made atomic flag

## Why
Ave Robustus II: cancellation is `std::stop_source` / `std::stop_token`, never a hand-made `std::atomic_bool *`. Seen
2026-10-18 while routing the readers: `SharingServer::fileSHA256(path, const std::atomic< bool > * cancel)` (fed by
`PeerStore`'s `m_cancel`) and `UDPClient::waitReadable(sock, timeoutMs, const std::atomic_bool * closing)`.

## What remains
- Both take a `std::stop_token`; their owners hold the `std::stop_source` (or the job's token) and stop it in their
  destructor / stop path.
- Check the latency stays bounded (the hash checks per 1 MiB block, the UDP wait per timeout slice).
- The resource-sharing peer test and the UDP / mDNS tests still pass (alpha `docs/` peer recipe).

## References
- `.claude/rules/ave-robustus-ii.md`; base `Base::TaskHandle` / `TaskGroup` (the cascade's stop-token primitives).
