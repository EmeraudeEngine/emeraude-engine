## Download manager (`Net::Manager`)

**Files**: `Manager.hpp/.cpp` (+ `Manager.console.cpp`), `DownloadItem.hpp`, `Types.hpp`.

**Identity**: `class Manager final : public ServiceInterface, public Base::ObservableTrait,
public Console::ControllableTrait`, `ClassId = "NetManagerService"` (console path
`Core.NetManagerService`). Constructed in `PrimaryServices` with the `FileSystem`, the `Settings`
and the shared `Base::ThreadPool`; registered to the console by `Core` next to Settings.

### Contract

```cpp
int download (const Base::Network::URI & url) noexcept;             // ticket >= 1, or InvalidTicket (0)
DownloadStatus downloadStatus (int ticket) const noexcept;         // Pending | Transferring | Error | Done
std::filesystem::path downloadedFilepath (int ticket) const noexcept; // empty unless Done
void dispatchCompleted () noexcept;                                // main thread, called by Core each cycle
size_t fileCount () const; size_t fileRemainingCount () const;     // tickets issued / transfers in flight
bool isDownloadEnabled () const; bool clearCache ();
std::vector< std::tuple< std::string, std::filesystem::path, uint64_t > > cachedFiles () const;
std::pair< uint64_t, uint64_t > downloadProgress (int ticket) const noexcept;  // {received, total (0 = unknown)}
enum NotificationCode { DownloadingStarted, FileDownloaded, DownloadingFinished, Progress }; // payload: ticket (int), none for Finished
```

0. **A failure says why.** `downloadFailure(ticket)` returns a `Base::Network::DownloadOutcome`
   (`BadScheme`, `Unreachable`, `TLSFailure`, `Timeout`, `Protocol`, `HTTPStatus`, `LocalIO`) plus
   the HTTP status when the exchange completed. `status(ticket)` prints them. A bare "Error" could
   not tell a 404 from an expired certificate, and neither could the consumer.
   ⚠️ **`TLSFailure` was never actually produced until 2026-08-28** — the value existed, was
   documented, had a `to_cstring()` entry, and nothing in `HTTPSClient.cpp` ever set it: an expired
   certificate came back as `Unreachable`, because `TLSConnection::connect()` collapses "TCP never
   reached" and "handshake refused" into one bool and the caller labelled both the same. That is the
   exact distinction the enum's own comment says it exists for — a caller retries `Unreachable` and
   must never retry a refused certificate. `TLSConnection::handshakeRefused()` now reports which
   phase failed, and `HTTPSClient` maps it. Measured on macOS through the whole chain: the console
   prints `failed: TLSFailure` where it printed `failed: Unreachable` the run before, with the same
   `certificate verify failed` line from `Network::TLSConnection` above it.

1. **One code path for the consumer.** `download(url)` always returns a ticket to wait on. A URL
   already in the cache, a URL already in flight (shared ticket) and a fresh transfer all end the
   same way: a `FileDownloaded` notification carrying the ticket, after which `downloadStatus()`
   is terminal (`Done` → `downloadedFilepath()` is the file; `Error` → nothing on disk). There is
   **no** synchronous "cache hit" return to special-case.
2. **Observers run on the main thread.** Workers never call `notify()`: they push an event, and
   `Core::executeMainLoopCycle()` calls `dispatchCompleted()` right after the console poll, which
   emits the queued notifications and persists the cache index. This is the same deferral scheme
   as `Console::Controller::poll()` and what the architecture rule "never emit from a worker or
   under a mutex" requires. Consequence: even a cache hit completes **one cycle later**, never
   inside `download()`.
3. **HTTPS only, verified.** The transfer is `Base::Network::HTTPSClient::download()` over a
   `asio::ssl::context` loaded with `TrustStore::applySystemTrustStore()` (plus
   `Core/Net/CABundleFile` when set). Certificate or hostname mismatch = `Error`. `http://` is
   refused at `download()` with a trace (there is no cleartext client in the base, by decision).
3b. **A failed URL is retried, not replayed.** Asking again for a URL whose ticket is `Error`
   restarts the transfer on that same ticket (the old behaviour re-queued the terminal `Error`
   forever: a texture that failed while the network was down could not be obtained again without
   restarting the process). A URL already in flight still shares its ticket.

3c. **The cache is bounded and swept.** `Core/Net/CacheMaxBytes` (default 2 GiB, 0 disables) evicts
   **strictly least-recently-used** entries after each successful download and at startup. ⚠️ Nothing
   is pinned: pinning the files of completed tickets was tried and makes the budget unenforceable —
   a ticket stays `Done` for the whole process lifetime, so every file would be pinned. What makes
   that safe is that the file just downloaded carries the highest use counter (evicted last) and
   `downloadedFilepath()` checks the file still exists before naming it. `.part` files left by a
   crash are removed at startup, and `clearCache()` walks the **directory**, not the index — an
   orphan is invisible to the map and would otherwise never be reclaimed.

