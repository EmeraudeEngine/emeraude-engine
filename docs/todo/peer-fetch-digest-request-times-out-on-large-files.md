---
id: peer-fetch-digest-request-times-out-on-large-files
title: A peer fetch deletes a complete copy when the peer's SHA-256 takes longer than the 30 s request budget
status: open
priority: high
scope: Resources/PeerStore (fetchFile, the digest wait), Resources/SharingServer (answerDigest)
opened: 2026-11-14
tags: [defect, resource-sharing, network, owner-decision]
---

# A peer fetch deletes a complete copy when the peer's SHA-256 takes longer than the 30 s request budget

## Why

After a download, `PeerStore::fetchFile()` asks the peer for the file's digest: `GET /sha256/<path>`.
`SharingServer::answerDigest()` computes it on its thread pool and answers ON THE SAME CONNECTION when the hash is
done: the connection stays silent meanwhile. The client's request runs under `RequestBudget` (30 s). When the peer's
hash takes longer, the request is aborted, the loop leaves without a digest (its `DigestPatience` of 120 s only
covers `503` answers), and the copy — complete and correct — is deleted: "the peer gave no SHA-256, the copy is
deleted".

**Measured 2026-11-14** (two instances on one machine, RTX 3070 Ti, `USD/WorldLobby.usdz`, 1 610 429 961 bytes):
- two RELEASE instances: fetched in 7.9 s, SHA-256 equal (the peer hashes it in about 3 s, some 500 MB/s);
- two DEBUG instances (a slow hash): `read(), unable to read from the peer : Operation aborted`, then
  "the peer gave no SHA-256, the copy is deleted" after 146 s.

At 500 MB/s the budget is reached by a file of about 15 GB; a slower peer (a laptop, a cold disk) reaches it sooner.

The same wait is where a stop is slow: a cancel or a shutdown that arrives while the client is parked on that
answer is seen when the answer comes — 3.5 s in the Release pair, 29.8 s in the Debug pair (the request budget).
The engine's shutdown waits that long (`~PeerStore()` waits for its job).

## What remains

- **Owner decision — the digest protocol**: the server answers at once (`202` or `503` with `Retry-After`) while it
  computes and the client polls within `DigestPatience` (each request short: a stop is seen within one poll); or the
  client's budget for that request follows the file's size. The first also bounds the stop latency.
- Reproduce with the Debug pair (the recipe of `docs/subsystems/resources/12-resource-sharing.md` § *Testing a pair on
  one machine*), then the same in Release with a file large enough.
- Base item `http-client-stop-does-not-wake-a-parked-read` is the other half of the stop latency.

## References

- `src/Resources/PeerStore.cpp` — `RequestBudget`, `DigestPatience`, the digest loop of `fetchFile()`.
- `src/Resources/SharingServer.cpp` — `answerDigest()`, `MaxDigestJobs`.
