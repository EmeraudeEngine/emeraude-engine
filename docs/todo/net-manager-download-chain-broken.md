---
id: net-manager-download-chain-broken
title: Net::Manager — the resource download chain is broken end to end; decide its fate
status: open
priority: unranked
scope: Net, Resources
opened: 2026-08-27
tags: [network, resources, download, decision]
---

# Net::Manager — the resource download chain is broken end to end; decide its fate

## Why

The network inventory of 2026-08-27 established that `Net::Manager` (resource downloads for
`"Source": "ExternalData"` entries) cannot deliver a resource, at four independent points:

1. **No notification is ever emitted** (`grep notify src/Net/` is empty) — `Resources::Container`
   waits for `FileDownloaded` forever; every `DownloadItem` stays `Pending`.
2. The pool lambda calls the legacy `Base::Network::download(uri, path, verbose)` with `true`
   where the caller meant `replaceExistingFile` — argument-meaning mismatch.
3. `Manager::download()` appends to `m_downloadItems` unsynchronised while workers hold
   references into it (reallocation → dangling reference); ticket `0` doubles as "already queued"
   and collides with `ServiceAccess::DownloadCacheHit`.
4. On the `Resources` side: a cache hit enqueues no loading task; `DownloadStatus::Error` is
   recorded as a success (`FIXME` in `Container.hpp`); `Core/Resources/DownloadEnabled` is read
   into a member nothing consults; the manager's cache directory and `LoadingRequest`'s are
   disjoint.

The transport itself is the **legacy** emeraude-base `Network.cpp`: cleartext even for
`https://`, throwing Asio overloads under `ASIO_NO_EXCEPTIONS` (DNS failure = `std::abort()`),
no timeout, no redirect, no chunked transfer, advertises `gzip` it cannot decode. emeraude-base's
production-grade `HTTPSClient` / `TLSConnection` / `TrustStore` exist and are **not referenced
by the engine**.

**There is no consumer**: no engine code loads an `ExternalData` resource, downstream
applications download from their scripting layer, and the one call site in a test application
is a debug key with hard-coded paths.

## What remains — an owner decision first

- [ ] **Decide between**:
  - **(a) Rewire and repair**: `Net::Manager` on `Base::Network::HTTPSClient` (HTTPS-only by
    construction), emit the four notifications, fix the argument mismatch, protect
    `m_downloadItems`, unify the two cache schemes, make `DownloadEnabled` effective, fix the
    cache-hit and error paths in `Container.hpp`. Then delete the legacy `Network::download()`
    from emeraude-base (a removal, compatible with its feature freeze).
  - **(b) Remove**: delete `Net::Manager`, `DownloadItem`, `CachedDownloadItem`, the
    `ExternalData` source type and `ServiceAccess`'s download shim, the `DownloadEnabled`
    setting, and the legacy `Network.cpp` in emeraude-base. Reintroduce a downloader only when
    a real consumer appears, on `HTTPSClient`.
- [ ] Whichever is chosen: update `src/Net/AGENTS.md` § Download manager,
  `src/Resources/AGENTS.md`, `docs/resource-management.md`, emeraude-base
  `docs/plans/network-tls/README.md`, and delete this file.

## ⚠️ Traps

- ⚠️ `src/Net/AGENTS.md` described, until 2026-08-27, an API (`requestDownload`, `isCached`,
  `clearCache`, `forceDownload`) that **never existed** and an automatic URL detection that does
  not exist. Never code against a description of this manager — read `Manager.hpp`.
- ⚠️ Any repair touching emeraude-base happens under its **feature freeze** ("Ave robustus!",
  `docs/todo/ave-robustus-formal-closure.md` there): removals and bug fixes only.

## References

- Engine: `src/Net/Manager.{hpp,cpp}`, `src/Resources/Container.{hpp,cpp}`,
  `src/Resources/LoadingRequest.cpp`, `src/Resources/BaseInformation.cpp`, `src/SettingKeys.hpp`.
- emeraude-base: `src/Network/Network.cpp` (legacy), `src/Network/HTTPSClient.hpp` (target).
