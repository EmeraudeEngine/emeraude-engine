# Net System

Context for the Emeraude Engine `src/Net/` directory: the hardware & discovery utilities (production surface) and the resource download manager (semi-stub).

## Module Overview

Two unrelated things live under `src/Net/`:

1. **`Net::Manager`, the resource download manager** — a `ServiceInterface` owned by
   `PrimaryServices`, meant to fetch `ExternalData` resources for the `Resources` system.
   ⚠️ **It is a semi-stub: the chain does not work end to end and has no consumer.** Read the
   section below before touching it or believing any older description of it.
2. **The hardware & discovery utilities** (`UDPClient`, `NetworkInterfaces`, `TCPClient`,
   `TCPServer`, `SerialPort`, `WiFiScanner`) — standalone, `noexcept`, cross-platform classes
   with no dependency on the manager. These are the **production surface** of this directory:
   they are consumed by downstream applications through scripting bridges (TCP/UDP/serial/WiFi
   modules). Documented in § Hardware & Discovery Utilities.

**Not multiplayer**: nothing here is gameplay networking, and nothing should become it without
an owner decision.

## Download manager (`Net::Manager`) — real state, 2026-08-27

**Files**: `Manager.hpp/.cpp`, `DownloadItem.hpp`, `CachedDownloadItem.hpp/.cpp`, `Types.hpp`.

**Identity**: `class Manager final : public ServiceInterface, public Base::ObservableTrait`,
`ClassId = "Net::ManagerService"`. It is **not** a `Console::ControllableTrait` — no console
command reaches it. Constructed in `PrimaryServices` with the `FileSystem` and the shared
`Base::ThreadPool`; `Core` observes it (`Core.cpp`, `onNotification`) with a handler whose
every `case` is an empty `break`.

**Real API** (header-verified — the methods older docs advertised, `requestDownload()`,
`isCached()`, `clearCache()`, `forceDownload()`, **do not exist**):
```cpp
int download (const Network::URL & url, const std::filesystem::path & output, bool replaceExistingFile) noexcept; // returns a ticket
DownloadStatus downloadStatus (int ticket) const noexcept;
size_t fileCount () const noexcept;
size_t fileRemainingCount () const noexcept;
size_t totalBytesTotal () const noexcept;   // accumulators, see header for exact names
void showProgressionInConsole (bool state) noexcept;
enum class NotificationCode { DownloadingStarted, FileDownloaded, DownloadingFinished, Progress };
```

**Transport**: `download()` enqueues **one lambda per request on the shared thread pool**
(no thread of its own, no `io_context`) which calls the **legacy** emeraude-base free function
`Base::Network::download(uri, path, verbose)` — plaintext HTTP over Asio. The engine
references **none** of emeraude-base's production-grade HTTPS stack (`HTTPSClient`,
`TLSConnection`, `TrustStore`), so:
- an `https://` URL resolves port 443 and then **speaks cleartext** to it — HTTPS cannot work;
- the legacy function uses the throwing Asio overloads under `ASIO_NO_EXCEPTIONS`: a DNS
  failure is a **`std::abort()`**, inside a function declared `noexcept`;
- there is no timeout, no retry, no redirect following, no chunked transfer.

**Verified defects (read the code, not this list, before fixing)**:
| Where | What |
|---|---|
| `grep notify src/Net/` → nothing | **No notification is ever emitted.** The four `NotificationCode`s are dead; `DownloadItem::setStatus()`/`setProgression()` are never called, every item stays `Pending` forever. |
| `Manager.cpp`, the pool lambda | Passes `true` as the third argument of `Base::Network::download()` — that parameter is `verbose`, not `replaceExistingFile`. The flag the caller asked for is dropped. |
| `Manager::download()` | `m_downloadItems.emplace_back()` without a mutex while pool workers hold `const auto & item = m_downloadItems.at(ticket)` — a reallocation dangles the worker's reference. |
| `Manager::download()` | Returns ticket `0` for "already queued", indistinguishable from a real ticket 0 and colliding with `Resources::ServiceAccess::DownloadCacheHit == 0`. |
| Cache | `m_downloadCache` is filled only from `downloads_db.json` at startup; `download()` never inserts. `clearDownloadCache()` is private and never called. `m_downloadEnabled`, `m_showProgression`, `m_nextCacheItemId` are written, never read. |

**Cache on disk**: `FileSystem::cacheDirectory("downloads")` for the files (`dlcached_<id>`),
`cacheDirectory("downloads_db.json")` for the index (written **next to** the directory, not
inside). ⚠️ `Resources` uses a **different** scheme — `cacheDirectory()/data/<ResourceClassId>/<filename>`
(`LoadingRequest::cacheFilepath()`). The two never meet.

### Resources integration — what exists and where it breaks

A resource is downloadable only when its store entry says so: **`"Source": "ExternalData"`**
(`BaseInformation::parseSource`); the data string is then validated with `Network::URL::isURL()`.
⚠️ There is **no URL sniffing on the resource name** — `getResource("https://…")` does nothing
special, whatever older docs said.

Chain: `Container` observes the manager in its constructor → on an async load of an
`ExternalData` resource calls `ServiceAccess::startDownload()` and parks the request under its
ticket → `Container::onNotification()` waits for `FileDownloaded`, checks `downloadStatus()`,
then enqueues the loading task. It breaks in four places:
1. `FileDownloaded` is never emitted (above) → **an `ExternalData` resource never loads**.
2. Cache hit (`DownloadCacheHit`): the request is marked processed and **no loading task is
   enqueued** (`Container.hpp`, comment "fix separately").
