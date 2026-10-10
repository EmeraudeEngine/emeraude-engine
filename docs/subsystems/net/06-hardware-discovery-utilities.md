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
a downstream application's JS path — same empty payload, same empty sender address, same port 0 — so a polling
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
**Verified on macOS 26.5 / arm64 and Windows** (2026-08-28, `tools/net-check` + a downstream
application's JS path): `bind(5353)` beside the system `mDNSResponder` (`SO_REUSEPORT` does its
job), joins on every real NIC, real DNS-SD answers, and the BSD `IP_RECVDSTADDR` + `IP_RECVIF`
path delivered destination and a non-zero interface index on 14/14 datagrams.
⚠️ **Windows: an APIPA interface makes `addMembership` fail** — a consumer joining all
interfaces inside one `try` block loses every interface after the first dead one, and Windows
always has dead ones. Join per interface, tolerate failures.

⚠️⚠️ **macOS *Local Network* privacy gate (macOS 15+), measured on a Developer-ID-signed,
hardened-runtime bundle**: while denied, `bind()`, TTL, loopback and **joins all report
success** and **inbound multicast works**, but **every outbound send is refused** (`sendto()`
→ -1), multicast *and* LAN unicast alike. Nothing in the socket API says "permission" — learn
that failure shape. Once the owner granted *Local Network*, the same bundle completed the full
round trip. `NSLocalNetworkUsageDescription` in the **main** plist is what makes the app
eligible; adding it to the helper plists or running from `/Applications` changes nothing, and
there is no multicast entitlement to chase.
- ⚠️ **A terminal run proves nothing**: a terminal-launched process inherits Terminal's grant
  and works while the app itself is denied. Test a signed bundle launched through LaunchServices.
- **Not established**: whether macOS prompts spontaneously on first use (no prompt was ever
  observed; the grant was applied by hand). So an app must behave sanely while denied, and may
  have to send the user to *System Settings → Privacy & Security → Local Network* itself.

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
**Verified on macOS 26.5 / arm64** (2026-08-28): 18 addresses on a multi-homed host, MAC
identical across an interface's addresses and empty on loopback, non-zero index everywhere,
`scopeId` set on link-local IPv6. **Windows** (2026-08-28, `net_check.exe` 47/0/2):
`enumerateMulticastCapable()` returns the NIC **and** `Loopback Pseudo-Interface 1` — Windows
sets the multicast flag on its own loopback, so the Linux exemption below is not needed there.

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
- **Asio is used only during `connect()`** — for DNS resolution (`asio::ip::tcp::resolver`) and the timed connection race of base `Network::connectFirstReachable()` (Happy Eyeballs, RFC 8305: the families interleaved, the next address after 250 ms or at once on a failure — a sequential connect paid ~2 s per refused `::1` on Windows; 2026-10-07). Once the connection is established, the kernel handle is detached from Asio via `socket->release()`, switched to blocking mode (Asio leaves it non-blocking for its reactor / IOCP), SIGPIPE is suppressed where needed (`SO_NOSIGPIPE` on macOS, `MSG_NOSIGNAL` per `send()` call on Linux, n/a on Windows), and stored as a raw `native_handle_type` (`std::intptr_t`).
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
