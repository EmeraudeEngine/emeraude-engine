# Net System

Context for the Emeraude Engine `src/Net/` directory: the download manager (HTTPS, for `ExternalData` resources), the web API client, and the hardware & discovery utilities.

## Module Overview

Three things live under `src/Net/`:

1. **`Net::Manager`, the download manager** — a service that fetches `https://` files into a
   URL-keyed cache through emeraude-base's `HTTPSClient`, for the `Resources` system
   (`"Source": "ExternalData"` entries) and for anything else that needs a file from the network.
   Drivable from the console (`Core.NetManagerService.*`). Rebuilt 2026-08-27 on the owner's
   decision — until then it was a semi-stub that never completed a download.
2. **`Net::APIClient`, the web API client** — a service that performs arbitrary HTTPS exchanges
   (`GET`/`POST`/`PUT`/`PATCH`/`DELETE`, caller headers, request body) and hands the response back
   **in memory**, on the main thread, addressed by a ticket. Drivable from the console
   (`Core.NetAPIClientService.*`). Added 2026-08-28. ⚠️ **It is not `Net::Manager` with a different
   verb** — see § Web API client for the four deliberate differences.
3. **The hardware & discovery utilities** (`UDPClient`, `NetworkInterfaces`, `TCPClient`,
   `TCPServer`, `SerialPort`, `WiFiScanner`) — standalone, `noexcept`, cross-platform classes
   with no dependency on the manager, consumed by downstream applications through scripting
   bridges (TCP/UDP/serial/WiFi modules). Documented in § Hardware & Discovery Utilities.

**Not multiplayer**: nothing here is gameplay networking, and nothing should become it without
an owner decision.

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
  [`Resources/AGENTS.md`](../Resources/AGENTS.md) § *A container binds to its store ONCE*.
- **`TLSFailure` was reported as `Unreachable`** (above), which made the fixture's own
  discriminator for "is the trust store really refusing?" unusable.

## Web API client (`Net::APIClient`)

**Files**: `APIClient.hpp/.cpp` (+ `APIClient.console.cpp`), `APIRequestItem.hpp`, `Types.hpp`.

**Identity**: `class APIClient final : public ServiceInterface, public Base::ObservableTrait,
public Console::ControllableTrait`, `ClassId = "NetAPIClientService"` (console path
`Core.NetAPIClientService`). Constructed in `PrimaryServices` with the `Settings` and the shared
`Base::ThreadPool`; registered to the console by `Core` next to the download manager.

### Why it is a separate service, and not `Net::Manager`

The owner ruled a distinct service on 2026-08-28. Four behaviours differ, each on purpose — a
future session must not "harmonise" them:

| | `Net::Manager` (downloads) | `Net::APIClient` (APIs) |
|---|---|---|
| **Cache / dedup** | URL-keyed disk cache; the same URL in flight folds onto one ticket | **None.** Two identical `POST`s are two calls; a polling `GET` must actually go out |
| **Retry** | A URL that failed retries on the same ticket | **None.** Replaying a write is how a payment gets charged twice |
| **Where the body lands** | Streamed to a file, never held whole | **Held in memory** — hence the retention ceiling below |
| **A non-2xx** | `Error` — a 404 is not a file | **`Done`** — an API says what went wrong in the body, and `responseStatusCode()` tells the caller |

### Contract

```cpp
int ticket = apiClient.post(URI{"https://api.example/actors"}, R"({"name":"paladin"})");
// … observe ResponseReceived, then, on the main thread:
if ( apiClient.requestStatus(ticket) == APIRequestStatus::Done )
{
    const auto httpStatus = apiClient.responseStatusCode(ticket);   // 201, 404, 422 …
    Json::Value document;
    if ( apiClient.responseJSON(ticket, document) ) { /* parsed on the worker */ }
    apiClient.release(ticket);                                       // ⚠️ MANDATORY, see below
}
```

- **Ticket lifetime — `release()` is part of the contract.** A response body sits in RAM. The
  caller calls `release(ticket)` once it has read it. A ceiling
  (`Core/Net/API/MaxRetainedTickets`, default 64) drops the **oldest terminal** tickets when the
  caller forgets, and logs **once** that it had to. ⚠️ That is a safety net, not a policy: the
  ticket it drops may be one nobody read yet. A ticket still in flight is **never** dropped.
- **`cancel(ticket)` frees the caller, not the socket.** `HTTPSClient` is synchronous and has no
  cancellation point: a call already on the wire runs to completion on its worker and its response
  is thrown away on arrival. A cancelled ticket emits **no** notification.
- **Authentication is set at runtime**, `setDefaultHeader("Authorization", "Bearer …")`, merged
  under the per-call headers (a per-call header of the same name wins). ⚠️ **Deliberately not read
  from `Settings`**: a token in `settings.json` would sit in cleartext on disk and be dumped by
  anything printing the settings.
- **Every header is validated twice** — once here, so a malformed one costs an `InvalidTicket` the
  caller sees immediately, and once in `HTTPSClient::run()`. See
  `HTTPSClient::isRequestHeaderAcceptable()`: RFC 9110 token names, no CR/LF/NUL/C0 in values, and
  the framing headers (`Host`, `Content-Length`, `Connection`, `Transfer-Encoding`,
  `Accept-Encoding`) refused outright.
- **JSON is parsed on the worker**, never on the main thread, and only when the server declared a
  JSON media type (`+json` suffixes included). A body that is not JSON is not an error: it stays
  readable through `responseBody()`.
- **One TLS connection per call** — `HTTPSClient` has no keep-alive reuse yet. Known cost on an
  API hit in rapid succession; fixing it is a base-level change to `TLSConnection`.

### Notifications

`RequestStarted` (payload: ticket) on the 0→1 in-flight edge, `ResponseReceived` (payload: ticket)
per completed call, `RequestsFinished` (no payload) on the 1→0 edge. All emitted from
`dispatchCompleted()`, which `Core` calls at the top of every main-loop cycle — **that is what puts
the observers on the main thread**. ⚠️ The batch edge is read from `m_inFlight`, not from the event
count: a cancelled ticket queues no event, and counting events would leave the batch "running"
forever after a batch that ended on a cancellation.

⚠️ `Core` registers the service to the console but does **not** observe it: unlike a download, an
API response is the business of whoever asked for it. Consumers observe `Net::APIClient` directly.

### Console

`Core.NetAPIClientService.{request,get,post,status,header,list,release,cancel,setHeader,removeHeader,headers,isEnabled}`.

> [!CAUTION]
> **Owner decision, 2026-08-28: this console surface is FULLY EXPOSED.** `request()` accepts an
> arbitrary URL, method and body, and `headers()` prints the default headers — **bearer token in
> clear**. Redaction was offered and the owner chose full exposure for debugging comfort. **Do not
> "fix" this into redaction without asking**: it is a decision, not an oversight. What contains it
> is that the remote console is closed by default and binds `127.0.0.1`
> (`Core/Console/EnableRemoteListener`).

### Settings

| Key | Default | What it does |
|---|---|---|
| `Core/Net/API/Enabled` | `true` | Whether calls are performed at all |
| `Core/Net/API/TimeoutSeconds` | `30` | Total budget of one call, redirects included |
| `Core/Net/API/MaxRetainedTickets` | `64` | Terminal tickets kept before the oldest are dropped; `0` disables the ceiling |
| `Core/Net/API/MaxResponseBytes` | `16 MiB` | Ceiling of ONE response body — it is held whole in RAM |
| `Core/Net/CABundleFile` | `""` | **Shared with the download manager**: a corporate CA is a property of the machine |