4. **The cache is the manager's, keyed by URL.** `cacheDirectory("downloads")/<FNV-1a of the URL,
   16 hex>.<ext>` + `index.json` (`Files: [{URL, Filename, Bytes}]`). Two URLs sharing a basename
   never collide; the extension is kept because loaders sniff it. The transfer streams into
   `<file>.part` then renames, so a reader never sees a partial file and a failure leaves nothing
   under the final name. The index is loaded at init (entries whose file vanished are dropped) and
   serialised under the lock and **written outside it**, atomically (temporary file + rename): a
   crash mid-write used to leave a truncated index, which orphaned every file it named, and the
   write itself was a disk I/O performed on the main loop while holding a mutex a worker needed.
   The URL→ticket lookup is a map: the previous linear scan re-serialised every tracked URL on
   every request.
5. **Settings**: `Core/Net/CacheMaxBytes`, `Core/Net/DownloadTimeoutSeconds` (default 120, the
   whole budget of one download including redirects), `Core/Net/DownloadEnabled` (default `true`) — off, every `download()` returns
   `InvalidTicket` and the resource falls back to its default; `Core/Net/CABundleFile` (default
   empty) — a PEM bundle added to the system store for private CAs. Both written on first run.
   ⚠️ The old `Core/Resources/DownloadEnabled` was inert and is gone.
5b. **`isEnabled()` says why it is not.** Three independent things disable downloads (the setting,
   an unusable cache directory, a trust store that would not load) and only the log used to name
   which: the console answers `{"enabled":false,"reason":"..."}`.

5c. **`DownloadingStarted` / `DownloadingFinished` are edges**, tracked with a flag: they used to
   key on `m_inFlight == 0`, so every cache hit — which never increments it — emitted `Finished`
   for a transfer that never happened. `Pending` is a real observable state now: the ticket is
   `Pending` until a worker picks it up, `Transferring` after.

6. **Progress, throttled to one notification per ticket per cycle.** `HTTPSClient::download()`
   takes a `DownloadProgress` hook (first post-freeze feature of emeraude-base, 2026-08-27); the
   manager's hook runs on the worker and only records `bytesReceived` / `bytesTotal` (0 when the
   server sent no `Content-Length` — chunked or read-until-close) under the items mutex and raises
   a pending flag. `dispatchCompleted()` then emits **at most one `Progress` per ticket per
   main-loop cycle** (payload: the ticket; read `downloadProgress(ticket)`), whatever the
   transport read granularity (16 KiB reads on a 100 MB file would otherwise mean ~6 000
   notifications). `Done` sets received = total = final size.

### Verified (Linux, 2026-08-27, validation layers on, 0 VUID)

Through the console, on a live instance: a 13 566-byte file fetched over TLS (150 system CAs
loaded), ticket `Done` with its cache path; the same URL requested again returns the **same
ticket** and completes again; `http://` refused at `download()`; an `ExternalData` PNG store entry
requested with `loadResource()` reaches **`Loaded`** (184 bytes downloaded, then decoded); an
`http://` store entry and an expired-certificate host (`expired.badssl.com`) both reach
**`Failed`** with nothing left in the cache directory; `index.json` lists exactly the two
successes; a second launch serves the PNG from the cache — `Loaded` with **zero** `Downloading`
trace.

### Threading

`m_itemsAccess` guards the tickets vector (`m_items`, ticket = index + 1), the cache map and
`m_inFlight`; `m_eventsAccess` guards the event queue. `download()` may be called from any thread
(a resource loading task on the pool may request a dependency). `dispatchCompleted()` releases
both mutexes before notifying, because an observer may call back into `download()`. The manager
is neither copyable nor movable: observers and workers hold its address.

### Console

```bash
python3 tools/remote-console.py "Core.NetManagerService.download(https://raw.githubusercontent.com/EmeraudeEngine/emeraude-base/main/README.md)"
python3 tools/remote-console.py "Core.NetManagerService.status(1)"        # {"ticket":1,"status":"Transferring","bytesReceived":1048576,"bytesTotal":13566000,"remaining":1}
python3 tools/remote-console.py "Core.NetManagerService.listCache()"
python3 tools/remote-console.py "Core.NetManagerService.clearCache()"
python3 tools/remote-console.py "Core.NetManagerService.isEnabled()"
```

### Resources integration

A resource is downloadable when its store entry says **`"Source": "ExternalData"`** with a
`https://` URL in `"Data"` (`BaseInformation::parseSource`, validated with `URL::isURL()`).
⚠️ There is **no URL sniffing on the resource name**.

Chain (all in `Container.hpp`, behind the non-template `ServiceAccess` firewall implemented in
`Container.cpp`): `getResource(name, async)` → `ServiceAccess::startDownload()` →
`netManager().download(url)` → the request is parked in `m_externalResources`
(**multimap**: two resources may share a URL, hence a ticket) → `Container::onNotification()`
receives `FileDownloaded` on the main thread → `Done`: `LoadingRequest::setDownloadProcessed(
downloadedFilepath)` rewrites the `BaseInformation` to `LocalData` on the cached file and the
usual `loadingTask` is enqueued; `Error` (or a refused `download()`): `LoadingRequest::setDownloadFailed()`
+ `ResourceTrait::failLoading()` — the fail-safe contract, observers get `LoadFailed` and the
consumer keeps the default resource. `LoadingRequest` no longer computes a cache path: the
manager owns the file.

Console check of the whole chain: drop a store JSON with an `ExternalData` entry
(`Core.openFiles("/abs/path/store.json")`), then
`Core.ResourcesManagerService.loadResource(ImageResource, MyPicture)` and poll
`Core.ResourcesManagerService.resourceStatus(ImageResource, MyPicture)` until `Loaded`.

⚠️ Do not improvise that check — use the replayable fixture,
`app_system/tools/external-data-check/`, which pins one store, three resources (nominal, expired
certificate, cleartext) and the same command sequence on the three OSes. Improvising it is how the
two defects below stayed hidden:

- **`Core.openFiles` on a store whose store name did not exist at boot registered resources no
  container could ever see** (`Resources::Manager::getLocalStore()` returned null, and a container
  binds its store once). On a host with no store directories — app_system — *every* container was
  sterile, so this whole chain was unreachable while reporting success. Fixed engine-side; see
  [`Resources/AGENTS.md`](../../../src/Resources/AGENTS.md) § *A container binds to its store ONCE*.
- **`TLSFailure` was reported as `Unreachable`** (above), which made the fixture's own
  discriminator for "is the trust store really refusing?" unusable.
