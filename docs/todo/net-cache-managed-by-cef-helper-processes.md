---
id: net-cache-managed-by-cef-helper-processes
title: CEF helper processes start the download manager and sweep / evict the cache the main process is using
status: open
priority: high
scope: Net (Manager) / PrimaryServices in helper processes
opened: 2026-09-30
tags: [net, cef, multi-process, race]
---

# CEF helper processes start the download manager and sweep / evict the cache the main process is using

## Why

Seen on Linux (2026-09-30, triad section 4): with projet-alpha's CEF, every helper process brings up the primary
services — `[Success][PrimaryServices] … primary service up! [Helper]` — and the log then shows `NetManagerService`
"Downloads enabled" once per process (3 times in a `beams` run). Each instance runs its startup sequence
(`Manager::onInitialize()`): `loadCacheIndex()`, `sweepPartialFiles()` (DELETES every `*.part`), then
`enforceCacheBudget()` (EVICTS files), and later rewrites `index.json` — on the SAME directory
(`~/.cache/LNIsle/projet-alpha/downloads`) as the main process.

A helper that starts while the main process is downloading (CEF spawns renderer / GPU / utility helpers on demand)
deletes the main process's `.part` file mid-transfer, may evict what the main process just cached, and races it on
`index.json`.

## What remains

- Decide which services a helper process needs (probably none of Net): skip the download manager (and the API
  client) in helper mode, where PrimaryServices is set up — an owner choice (engine-side helper-mode service set vs
  a projet-alpha boot option).
- Or, if a helper must keep it: make the cache single-owner (a lock file held by the owner process, the others
  read-only).

## References

- `src/Net/Manager.cpp` (`onInitialize`, `sweepPartialFiles`, `enforceCacheBudget`), `src/PrimaryServices.cpp`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § Section 4.