### ⚠️ Cross-platform status

`APIClient` itself is pure C++ over `HTTPSClient` — no syscall, no platform leg. What it rides on
(`HTTPSClient`, `TLSConnection`, `TrustStore`) cleared steps 1-3 of the handover on Linux, macOS
**and** Windows (2026-08-28). **The service itself has only ever run on Linux**, exactly like
`Net::Manager` — see the handover item, it is the same gap.

## Cross-platform status — read before trusting anything here

**All three platforms have now executed this directory** (2026-08-28). That is new — it had been
Linux-only for its whole life — and it cost four bugs in two days. Read the two subsections below
before touching anything here.

| Leg | Linux | macOS | Windows |
|---|---|---|---|
| `UDPClient` multicast / mDNS round trip | ✅ 2026-08-26 | ✅ 2026-08-28 | ✅ 2026-08-28 (6 LAN hosts answered) |
| `NetworkInterfaces` IPv4+IPv6+MAC | ✅ | ✅ 2026-08-28 (`AF_LINK`) | ✅ 2026-08-28 (`GetAdaptersAddresses`) |
| Trust store + hermetic & live TLS suites | ✅ | ✅ 2026-08-28 | ✅ 2026-08-28 (78/78, 43 CAs from `ROOT`) |
| `Net::Manager` downloader, `ExternalData` chain | ✅ | ❌ not yet | ⚠️ download ✅, `ExternalData` chain ❌ |
| `SerialPort` against real hardware | ✅ | ❌ no adapter available | ❌ no adapter, but see below |

⚠️ Depth is **not** uniform, and the table flattens that. macOS was exercised through an
out-of-tree harness on this directory's sources; Windows through app_system's own JS path
(`--mode=test`, dev-check fixtures over CDP), which is the layer macOS has never run. Neither
substitutes for the other.

### What macOS taught us (2026-08-28)

Three bugs, all fixed in the same sitting. The first two are **not macOS quirks** — they were
latent everywhere and only macOS made them visible:

1. ⚠️⚠️ **`shutdown()` does not wake a reader on an unconnected datagram socket.** POSIX makes it
   fail with `ENOTCONN`; **Linux is the lenient outlier** that wakes the reader anyway. Measured:
   `close()` waited out the receive() timeout **in full** (10 s) instead of returning. Since
   app_system binds `UDP.close()` as a *synchronous* WebModule method, that stall lands on the
   renderer's main thread. `UDPClient` no longer trusts the kernel for the wake-up — `close()`
   raises a flag and `receive()` polls in 50 ms slices (`PollSliceMs`). **Confirmed on Windows the
   same day, by measurement rather than inference**: the pre-fix symptom was visible there
   (~107 ms `close()`, one drain-loop block), and after the fix `close()` returns in **62 ms with a
   `receive()` parked on a 3000 ms timeout — and the same 62 ms at 1000 ms**, i.e. bounded by the
   poll slice and independent of the receive timeout, which is exactly the contract.
   `TCPClient` is *not* concerned (it shuts down a **connected** socket, which works everywhere),
   and neither is `TCPServer` (Asio `acceptor->cancel()`). Verified on this machine: unconnected
   UDP → not woken; connected UDP → woken in 203 ms; listening TCP → not woken.
2. ⚠️ **A moved-from `UDPClient` crashed on destruction**, on every platform: the move handed away
   `m_handleMutex`, and `close()` — unlike `TCPClient::close()` — dereferenced it with no null
   check. Caught by AddressSanitizer. `close()`/`send()`/`receive()` now guard, and `open()`
   re-arms the guards so a moved-from instance is reusable rather than a silent trap.