3. `DownloadStatus::Error` is handled by `markDownloadProcessed(..., true)`: **a failure
   rewrites the `BaseInformation` to a cache file that was never written** (`FIXME` in code).
4. `Core/Resources/DownloadEnabled` (`SettingKeys.hpp`) is read into
   `Resources::Manager::m_downloadingAllowed`, which **nothing consults**. The setting is inert.

**Consumers**: none in the engine beyond this dead chain; none in any downstream application
(they download from their scripting layer). One test application binds a debug key to
`netManager().download()` with hard-coded paths — not a use case.

**Fate**: an **open owner decision**, tracked in
[`docs/todo/net-manager-download-chain-broken.md`](../../docs/todo/net-manager-download-chain-broken.md)
— either rewire onto `Base::Network::HTTPSClient` and repair the chain, or remove the manager,
the `ExternalData` path and the legacy `Network.cpp`. **Do not build on the manager, and do not
write a workaround in a consumer, until that decision is taken.**

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
                     uint16_t & senderPort, uint32_t timeoutMs = 0) noexcept;
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

**Platform**: Cross-platform (BSD sockets on Linux/macOS, Winsock on Windows). No external dependencies.

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

**Verified (Linux 6.x, 2026-08-27)** with an out-of-tree binary compiled straight from
`NetworkInterfaces.cpp`: 3 interfaces × 2 families, `/8` `/24` `/64` `/128` prefixes, IPv6
scope ids equal to the interface index, MACs on both NICs, empty MAC on `lo`,
`enumerateMulticastCapable()` returning the two NICs' IPv4 addresses only.
⚠️ **macOS and Windows: compile-only** for the IPv6 and MAC paths (`AF_LINK`, `AF_UNSPEC`),
same standing as the multicast surface — see `docs/todo/udp-multicast-macos-verification.md`.

**Traps**:
- ⚠️ On Linux, **loopback carries no `IFF_MULTICAST` flag**. `lo` is therefore absent from
  `enumerateMulticastCapable()`, and a multicast self-test on one machine must go through a
  real NIC with `setMulticastLoopback(true)`.
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

---

## Critical Points

- **The hardware utilities are the production surface; the download manager is not.** See
  § Download manager for its real state before touching `Resources`' `ExternalData` path.
- **Hardware utilities are standalone**: `UDPClient`, `NetworkInterfaces`, `SerialPort`,
  `WiFiScanner` have no dependency on `Net::Manager` nor on Asio. `TCPClient`/`TCPServer` use
  Asio internally (connect / accept only) behind a blocking-with-timeout API — consumers never
  see Asio types, and since 2026-08-27 **`TCPServer.hpp` no longer includes `asio.hpp`** (the
  io_context and the acceptor live in a private `Impl` defined in the TU).
- **noexcept everywhere**: all hardware utility functions are noexcept, errors return
  false/empty containers (Asio is configured with `ASIO_NO_EXCEPTIONS`). ⚠️ That guarantee is
  honest only where the `error_code` overloads are used — the legacy `Base::Network::download()`
  the manager calls is not (see above).
- ⚠️⚠️ **Socket options that feed `bind()` or `recvmsg()` must be set at `open()` time, never lazily.** Measured on Linux 6.x: arming `IP_PKTINFO` at the first `receive(DatagramInfo)` instead of at `open()` still yields a correct destination address — it sits in the IP header — but the receiving interface index comes back as **0** for any datagram already queued. The structure looks perfectly plausible and the index is silently wrong, which is exactly what breaks per-interface attribution on a multi-homed host. Same family as `SO_REUSEADDR`/`SO_REUSEPORT`, which `bind()` reads once and ignores forever after.
- ⚠️ **A green compile proves nothing about a socket.** The multicast surface and the
  IPv4/IPv6/MAC enumeration were validated by out-of-tree binaries compiled straight from
  `UDPClient.cpp` / `NetworkInterfaces.cpp` (they depend on nothing but `emeraude_export.hpp`),
  doing a real round-trip, a real DNS-SD exchange, a real enumeration. Reuse that technique
  rather than trusting the build.

## Important Files

- `Manager.cpp/.hpp`, `DownloadItem.hpp`, `CachedDownloadItem.hpp/.cpp`, `Types.hpp` - Download manager (semi-stub, see § Download manager)
- `UDPClient.hpp/.cpp` - UDP client, IPv4 multicast and SSDP discovery
- `NetworkInterfaces.hpp/.cpp` - Local IPv4/IPv6 address enumeration with MAC (feeds the multicast API and scripting bridges)
- `TCPClient.hpp/.cpp` - TCP client (Asio-based, blocking-with-timeout API)
- `TCPServer.hpp/.cpp` - TCP server (Asio-based, accept returns owned TCPClient)
- `SerialPort.hpp` + `.{linux,mac,windows}.cpp` - Serial port abstraction
- `WiFiScanner.hpp` + `.{linux,mac,windows}.cpp` - WiFi scanning
- Download cache: `cacheDirectory("downloads")` + `cacheDirectory("downloads_db.json")` (manager); `cacheDirectory()/data/<ClassId>/` (Resources) — disjoint

## Detailed Documentation

Related systems:
- @docs/todo/net-manager-download-chain-broken.md - The open decision on the download manager
- @src/Resources/AGENTS.md - Fail-safe loading system, `ExternalData` source type
- @docs/resource-management.md - Resources architecture (carries the same warning on downloads)
- emeraude-base `src/Network/` (`HTTPSClient`, `TLSConnection`, `TrustStore`, `URI`) - the production-grade HTTPS stack the engine does not use yet