3. ⚠️ **`SerialPort.mac.mm` ran at 9600 bauds without telling anyone.** `toBaudConstant()` fell
   back to `B9600` in its `default:` branch and `open()` returned **true** — measured on a pty:
   `open(250000)` → `true`, line at 9600. macOS stops at `B230400` where Linux carries constants
   to `B4000000`, so **250000 (Marlin's default) hit that branch every time**. Now mirrors the
   Linux `BOTHER` design with `ioctl(IOSSIOSPEED)` (`<IOKit/serial/ioss.h>`, applied **after**
   `tcsetattr` or it is undone), and `open()` returns **false** when the adapter refuses the rate.
   The macOS switch also gained `B7200`/`B14400`/`B28800`/`B76800`, which it simply lacked.

### What Windows added (2026-08-28, same day, app_system's JS path)

Windows was validated through `--mode=test` and the dev-check fixtures, i.e. the layer **macOS has
never run**. What it contributed to this directory:

- **The `close()` fix is measured, not inferred** — see point 1 above.
- **The deadline-based timeout accounting holds**: 201/200, 1001/1000, 3013/3000 ms
  (+0.4%, +0.1%, +0.4%). None of the slice-counting drift it was written to avoid.
- **`MulticastOptionValue`'s `DWORD` branch works** — TTL 255 and loopback both took.
- ⚠️ **`SerialPort.windows.cpp` needs no baud work at all**, and this closes the question the other
  two platforms opened: it assigns `dcb.BaudRate = config.baudRate` directly — an arbitrary `DWORD`,
  with **no `CBR_*` constant table** to fall out of. The Linux `termios2`/`BOTHER` and macOS
  `IOSSIOSPEED` problem structurally cannot arise there. Still untested against real hardware, but
  there is no lookup to be silently wrong.
- ⚠️ **A dead interface aborts a naive join loop, and Windows always has dead interfaces.**
  `IP_ADD_MEMBERSHIP` fails on an APIPA address (`169.254.x.x`), and a Windows host routinely
  exposes 4+ of them (disconnected Wi-Fi, Bluetooth PAN, Hyper-V/VPN) where a Linux dev box has
  none. `joinMulticastGroup()` correctly returns `false`; it is the **consumer** that must treat
  that as "skip this interface", never "abort discovery". Enumerating on
  `family == IPv4 && !internal` does **not** exclude them — APIPA is not flagged internal.
- ⚠️ **The long-standing Windows trap "`TCPServer` binding `"::"` opens a v6-only socket" was
  never real on this code path — measured 2026-08-28, and the claim is retracted.** The pre-fix
  acceptor already accepted an IPv4 peer on `"::"` there (4/4): Asio clears `IPV6_V6ONLY` on every
  `AF_INET6` socket it creates on Windows, and the option reads 0 before any `bind()`. A raw
  `::socket(AF_INET6)` on Windows *does* default to 1, which is where the trap genuinely lives —
  raw Winsock, not `TCPServer`. The explicit call added that day is kept for stating the intent
  instead of depending on an Asio internal, but it fixes nothing observable. Recorded so nobody
  hunts the symptom again.

> [!NOTE]
> The bug the Windows run actually surfaced first was **not in this directory**: app_system's
> `SharedDataManager::createJob<>()` locked a non-recursive mutex twice, which MS-STL turns into a
> `std::system_error` inside a `noexcept` binding — instant renderer death on every
> `JobInterface` module. glibc self-deadlocks instead of throwing, so Linux would have hung rather
> than crashed. Fixed in app_system; recorded here only because it is why the Windows network run
> could not start.

### What the Linux replay closed (2026-08-28)

Linux was re-run right after the macOS sitting, because that sitting had changed shared code on a
platform that was green. `tools/net-check` — **48 pass / 0 fail / 1 warn**, same result under
`-fsanitize=address,undefined`:

- ✅ **The `MulticastOptionValue` width change is good**: `int` accepted, TTL reads back 255.
- ✅ **The shared-path changes replay too**, which the commit message did not claim and nobody had
  measured here: `close()` returns a parked `receive()` (300 ms, bounded by the poll slice), the
  deadline-based accounting shows **0 % drift over 1200 ms**, and the moved-from instance is safe
  and reusable. `shutdown_semantics` re-confirms Linux as the lenient outlier — `ENOTCONN` on an
  unconnected UDP socket **and the reader woken anyway**, which is exactly why the bug could hide
  here for the directory's whole life.
- ⚠️ **It also found one defect of its own**, unrelated to the macOS work and present since
  `NetworkInterfaces` existed: `enumerateMulticastCapable()` dropped `lo` on Linux — see the
  loopback trap under [§ Network Interfaces](#network-interfaces-netnetworkinterfaces). Fixed the
  same day, Linux-scoped.

> [!NOTE]
> The lesson is the one the macOS sitting already paid for once, in the other direction: **a fix
> proven on one platform is a change on every other one.** Two of the three items above are shared
> code that no per-platform reasoning would have flagged, and the third was found only because the
> replay ran the whole harness instead of the one assertion the commit pointed at.

Two findings that are **not** bugs but invalidate a written assumption:

- The `IP_MULTICAST_TTL` byte-type story does **not** reproduce on macOS 26 / arm64: this kernel
  accepts the `int` form as readily as the `unsigned char` one. "The TTL silently stayed at 1 and
  devices one hop away went missing" is not an explanation that holds here — do not hunt that ghost.
  Since that leniency is an undocumented implementation detail, the width is now taken from each
  platform's own manual through **`MulticastOptionValue`** (top of `UDPClient.cpp`), applied to
  `IP_MULTICAST_TTL` **and** `IP_MULTICAST_LOOP` in `setMulticastTTL()`, `setMulticastLoopback()`
  and `ssdpDiscover()`: `unsigned char` on macOS/BSD (`ip(4)`), `int` on Linux (`ip(7)`), `DWORD`
  on Windows. This **changed the Linux leg** from `unsigned char` to `int`; ✅ **replayed on Linux
  2026-08-28** — the `int` width is accepted and the TTL reads back as 255 through it. That kernel
  also accepts both widths, exactly like macOS 26, which is why the leniency is treated as an
  implementation detail rather than a contract.
- `SerialPort.mac.mm` never claims the port: Linux does `ioctl(TIOCEXCL)` ("a second process
  opening the same adapter mid-print is a corrupted stream nobody can diagnose"), macOS does not,
  though the ioctl exists there. Left alone deliberately — exclusive open is a behaviour change,
  not a bug fix.

### The remaining handover

Two todo items carry the checklist, and they are meant to be run in one sitting on those machines:

- [`emeraude-base/docs/todo/tls-stack-windows-macos-validation.md`](../../dependencies/emeraude-base/docs/todo/tls-stack-windows-macos-validation.md)
  — the trust store per platform, the hermetic and live suites, the downloader from the console, the
  `ExternalData` chain, and the traps (MS-STL's throwing `path::string()` under `-fno-exceptions`,
  the MSVC-only `#pragma comment(lib, …)`, the `IOKit` link that arrives through hwloc, the
  `v6_only` default on Windows).
- [`docs/todo/udp-multicast-macos-verification.md`](../../docs/todo/udp-multicast-macos-verification.md)
  — multicast/mDNS, the IPv6+MAC enumeration, the non-blocking receive and the SSDP TTL type, plus
  the macOS 15 *Local Network* permission (test a **signed, packaged** binary, never a console run).

## Development Commands

> [!WARNING]
> **There is no test target in the engine.** The engine `CMakeLists.txt` declares no
> `add_test()`; the only unit suite in the cascade lives in **emeraude-base**
> (`EmeraudeBaseUnitTests`).
>
> To exercise a `Net` utility, compile it out-of-tree: the hardware utilities depend on
> nothing but `emeraude_export.hpp`, so a standalone binary works and gives a real runtime
> check rather than a compile check.
>
> ```bash
> g++ -std=c++20 -I<engine>/src my_check.cpp \
>     <engine>/src/Net/UDPClient.cpp <engine>/src/Net/NetworkInterfaces.cpp -o my_check
> ```
>
> **Do not write that harness from scratch — one is in the tree**:
> [`tools/net-check/`](../../tools/net-check/README.md) builds on Linux, macOS and Windows and runs
> 48 assertions over `UDPClient` + `NetworkInterfaces` (interfaces and MACs, multicast option width,
> non-blocking receive, `close()` waking a parked reader, moved-from safety, a real mDNS round trip,
> SSDP). `shutdown_semantics.cpp` next to it answers "does `shutdown()` wake a reader on this
> kernel?" for the three socket shapes — that is the probe that found the 2026-08-28 bug.

## Hardware & Discovery Utilities

Beyond the download manager, the Net module provides cross-platform hardware utilities for device discovery and communication. These are **standalone utility libraries** (not services), called on-demand.

### UDP Client (`Net::UDPClient`)

**File**: `UDPClient.hpp/.cpp`

Cross-platform UDP client providing both generic datagram socket operations (open/bind/send/receive/close) and self-contained SSDP discovery.

**API**:
```cpp
namespace EmEn::Net
{
    struct SSDPDevice {
        std::string address;
        uint16_t port{0};
        std::map< std::string, std::string > headers;
    };

    struct DatagramInfo {
        std::string senderAddress;
        std::string destinationAddress;   // what the datagram was addressed TO
        uint32_t interfaceIndex{0};       // receiving interface, 0 when unavailable
        uint16_t senderPort{0};
        bool multicast{false};            // destination is in 224.0.0.0/4
    };

    class UDPClient final {
    public:
        // Socket lifecycle
        bool open () noexcept;
        bool bind (uint16_t port, const std::string & address = {}) noexcept;
        void close () noexcept;
        bool isOpen () const noexcept;

        // Data transfer
        int send (const std::string & host, uint16_t port, const void * data, size_t length) noexcept;
        int send (const std::string & host, uint16_t port, const std::string & data) noexcept;
        int receive (void * buffer, size_t maxLength, std::string & senderAddress,
                     uint16_t & senderPort, uint32_t timeoutMs = 0,
                     bool * timedOut = nullptr) noexcept;
        int receive (void * buffer, size_t maxLength, DatagramInfo & info,
                     uint32_t timeoutMs = 0) noexcept;
        std::string receiveString (size_t maxLength, std::string & senderAddress,
                                   uint16_t & senderPort, uint32_t timeoutMs = 0) noexcept;

        // Query
        bool getLocalAddress (std::string & address, uint16_t & port) const noexcept;

        // Options
        bool setBroadcast (bool enable) noexcept;

        // IPv4 multicast
        bool joinMulticastGroup (const std::string & groupAddress,
                                 const std::string & interfaceAddress = {}) noexcept;
        bool leaveMulticastGroup (const std::string & groupAddress,
                                  const std::string & interfaceAddress = {}) noexcept;
        bool setMulticastTTL (uint8_t ttl) noexcept;
        bool setMulticastLoopback (bool enable) noexcept;
        bool setMulticastInterface (const std::string & interfaceAddress = {}) noexcept;

        // SSDP convenience (static, self-contained)
        static std::vector< SSDPDevice > ssdpDiscover (const std::string & searchTarget,
                                                        int timeoutSeconds = 5) noexcept;
    };
}
```

**Design**: RAII (closes in destructor), movable, non-copyable, all functions `noexcept`. The `ssdpDiscover()` static method creates a temporary socket internally for UDP multicast M-SEARCH (239.255.255.250:1900).

⚠️ **`receive(timeoutMs = 0)` is NON-BLOCKING** — it polls and returns 0 at once. It used to run a
blocking `recvfrom` with no way out, parking the calling thread forever while the header promised the
opposite (2026-08-27).

⚠️ **`close()` is safe against a parked reader**, but NOT the way `TCPClient` is. A `shared_mutex`
covers each syscall and `close()` takes the exclusive lock before invalidating the descriptor —
without it, a `close()` from another thread let the kernel recycle the fd under a `recvfrom` still in
flight, and the reader then read from an unrelated file. What actually **returns** the reader is
`m_closing`, an atomic flag `waitReadable()` checks between poll slices: `shutdown()` is called too,
but it fails with `ENOTCONN` on an unconnected datagram socket everywhere except Linux, so it cannot
be relied on. See § *What macOS taught us* above. Do not delete the flag as redundant.

> [!WARNING]
> **Known hole, pre-existing and NOT fixed (2026-08-28)**: the wake-up only covers a reader parked in
> `select()`. A reader that cleared `select()` and is inside the blocking `recvfrom()`/`recvmsg()`
> holds the shared lock and nothing returns it — `close()` then waits on its exclusive lock
> indefinitely. Reachable: `SO_REUSEPORT` is always on and `UDP.receive()` is bound as an async
> method, so two concurrent receives on one socket both see "readable", one takes the datagram and
> the other blocks forever. The real fix is a non-blocking socket (`FIONBIO` / `O_NONBLOCK`) with
> `EWOULDBLOCK` treated as "no data" — it was scoped out of the macOS pass rather than bundled into
> it. Verified as a genuine mechanism, ruled pre-existing.

⚠️ **`ssdpDiscover()` refuses a search target holding a control character**: it is interpolated into
the `M-SEARCH` request line, so a `\r\n` in it emitted an attacker-shaped datagram from the host. Its
multicast TTL is set through `MulticastOptionValue`, the width each platform's own manual documents
(`u_char` on macOS/BSD, `int` on Linux, `DWORD` on Windows) — a rejected option leaves the TTL at 1
and discovery then misses every device more than one hop away. ⚠️ Note the earlier claim that an
`int` is *refused* on BSD/macOS did **not** reproduce on macOS 26 (that kernel accepts both widths);
the per-platform width is kept because the leniency is undocumented, not because it was observed.

**Platform**: Cross-platform (BSD sockets on Linux/macOS, Winsock on Windows). No external dependencies.

⚠️⚠️ **A return of 0 from `receive()` is AMBIGUOUS, and `timedOut` is the only thing that
disambiguates it (since 2026-08-28).** A zero-length datagram is legal in UDP and returns 0 exactly
like an expired wait. Before the flag existed the two were **byte-for-byte identical** through
app_system's JS path — same empty payload, same empty sender address, same port 0 — so a polling
consumer could not tell "nothing came" from "someone sent me an empty datagram". Two fixes:

- the plain overload takes an optional `bool * timedOut`, set true only when the wait ended with no
  datagram (a timeout **or** a concurrent `close()` — it does not separate those two, and does not
  pretend to: a closed socket is observable through `isOpen()` and the next call returns -1);
- `DatagramInfo` gained a `timedOut` member, and that overload now **always resets `info`**. It used
  to leave it untouched on the timeout path, so a caller reusing the struct read the *previous*
  datagram's sender as if it had just arrived.

Related, same sitting: the sender-address fill was gated on `bytesRead > 0`, so a legitimately
received zero-length datagram came back with an **empty sender**.

⚠️⚠️ **But `>= 0` alone is wrong too, and it invents an arrival — a return of 0 has TWO causes.**
A real zero-length datagram carries a sender the kernel filled in; a **wake-up** (a `close()` from
another thread making the descriptor readable) returns 0 with `sender` left **untouched**, i.e. still
zero-initialised. Filling unconditionally published that as *"a datagram from `0.0.0.0:0`"* — a
plausible-looking sender that never existed, reported as neither a timeout nor an error. That was a
regression introduced by the zero-length fix itself, and the sixth door onto the same class of bug in
one day; the Linux instance caught it.

`sin_port` is the discriminator: **port 0 is reserved and can never be a real UDP source**, so a
non-zero port means the kernel wrote the address. `m_closing` is the known cause of the other case,
but the structural check also covers any path where the stack returns 0 without filling the sender.
Both overloads now report that case as `timedOut` and leave the sender empty.

Measured, the three outcomes of a 0-byte return:

| Case | `timedOut` | sender |
|---|---|---|
| Timeout, no traffic | `true` | empty |
| Real zero-length datagram | `false` | `127.0.0.1:50186` |
| `close()` waking a parked `receive()` | `true` | empty |

⚠️ `receiveString()` **cannot** express the difference (both give an empty string) and says so in its
documentation. Use the buffer overload when it matters.

**The rule for every consumer**: poll on `timedOut`, never on "the payload is empty" — the same shape
as `TCPServer::accept()` returning `timedOut` rather than an error.

#### IPv4 multicast

IPv4 only — IPv6 (`ipv6_mreq` / `IPV6_JOIN_GROUP`) is deliberately out of scope.

**Every interface parameter is an interface ADDRESS, never a name**: `"192.168.1.42"`, not
`"eth0"`. This is the single most common misuse of the API. Obtain the candidates from
`NetworkInterfaces::enumerateMulticastCapable()`.

**Order**: `open()` → `bind()` → `joinMulticastGroup()`. Windows *requires* the socket to be
bound before `IP_ADD_MEMBERSHIP`; POSIX is permissive. Bind first everywhere.

**`joinMulticastGroup()` is idempotent.** The client records its memberships as
`(group, interface)` pairs. A repeat join returns `true` without touching the socket, where
the kernel would have answered `EADDRINUSE` — a consumer re-walking the interface list on a
timer can therefore re-join blindly. `leaveMulticastGroup()` on a group never joined is a
success too, for teardown paths that call it unconditionally. `close()` drops the list, in
step with the kernel which releases the memberships with the socket.

**`open()` enables `SO_REUSEPORT` on POSIX** (WinSock has no equivalent), alongside the
pre-existing `SO_REUSEADDR`. Both are read by `bind()` and inert afterwards — they are set at
creation time and **must never be moved next to the `bind()` call**. This is what allows
sharing a service port with a system daemon already holding it (mDNS on 5353).
⚠️ **Side effect on unicast**: two sockets bound to the same unicast port no longer collide
with `EADDRINUSE`; the kernel spreads incoming datagrams between them. A port conflict that
used to be a loud failure is now silent packet loss.

**Verified on Linux 6.x** (2026-08-26): binding 5353 next to a running `avahi-daemon`,
joining `224.0.0.251` on two NICs, and reading real DNS-SD answers from four LAN devices —
each datagram carrying a resolved interface index.
⚠️ **macOS is NOT verified.** It compiles (BSD path: `IP_RECVDSTADDR` + `IP_RECVIF`) but no
run was made. Two distinct risks, in order: sharing 5353 with the system `mDNSResponder`, and
**since macOS 15 (Sequoia) the Local Network privacy permission** — an app doing Bonjour or
multicast needs `NSLocalNetworkUsageDescription` and an entry under Privacy & Security, and
field reports show multicast working from a terminal but not from a double-clicked bundle.
Any macOS spike must therefore test a **signed, packaged binary**, never a console test.

---

### Network Interfaces (`Net::NetworkInterfaces`)

**File**: `NetworkInterfaces.hpp/.cpp`

Enumeration of the local IP addresses, **IPv4 and IPv6**, with the hardware address. Exists
because (a) the multicast API takes interface *addresses*, and (b) every scripting bridge
wants the Node.js `os.networkInterfaces()` shape — without this unit each consumer would
write its own `getifaddrs` / `GetAdaptersAddresses` `#ifdef` block (one downstream
application did, ~310 lines, now deleted): platform divergence leaking downstream, which the
co-development doctrine forbids.

**API**:
```cpp
namespace EmEn::Net::NetworkInterfaces
{
    enum class AddressFamily : uint8_t { IPv4, IPv6 };

    constexpr uint8_t PrefixLengthUnknown{0xFF};

    struct Interface {
        std::string name;              // "eth0", "en0", "Ethernet 2" — informative only
        std::string address;           // dotted-decimal or RFC 5952; THIS is what the multicast API wants (IPv4)
        std::string netmask;           // same notation as the address, empty when not reported
        std::string mac;               // lowercase "aa:bb:cc:dd:ee:ff", EMPTY when none (loopback, tunnels)
        uint32_t index{0};
        uint32_t scopeId{0};           // IPv6 sin6_scope_id, always 0 for IPv4
        uint8_t prefixLength{PrefixLengthUnknown};   // CIDR length derived from the netmask
        AddressFamily family{AddressFamily::IPv4};
        bool loopback{false};
        bool up{false};
        bool multicastCapable{false};
    };

    std::vector< Interface > enumerate () noexcept;                  // IPv4 + IPv6
    std::vector< Interface > enumerateMulticastCapable () noexcept;  // IPv4 only, up, multicast-capable
    const char * to_cstring (AddressFamily) noexcept;                // "IPv4" / "IPv6"
}
```

One entry **per address**: an interface holding several addresses (typically one IPv4 plus
one link-local IPv6) yields several entries sharing name, index and MAC. The shape is the
flat Node.js entry on purpose, so a bridge only renames fields (`loopback` → `internal`,
`to_cstring(family)` → `family`, `address + "/" + prefixLength` → `cidr`) and never
re-enumerates.

**Contract on absence, deliberately different from Node.js**: `mac` is **empty** when the OS
reports no hardware address — Linux hands out an all-zero 6-byte `AF_PACKET` address for
`lo`, and it is folded to empty too. The `"00:00:00:00:00:00"` placeholder Node prints belongs
to the bridge, not to the engine. `prefixLength` is `PrefixLengthUnknown` when there is no
netmask **or when the mask is non-contiguous** (no CIDR form exists), never a misleading count.

**Platform**: `getifaddrs()` on Linux/macOS — the MAC comes from the `AF_PACKET` (Linux) /
`AF_LINK` (BSD, macOS) entry of the same name, collected in a first pass;
`GetAdaptersAddresses(AF_UNSPEC)` on Windows (links `Iphlpapi`), MAC from `PhysicalAddress`,
netmask **derived** from `OnLinkPrefixLength` so both fields are filled like on POSIX, index
read per family (`IfIndex` vs `Ipv6IfIndex`). Unlike `SerialPort` and `WiFiScanner`, this
unit is **not** split per OS — a single TU with one Windows branch and a Linux/BSD
sub-branch for the hardware-address family.

**Verified (Linux 6.x, 2026-08-27, re-run 2026-08-28)** with an out-of-tree binary compiled
straight from `NetworkInterfaces.cpp`: 3 interfaces × 2 families, `/8` `/24` `/64` `/128`
prefixes, IPv6 scope ids equal to the interface index, MACs on both NICs, empty MAC on `lo`,
`enumerateMulticastCapable()` returning the two NICs **and `lo`** since the loopback fix below.
⚠️ **macOS and Windows: compile-only** for the IPv6 and MAC paths (`AF_LINK`, `AF_UNSPEC`),
same standing as the multicast surface — see `docs/todo/udp-multicast-macos-verification.md`.

**Traps**:
- ⚠️ On Linux, **loopback carries no `IFF_MULTICAST` flag** (`lo` is `<LOOPBACK,UP,LOWER_UP>`
  where macOS `lo0` does carry it) — **and the flag is wrong**: the kernel supports multicast on
  `lo` perfectly well. Measured 2026-08-28: `IP_ADD_MEMBERSHIP` and `IP_MULTICAST_IF` on
  `127.0.0.1` are both accepted, and the datagram makes the round trip. Until that date
  `enumerateMulticastCapable()` filtered on the flag alone, so `lo` was dropped **on Linux only**,
  in contradiction with its own documented contract ("loopback is deliberately kept"); on a
  machine with no link the list came back **empty**, and every "join on each interface" loop then
  did nothing at all without reporting an error. The filter now exempts loopback from the flag,
  `#if defined(__linux__)` only. **Never infer multicast support on Linux from `IFF_MULTICAST`.**
  macOS needs no exemption (flag present). ❓ **Windows is deliberately left alone**: neither the
  flag its loopback pseudo-interface reports (`IP_ADAPTER_NO_MULTICAST`) nor the outcome of a join
  on `127.0.0.1` has been measured, and a non-joinable entry in that list is exactly the dead-
  interface trap the Windows run documented. Measure before widening the exemption.
- ⚠️ `up` requires `IFF_UP` **and** `IFF_RUNNING`: a NIC with no cable is up but not running,
  and joining a group on it buys nothing.
- ⚠️ The result is a snapshot. Interfaces appear and vanish at runtime (VPN, hotplug,
  container bridges): a consumer tracking them must poll and diff.
- ⚠️ The kernel caps memberships per socket — **20 by default on Linux**
  (`net.ipv4.igmp_max_memberships`). Joining every interface of a host running containers or
  VPNs can reach the cap, and the join then fails.
- ⚠️ Feed the multicast API with `enumerateMulticastCapable()`, never with a filter you wrote
  over `enumerate()`: the IPv6 entries carry addresses `joinMulticastGroup()` cannot parse.

---

### TCP Client (`Net::TCPClient`)

**File**: `TCPClient.hpp/.cpp`

Cross-platform stateful TCP client built on Asio. Exposes a simple blocking-with-timeout API so the consumer never has to deal with the Asio model directly. Designed for printer protocols (MKS, Chitu), RTSP control channels, LAN gaming, and any application needing a stateful TCP connection.

**API**:
```cpp
namespace EmEn::Net
{
    class TCPClient final {
    public:
        // Lifecycle
        TCPClient () noexcept;
        ~TCPClient () noexcept;
        // Movable, non-copyable.

        // Connection
        bool connect (const std::string & host, uint16_t port,
                      uint32_t timeoutMs = 5000) noexcept;
        void close () noexcept;
        bool isConnected () const noexcept;

        // Data transfer
        int  send (const void * data, size_t length) noexcept;
        int  send (const std::string & data) noexcept;
        int  receive (void * buffer, size_t maxLength,
                      uint32_t timeoutMs = 0) noexcept;          // 0 = block forever
        std::string receiveString (size_t maxLength,
                                   uint32_t timeoutMs = 0) noexcept;

        // Address queries
        bool getLocalAddress  (std::string & address, uint16_t & port) const noexcept;
        bool getRemoteAddress (std::string & address, uint16_t & port) const noexcept;

        // Socket options (long-lived sessions, low-latency traffic)
        bool setNoDelay     (bool enable) noexcept;                // TCP_NODELAY
        bool setKeepAlive   (bool enable,
                             uint32_t initialDelaySeconds = 7200) noexcept;
        bool setRecvTimeout (uint32_t timeoutMs) noexcept;         // SO_RCVTIMEO
        bool setSendTimeout (uint32_t timeoutMs) noexcept;         // SO_SNDTIMEO

        // Last error (Node.js-like error code mapping in wrapping layers)
        std::error_code lastError () const noexcept;
    };
}
```

**Design**:
- **Asio is used only during `connect()`** — for DNS resolution (`asio::ip::tcp::resolver`) and a timed `async_connect` + `io_context::run_for(timeout)`. Once the connection is established, the kernel handle is detached from Asio via `socket->release()`, switched to blocking mode (Asio leaves it non-blocking for its reactor / IOCP), SIGPIPE is suppressed where needed (`SO_NOSIGPIPE` on macOS, `MSG_NOSIGNAL` per `send()` call on Linux, n/a on Windows), and stored as a raw `native_handle_type` (`std::intptr_t`).
- **Runtime I/O (`send`, `receive`, `close`) bypasses Asio completely** and uses `::send()` / `::recv()` / `::shutdown()` / `::closesocket()` / `::close()` directly. The kernel guarantees that concurrent `recv()` and `send()` on the same socket are atomic — what Asio cannot guarantee at the userland level (its `win_iocp_socket_service` impl state is documented as *Shared objects: Unsafe*) the kernel provides natively.
- **Per-call recv timeout** is enforced via `SO_RCVTIMEO` (save/restore around each `receive()`), so a prior `setRecvTimeout()` configuration survives the call.
- **Close is two-phase** under a per-instance `std::shared_mutex`:
  1. Phase 1 — shared lock + `shutdown(SHUT_RDWR)`: wakes up any thread currently blocked in `::recv()` / `::send()` on this handle (kernel returns 0/EOF or `EPIPE`). The handle stays valid here.
  2. Phase 2 — exclusive lock + `closesocket()`: blocks until every in-flight `send`/`receive` has released its shared lock, then invalidates the handle. This eliminates the close-during-receive race (where the kernel could reuse the freed handle behind a syscall still in progress on another thread).
- **Multiple `TCPClient` instances** are fully independent and safe to use concurrently from different threads.
- **One single instance** is safe for full-duplex use from two threads (one `send` thread, one `receive`-polling thread) — this is the canonical pattern for printer sessions, RTSP control, and the WebModule `TCP.client.*` bindings in AppSystem.
- Hostname resolution supports both IPv4 and IPv6 endpoints.
- Move-only via `std::unique_ptr< std::shared_mutex >` + raw handle.

⚠️ **`receive()` returns 0 for a timeout AND at the end of stream** — `peerClosed()` (2026-08-27)
tells them apart. Without it a polling consumer could never learn the peer had gone: it polled
forever, leaking its job and the descriptor.

⚠️ **`TCPServer::accept()` returns `std::nullopt` for a timeout and for a failure** —
`lastAcceptTimedOut()` tells them apart, and `lastError()` is cleared on entry: a stale error used to
be re-reported on every subsequent timeout, i.e. several spurious errors per second in a 200 ms
accept loop.

#### Why we bypass Asio for the runtime I/O path

> **Asio is not "bad" — its thread-safety model is just very precise.** The official doc says *"Distinct objects: Safe. Shared objects: Unsafe."*: two threads can use two different sockets in parallel, but never the same socket simultaneously, even when one thread reads and the other writes. The intended way to do full-duplex with Asio is **one** I/O thread driving `io_context::run()`, with every operation posted via `asio::strand` (which serialises completion handlers on that thread). Application threads doing send/recv post a lambda onto the strand and wait on a `std::future` if they need a synchronous answer. This is how Boost.Beast, gRPC and Crow are built.
>
> We do *not* use that "Asio correctly" pattern here for two reasons:
>
> 1. **Use case fit.** The TCPClient is a 1-N persistent connection holder (printer, RTSP, LAN-game) — not a 10k-concurrent-connection server. The strand pattern adds significant scaffolding (dedicated I/O thread per `TCPClient`, every public method becoming `post + future::get`, completion handler types, timer-based timeouts) for a single connection where the kernel's native `recv`/`send` atomicity is already sufficient.
> 2. **Subtle Windows-IOCP failure mode.** A previous iteration of this class used Asio's *synchronous* API on the same socket from two threads (one thread in `socket.read_some(..., ec)`, another in `asio::write(socket, ...)`). The sync API looks like a thin wrapper around BSD `recv`/`send`, so the expectation is "the kernel makes this safe". In reality, every sync Asio call touches **userland Asio state** (non-blocking flag toggling, `cancel_token_`, `win_iocp_socket_service::implementation_type` fields) without internal locking. On Linux/macOS the reactor's state is small and races silently without crashing. On Windows IOCP the state is much richer, and the race lands somewhere fatal — release builds reveal it because the optimiser does not sequence the accesses, debug builds hide it through timing.
>
> The kernel-level concurrency contract (`recv()` and `send()` on the same fd are atomic and reentrant — POSIX `read(2)`/`write(2)`, Winsock `WSARecv`/`WSASend`) is the only thing we need for the polling-receive + sender pattern. Going raw means **less code, no Asio userland race surface, identical behaviour across all three OS at the syscall level**. The trade-off is that we re-implement the small bit of glue Asio would otherwise provide (DNS resolution + timed connect), which is exactly why we keep Asio for `connect()` only.
>
> **If you ever need many concurrent connections** (genuine TCP server with hundreds of peers, parallel HTTP clients, etc.), switch to the Asio strand pattern: one io_context, dedicated I/O thread, one strand per connection, async ops only. Don't extend this class — model the new one on Boost.Beast's `tcp_stream` and use proper async composed operations.

**Platform-specific socket options**:
- `TCP_NODELAY`, `SO_KEEPALIVE`, `SO_RCVTIMEO`, `SO_SNDTIMEO` — universal (Linux, macOS, Windows).
- Initial keep-alive delay — Linux uses `TCP_KEEPIDLE`, macOS uses `TCP_KEEPALIVE`, Windows uses `WSAIoctl(SIO_KEEPALIVE_VALS, ...)`. Best-effort; granularity is platform-specific.

**Recommended use cases**:
- **Printer sessions** (MKS, Chitu): `setKeepAlive(true, 60)` to detect NAT timeouts.
- **RTSP control + game packets**: `setNoDelay(true)` to disable Nagle.
- **Long-running connections**: `setTimeout(timeoutMs)` to detect a frozen peer in seconds rather than minutes.

---

### TCP Server (`Net::TCPServer`)

**File**: `TCPServer.hpp/.cpp`

Cross-platform TCP server built on Asio. Exposes a simple blocking-with-timeout `accept()` that returns a fully-owned `TCPClient`. Each accepted client owns its own io_context, so the server's lifetime is independent from the lifetime of the clients it produced.

**API**:
```cpp
namespace EmEn::Net
{
    class TCPServer final {
    public:
        static const int DefaultBacklog;       // = asio::socket_base::max_listen_connections

        // Lifecycle
        TCPServer () noexcept;
        ~TCPServer () noexcept;
        // Movable, non-copyable.

        // Listening
        bool listen (uint16_t port,
                     int backlog = DefaultBacklog,
                     const std::string & address = {}) noexcept;
        void close () noexcept;
        bool isListening () const noexcept;

        // Accept (blocks up to timeoutMs, 0 = block forever)
        std::optional< TCPClient > accept (uint32_t timeoutMs = 0) noexcept;

        // Address query (recovers OS-assigned port if listen() was called with port = 0)
        bool getLocalAddress (std::string & address, uint16_t & port) const noexcept;

        std::error_code lastError () const noexcept;
    };
}
```

**Design**:
- The server holds a single `asio::io_context` + `asio::ip::tcp::acceptor`.
- `listen(port = 0, ...)` lets the OS pick a free port — recover it via `getLocalAddress()`.
- `accept(timeoutMs)` uses `async_accept` + `run_for(timeout)` and returns `std::nullopt` on timeout/error.
- On a successful accept, the underlying socket is **detached** from the server's io_context (`socket.release()`) and **migrated** onto a fresh io_context owned by the returned `TCPClient` (`socket.assign()`). This decouples the lifetimes — destroying the server does not affect already-accepted clients.
- `SO_REUSEADDR` is enabled on a best-effort basis.
- **Binding `"::"` asks for a dual-stack socket EXPLICITLY** (`IPV6_V6ONLY` off), since 2026-08-28.
  ⚠️ Read the honest version of why: it corrects **nothing observable**. No platform exhibited the
  "IPv4 peers silently refused" symptom on this path — Windows included, measured, because Asio
  clears that option itself on every `AF_INET6` socket; Linux and macOS default their sysctl
  (`bindv6only` / `net.inet6.ip6.v6only`) to 0. What the call buys is independence from that Asio
  internal, which is not ours to rely on, and an intent stated in our own code. Unlike
  `SO_REUSEADDR` it is **not** best-effort: a stack refusing it fails the `listen()`, because a
  socket that is up while invisible to half the network is the outcome worth refusing outright.
  Bind `"0.0.0.0"` for a deliberate IPv4-only listen.
- ⚠️⚠️ **`close()` drains the io_context, and `accept()` never reports `operation_aborted`.** Both
  since 2026-08-28, and each guards a different half of the same defect. `acceptor->cancel()` only
  *schedules* the pending `async_accept` handler with `operation_aborted`; it does not run it. That
  handler used to survive in the shared io_context and be executed by the **next** `accept()`,
  before its own — which then read a stale cancellation as a real error. Consequence, measured on
  Linux through the dev-check TCP card: calling `listen()` a second time on the same server (which
  closes internally first) left it **permanently unable to accept**. `isListening()` answered
  `true`, the binding reported `Listening 0.0.0.0:<port>`, and every single `accept()` failed with
  `operation_aborted` forever — a server that is up and can never take a client. `close()` now
  drains, and `accept()` treats `operation_aborted` as "no peer this round" (`timedOut`) rather than
  an error, since that code is *always* the completion of a cancel the class issued itself and is
  never the caller's business. ⚠️ **The rule this implies, stated exhaustively — an earlier
  wording of it ("key on `timedOut`/`notListening`, never on an error") had a hole a consumer could
  fall through: continue ONLY on `timedOut`. Stop on a peer, stop on `notListening`, stop on an
  error, and stop on anything you do not recognise.** Four separate silent spins in this cascade all
  had the same shape — the consumer re-armed on an outcome it had not matched — so the loop must be
  exhaustive by construction rather than gain one more branch per defect found.

**Expected usage pattern**:
```cpp
EmEn::Net::TCPServer server;
if ( !server.listen(7777) ) {
    // Inspect server.lastError()
    return;
}

std::atomic< bool > running{true};

std::thread acceptThread([&] {
    while ( running ) {
        auto client = server.accept(/* timeoutMs = */ 200);   // Short timeout for graceful shutdown
        if ( !client ) {
            continue;
        }

        client->setNoDelay(true);   // Game / RTSP traffic
        // Hand off to a connection manager / per-client thread.
        handleClient(std::move(*client));
    }
});

// ... main loop ...

running = false;
server.close();   // Cancels the pending accept; the thread sees nullopt and exits.
acceptThread.join();
```

**Recommended use cases**:
- **Local MQTT broker** (Chitu WiFi printers): one accept loop, one socket per client.
- **LAN game server**: 4-8 player FPS — trivially handled by a thread-per-client or shared poll loop.
- **Remote console / debug listeners**: see `RemoteListener` for a higher-level Asio-async variant when you need concurrent multi-client broadcast.

**Platform**: Cross-platform via Asio (any OS that compiles standalone Asio with `ASIO_NO_EXCEPTIONS`).

---

### Serial Port (`Net::SerialPort`)

**Files**: `SerialPort.hpp` + `SerialPort.{linux,mac,windows}.cpp`

Full cross-platform serial port abstraction: enumeration, open/close, read/write with timeout, flow control.

**API**:
```cpp
namespace EmEn::Net
{
    struct SerialPortInfo {
        std::string path;            // "/dev/ttyUSB0", "COM3"
        std::string manufacturer;
        std::string serialNumber;
        std::string pnpId;
        std::string locationId;
        uint16_t vendorId{0};        // USB VID
        uint16_t productId{0};       // USB PID
    };

    struct SerialPortConfig {
        uint32_t baudRate{9600};
        uint8_t dataBits{8};
        uint8_t stopBits{1};
        char parity{'N'};            // 'N', 'E', 'O'
        bool rtscts{false};
        bool xon{false};
        bool xoff{false};
    };

    class SerialPort final {
    public:
        static std::vector< SerialPortInfo > listPorts () noexcept;
        bool open (const std::string & path, const SerialPortConfig & config = {}) noexcept;
        void close () noexcept;
        bool isOpen () const noexcept;
        int write (const void * data, size_t length) noexcept;
        int write (const std::string & data) noexcept;
        int read (void * buffer, size_t maxLength, uint32_t timeoutMs = 0) noexcept;
        std::string readString (size_t maxLength = 4096, uint32_t timeoutMs = 0) noexcept;
        const std::string & path () const noexcept;
    };
}
```

⚠️ **Enumeration cannot abort (2026-08-27).** Every `std::filesystem` call takes its `error_code`
overload and the USB ids are read with `from_chars`: the throwing forms terminate the process in a
`noexcept` function, and the walk races with the user — a USB adapter unplugged between the iteration
and `canonical()` used to kill the application.

⚠️ **A baud rate with no POSIX constant is applied, not silently downgraded.** 250000 — the default of
Marlin-based 3D printers — has no `B250000`; it goes through `TCSETS2` + `BOTHER` (the kernel's
`termios2`, whose layout is mirrored in the TU because `<asm/termbits.h>` collides with `<termios.h>`;
the rebuilt ioctl numbers were checked against the kernel's). A rate the driver refuses now fails
`open()` instead of running at 9600 with unreadable replies.

**macOS got the same contract on 2026-08-28**, through `ioctl(IOSSIOSPEED)` (`<IOKit/serial/ioss.h>`,
applied **after** `tcsetattr`, which would otherwise undo it) — it stops at `B230400` where Linux
carries constants to `B4000000`, so 250000 used to hit the silent `B9600` fallback there. ⚠️ Never
exercised against a real adapter: a pty refuses `IOSSIOSPEED` for every rate, so only the *failure*
branch has run.

**Windows needs nothing here** (checked 2026-08-28): `SerialPort.windows.cpp` assigns
`dcb.BaudRate = config.baudRate` — an arbitrary `DWORD`, no `CBR_*` table — so the silent-fallback
class of bug that hit both POSIX legs cannot occur. Untested against hardware, but there is no
lookup table to be wrong.

⚠️ `TIOCEXCL` (claiming the port, so a second process cannot corrupt the stream mid-print) is
**Linux-only** — `SerialPort.mac.mm` does not do it, though the ioctl exists on macOS. Deliberately
left alone: exclusive open is a behaviour change, not a bug fix.

**Platform details**:
| Platform | Enumeration | I/O | Dependencies |
|----------|-------------|-----|--------------|
| Linux | `/sys/class/tty` + sysfs | POSIX termios + select | None (kernel APIs) |
| macOS | IOKit (IOSerialKeys, IOUSBLib) | POSIX termios | IOKit framework |
| Windows | SetupAPI + GUID_DEVCLASS_PORTS | CreateFile + DCB | SetupAPI.lib |

**Design**: RAII (closes in destructor), movable, non-copyable, all functions `noexcept`.

---

### WiFi Scanner (`Net::WiFiScanner`)

**Files**: `WiFiScanner.hpp` + `WiFiScanner.{linux,mac,windows}.cpp`

Cross-platform WiFi network enumeration and current connection query.

**API**:
```cpp
namespace EmEn::Net::WiFiScanner
{
    struct Network {
        std::string ssid;
        std::string bssid;
        int32_t signalLevel{0};     // dBm (e.g., -50)
        int32_t quality{0};         // 0-100%
        uint32_t frequency{0};      // MHz
        int32_t channel{0};
        std::string security;       // "WPA2", "WPA3", "Open", etc.
        std::string mode;           // "Infra", "Ad-Hoc"
    };

    [[nodiscard]] std::vector< Network > scan () noexcept;
    [[nodiscard]] std::vector< Network > getCurrentConnections () noexcept;
}
```

**Platform details**:
| Platform | Scan method | Dependencies |
|----------|-------------|--------------|
| Linux | `nmcli` (NetworkManager CLI) | Requires NetworkManager |
| macOS | CoreWLAN framework (CWWiFiClient) | CoreWLAN.framework |
| Windows | WLAN API (WlanScan, WlanGetNetworkBssList) | wlanapi.lib |

**Note**: Windows was migrated from `netsh` shell parsing to the native WLAN API for reliability and performance.

⚠️ **The Linux parser cannot be killed by a neighbour's SSID (2026-08-27).** nmcli's terse output
escapes **both** `:` and `\` with a backslash: scanning for `\:` alone mis-read an SSID ending with a
backslash, every field shifted left, and a text field reached `std::stoi` — `std::terminate` in a
`noexcept` function. Unescaping is now a single left-to-right pass and the numbers are read with
`from_chars`; a malformed line yields zeros instead of dying.

---

## Critical Points

- **The download manager completes on the main thread, one cycle later, always through
  `FileDownloaded`.** Never special-case a cache hit in a consumer, never expect the callback
  inside `download()`, never call `notify()` from a worker — push an event and let
  `dispatchCompleted()` emit it. See § Download manager.
- **Hardware utilities are standalone**: `UDPClient`, `NetworkInterfaces`, `SerialPort`,
  `WiFiScanner` have no dependency on `Net::Manager` nor on Asio. `TCPClient`/`TCPServer` use
  Asio internally (connect / accept only) behind a blocking-with-timeout API — consumers never
  see Asio types, and since 2026-08-27 **`TCPServer.hpp` no longer includes `asio.hpp`** (the
  io_context and the acceptor live in a private `Impl` defined in the TU).
- **noexcept everywhere**: all hardware utility functions are noexcept, errors return
  false/empty containers (Asio is configured with `ASIO_NO_EXCEPTIONS`). ⚠️ That guarantee is
  honest because every Asio call in the cascade uses the `error_code` overloads — the legacy
  `Base::Network::download()`, which used the throwing ones (abort on a DNS failure), was removed
  from emeraude-base on 2026-08-27.
- ⚠️⚠️ **Socket options that feed `bind()` or `recvmsg()` must be set at `open()` time, never lazily.** Measured on Linux 6.x: arming `IP_PKTINFO` at the first `receive(DatagramInfo)` instead of at `open()` still yields a correct destination address — it sits in the IP header — but the receiving interface index comes back as **0** for any datagram already queued. The structure looks perfectly plausible and the index is silently wrong, which is exactly what breaks per-interface attribution on a multi-homed host. Same family as `SO_REUSEADDR`/`SO_REUSEPORT`, which `bind()` reads once and ignores forever after.
- ⚠️⚠️ **Never stream a `std::filesystem::path` into a trace — on Windows it is a `terminate`, not
  a cosmetic issue.** `path::operator<<` emits `quoted(p.string())`, and `string()` throws on MS-STL
  for content the ANSI code page cannot represent; the whole cascade is built `-fno-exceptions`, so
  the throw becomes an abort. The cache lives under the user's profile, so a Windows account name
  with a non-ANSI character was enough: `Manager.cpp` had **four** such sites, one of them on the
  init path (`isDirectoryUsable` failing), i.e. a crash at startup rather than during a download.
  Fixed 2026-08-28 by wrapping each in `IO::toU8String()`. Reported from Windows, found and fixed on
  Linux — a green Linux run can never surface this class of defect, only a reading of the code can.
  Side effect worth knowing: `operator<<` supplied its own quotes, so those messages used to be
  doubly quoted; they now carry only the ones the format string writes.
- ⚠️ **A green compile proves nothing about a socket.** The multicast surface and the
  IPv4/IPv6/MAC enumeration were validated by out-of-tree binaries compiled straight from
  `UDPClient.cpp` / `NetworkInterfaces.cpp` (they depend on nothing but `emeraude_export.hpp`),
  doing a real round-trip, a real DNS-SD exchange, a real enumeration. Reuse that technique
  rather than trusting the build.

## Important Files

- `Manager.cpp/.hpp` + `Manager.console.cpp`, `DownloadItem.hpp`, `Types.hpp` - Download manager (HTTPS, URL-keyed cache, main-thread notifications)
- `APIClient.cpp/.hpp` + `APIClient.console.cpp`, `APIRequestItem.hpp`, `Types.hpp` - Web API client (arbitrary HTTPS exchanges, in-memory responses, ticket retention)
- `UDPClient.hpp/.cpp` - UDP client, IPv4 multicast and SSDP discovery
- `NetworkInterfaces.hpp/.cpp` - Local IPv4/IPv6 address enumeration with MAC (feeds the multicast API and scripting bridges)
- `TCPClient.hpp/.cpp` - TCP client (Asio-based, blocking-with-timeout API)
- `TCPServer.hpp/.cpp` - TCP server (Asio-based, accept returns owned TCPClient)
- `SerialPort.hpp` + `.{linux,mac,windows}.cpp` - Serial port abstraction
- `WiFiScanner.hpp` + `.{linux,mac,windows}.cpp` - WiFi scanning
- Download cache: `cacheDirectory("downloads")/<hash>.<ext>` + `downloads/index.json` — owned by the manager, the only one

## Detailed Documentation

Related systems:
- @src/Resources/AGENTS.md - Fail-safe loading system, `ExternalData` source type, the `ServiceAccess` firewall
- @docs/resource-management.md - Resources architecture
- emeraude-base `src/Network/` (`HTTPSClient`, `TLSConnection`, `TrustStore`, `URI`) - the HTTPS stack the manager runs on
- emeraude-base `src/Network/HTTPSClient.hpp` `DownloadProgress` - the hook behind the Progress notification
- emeraude-base `src/Network/HTTPSClient.hpp` `HTTPRequestOptions` / `request()` / `isRequestHeaderAcceptable()` - the API-traffic entry point `Net::APIClient` runs on
