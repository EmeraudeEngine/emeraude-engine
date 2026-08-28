---
id: udp-multicast-macos-verification
title: "UDP multicast / mDNS — packaged macOS bundle + the JS path on macOS left"
status: open
priority: low
scope: Net
opened: 2026-08-26
tags: [network, macos, mdns]
---

# UDP multicast / mDNS — packaged macOS bundle + the JS path on macOS left

## Why

IPv4 multicast was added to `EmEn::Net::UDPClient` plus the new `EmEn::Net::NetworkInterfaces`
module (engine, `develop`, commit `bdb1ebca`, pushed), for mDNS/DNS-SD discovery.

`NetworkInterfaces` was then extended to IPv4+IPv6 with the hardware address (2026-08-27):
the `AF_LINK` (MAC) and IPv6 paths of `NetworkInterfaces.cpp` were in the same standing as the
multicast surface — verified on Linux, compile-only on macOS.

**Verified on Linux 6.x (2026-08-26)**: full multicast round trip, bind of **5353 alongside
avahi-daemon**, join of `224.0.0.251` on 2 NICs, a real DNS-SD query, answers from 4 LAN devices,
interface index resolved on every datagram. Cascade build exit 0, zero warning.

**Verified on macOS 26.5.2 / arm64 (2026-08-28)** — see below. Priority dropped from medium to
low: what remains is the packaged-bundle permission case, not the code.

## Done on macOS (2026-08-28)

Method: the real `UDPClient.cpp` + `NetworkInterfaces.cpp` compiled out-of-tree into a standalone
harness (the technique `src/Net/AGENTS.md` § Development Commands describes), clang++ 21,
`-Wall -Wextra` clean, then re-run under ASan+UBSan. Host: multi-homed, en0 + en1 + awdl0 + llw0
+ 4 utun. **47 assertions, 0 failure** after the fixes below.

- [x] `NetworkInterfaces::enumerate()` returns IPv4 + IPv6 + the MAC. The macOS `AF_LINK` leg
  works: 18 addresses, MAC repeated identically across an interface's addresses, **empty** on
  loopback (never the `00:00:…` placeholder), index non-zero everywhere, `scopeId` set on the
  link-local IPv6. `enumerateMulticastCapable()` returned lo0 + en0 + en1, IPv4 only.
  **The Windows leg (`GetAdaptersAddresses(AF_UNSPEC)`) is still unrun.**
- [x] `UDPClient::receive(timeoutMs = 0)` is non-blocking: returned 0 in 0 ms; `receive(300)`
  parked ~300 ms.
- [x] `close()` from another thread returns a parked `receive()`. **It did not** — this is the
  bug the sitting found, see below. Fixed and re-verified: now ~50 ms, one poll slice.
  **Not verified on Winsock**, and Winsock is likely to have had the same symptom.
- [x] `ssdpDiscover()` — 54 responses from the LAN, TTL applied.
- [x] mDNS round trip: `bind(5353)` **alongside the system `mDNSResponder`** (`SO_REUSEPORT`
  does its job), join `224.0.0.251` on 3 interfaces, TTL 255, a real DNS-SD PTR query for
  `_services._dns-sd._udp.local` → **14 datagrams from 6 hosts, 16 records decoded** (AirPlay,
  Hue, Sonos, IPP, SMB, SFTP…). Idempotent re-join, tolerant leave of a never-joined group, and
  an interface *name* correctly refused, all confirmed.
- [x] ⚠️⚠️ **The `IP_PKTINFO`-armed-at-`open()` discipline pays off on BSD too**: the
  destination address came back on **14/14** datagrams (`IP_RECVDSTADDR`) and the interface index
  was **non-zero on 14/14** (`IP_RECVIF`, idx=6). This is the trap below, and it is not tripped.

## ⚠️ The TTL premise does NOT hold on macOS — do not hunt this ghost

This item used to say the `ssdpDiscover()` TTL fix "is the fix that matters here": as an `int`
the option was supposedly refused on BSD/macOS, the TTL stayed 1, and every device more than one
hop away silently went missing.

**Measured on macOS 26 / arm64: the kernel accepts the `int` form exactly as readily as the
`unsigned char` one**, and the TTL reads back as 255 either way. So a missing device at 2 hops on
macOS has some other cause — do not go looking there.

That leniency is an implementation detail of the current kernels, not a contract, and it is in no
manual. So the type is now chosen **per platform, from each platform's own documentation**
(`MulticastOptionValue`, top of `UDPClient.cpp`), for `IP_MULTICAST_TTL` and `IP_MULTICAST_LOOP`
alike, in `setMulticastTTL()`, `setMulticastLoopback()` and `ssdpDiscover()`:

| Stack | Type sent | Documented in |
|---|---|---|
| macOS / BSD | `unsigned char` | `ip(4)` |
| Linux | `int` | `ip(7)` |
| Windows | `DWORD` | WinSock `IPPROTO_IP` options |

The Linux leg therefore changed from `unsigned char` to `int` (2026-08-28). ✅ **Replayed on Linux
the same day**: the `int` width is accepted and the TTL reads back as 255 through it — and that
kernel, like macOS 26, accepts both widths, which is why the leniency is treated as an
implementation detail and each platform keeps the type its own manual specifies.

## Bugs found in this sitting (all fixed 2026-08-28)

1. ⚠️⚠️ **`close()` did not wake a parked `receive()`.** `shutdown()` fails with `ENOTCONN` on an
   unconnected datagram socket — that is plain POSIX, and **Linux is the lenient outlier** that
   wakes the reader anyway. Measured: `close()` waited out the full 10 s receive timeout. Because
   app_system binds `UDP.close()` as a **synchronous** WebModule method, that stall lands on the
   renderer's main thread (~100 ms through the `dgram` shim, up to the caller's timeout — 1000 ms
   by default — through `udp.Socket.receive()`). `UDPClient` now signals the wake-up itself
   (`m_closing` + 50 ms poll slices in `waitReadable()`) instead of trusting the kernel.
   Isolated probe on this machine: UDP unconnected → not woken; UDP **connected** → woken in
   203 ms; TCP **listening** → not woken. Hence `TCPClient` (connected) and `TCPServer` (Asio
   `acceptor->cancel()`) were never affected.
2. ⚠️ **A moved-from `UDPClient` segfaulted on destruction**, on every platform — `close()`
   dereferenced the `m_handleMutex` the move had given away, with no null check (`TCPClient::close()`
   has one). Caught by ASan. Guards added; `open()` re-arms them so such an instance is reusable.
3. ⚠️ **`SerialPort.mac.mm` silently ran at 9600 bauds** — adjacent, found while walking the
   companion checklist's serial item. See that checklist and `src/Net/AGENTS.md`.

## What is left

> The consolidated per-machine run-list (all three OSes are on hand) lives in the companion
> checklist: [`emeraude-base/docs/todo/tls-stack-windows-macos-validation.md`](../../dependencies/emeraude-base/docs/todo/tls-stack-windows-macos-validation.md)
> § *Full re-test run-list*. The runtime harness is now in the tree at
> [`tools/net-check/`](../../tools/net-check/README.md) — one compiler command per OS, no engine build.

- [x] ⚠️⚠️ **A SIGNED AND PACKAGED binary on macOS — MEASURED 2026-08-28, AND IT FAILS.** The field
  report is reproduced, and the answer is worse than "it needs a permission": **macOS never asks.**

  **What was built.** A genuine Developer-ID-signed bundle: `codesign --verify --deep --strict` →
  *valid on disk*, *satisfies its Designated Requirement*; `flags=0x10000(runtime)` (hardened
  runtime), `Identifier=co.lychee.lycheeslicer`, `TeamIdentifier=QEHS3E9BP6`, Info.plist **bound**,
  1675 files sealed, `NSLocalNetworkUsageDescription` present, no quarantine attribute. Not
  notarized (no credentials on the machine) — irrelevant here, notarization is Gatekeeper, not TCC.

  **What happens.** Launched through **LaunchServices** (`open`), the bundle:
  - **binds** `0.0.0.0:5353` beside `mDNSResponder`, sets TTL 255 + loopback, and **joins**
    `224.0.0.251` on both real NICs — every one of those reports success;
  - **receives** multicast normally (6 datagrams of ambient LAN mDNS traffic);
  - has **every outbound send refused** — multicast *and* plain LAN unicast alike (`sendto()` → -1);
  - and **shows no authorisation prompt at all**, so there is nothing for a user to accept.

  **The differentiator is the launch path, not the signature.** The *same* signed binary launched
  from a terminal works completely (DNS-SD answered by 7 LAN hosts): a terminal-launched process
  inherits Terminal's own Local Network grant. That is precisely why every earlier run in this file
  proved nothing, and it is now demonstrated rather than suspected.

  **Ruled out by measurement, not by reasoning:**
  - `NSLocalNetworkUsageDescription` in the **main** plist — present throughout, necessary but not
    sufficient;
  - the same key added to **all five helper plists** (`co.lychee.lycheeslicer.helper*`, including
    `.renderer` where the UDP WebModule actually runs) — **no change**. This was the standing
    hypothesis recorded here; it is wrong, or at least not the whole story;
  - running from **`/Applications`** instead of an arbitrary directory — **no change**;
  - the code path itself — raw sockets, the `dgram` shim and the native binding all behave
    identically, and all work from a terminal.

  **Still unknown, and it needs the machine's owner** (an AI cannot read `TCC.db`, SIP-protected,
  nor click the settings pane): whether macOS has **cached a denial** for `co.lychee.lycheeslicer`
  from the very first attempt, or never intends to prompt. *System Settings → Privacy & Security →
  Local Network* answers it in ten seconds — is `LycheeSlicer` listed, and is its switch off?
  If it is listed and off, this is a cached deny and the prompt logic is fine; if it is absent, the
  app is being refused without ever being offered, and the next avenue is Apple's multicast
  entitlement (`com.apple.developer.networking.multicast`, request-only) — noting that **LAN
  unicast fails too**, which points at the broader Local Network gate rather than multicast alone.

  ⚠️ **Treat this as a shipping blocker for printer discovery on macOS**, not a validation detail.

  **One defect fixed on the way, and it is platform-neutral.** app_system's `UDPModule` reported a
  refused send as a **success**: `sendData()`/`sendBuffer()` wrote the engine's raw byte count into
  the task and called `finish()`, so the OS refusal arrived in JavaScript as
  `{error:false, success:true, bytesSent:-1}`. The JS layer then lost it twice — `udp.Socket.send()`
  resolved that `-1` as the byte count, and the `dgram` shim guarded only on `bytesSent < 0`, which
  silently stops working once the native side aborts instead (an aborted task carries no
  `bytesSent`, and `undefined < 0` is false). Without that fix this whole investigation reads as
  "sends succeed but nothing arrives". Now all three paths surface
  `UDP send refused by the system (host '…', port …)`. Success path re-verified unchanged.
- [x] **The app's own JS path on macOS — done 2026-08-28** (`--mode=test`, dev-check mDNS card
  driven over CDP, exactly as Windows was, so the two runs compare). Bind `0.0.0.0:5353` beside the
  system `mDNSResponder`, TTL 255 + loopback, join `224.0.0.251` on **both** real NICs
  (`192.168.1.61`, `192.168.1.26`) with **zero failures** — the APIPA trap that bites Windows has no
  equivalent here — DNS-SD service enumeration answered by **5 LAN hosts** plus our own query back
  through the loopback, idempotent re-join accepted, `dropMembership` on a never-joined group
  tolerated, `close(cb)` + `"close"` event both fired.
  It also produced **the macOS half of the `close()` measurement**, which this item had only for
  Windows: with a `receive()` parked on **3000 ms**, `close()` returns in **20.0 ms**; on 1000 ms,
  **14.7 ms** — bounded by the poll slice and independent of the receive timeout, the same contract
  Windows shows at 62 ms. Through the `dgram` shim's drain loop it is 24.9 ms. So the fix is now
  measured, not inferred, on the platform where the bug was found.
  Same sitting, same JS path: the native `TCP.server`/`TCP.client` bindings and the `net` wrapper's
  full loopback demo (connect, data both ways, `peerClosed`, server stop) also pass, and the
  `TCPServer` `"::"` dual-stack fix was confirmed end to end — an **IPv4** peer connecting to a
  `"::"` listener is accepted. ⚠️ macOS never exhibited that symptom (`net.inet6.ip6.v6only = 0`),
  so this confirms the fix does no harm; **Windows still owns the confirmation that it helps.**
  ⚠️ No *Local Network* permission prompt appeared — expected, and it still proves nothing: this ran
  from an already-authorised Terminal on an ad-hoc-signed bundle. The packaged case below is
  untouched by this run.
- [x] ⚠️ **Linux, a RE-RUN — done 2026-08-28**, `tools/net-check`: **48 pass / 0 fail / 1 warn**,
  identical under `-fsanitize=address,undefined`. The `int` width is accepted and the TTL reads
  back as **255** through it; this kernel accepts **both** widths, like macOS 26. The replay also
  covered the shared-path changes of that same commit, which nothing had measured on Linux:
  `close()` returns a parked `receive()` (300 ms, bounded by the poll slice), the deadline-based
  accounting shows **0 % drift over 1200 ms**, and the moved-from instance is safe and reusable.
  `shutdown_semantics` re-confirms Linux as the lenient outlier (`ENOTCONN` **and** reader woken).
  ⚠️ It also surfaced **one defect of its own**, unrelated to the macOS work and as old as
  `NetworkInterfaces`: `enumerateMulticastCapable()` dropped `lo` on Linux, because the filter
  trusted `IFF_MULTICAST` and Linux never sets it on loopback — while supporting multicast there
  all the same (measured: join + `IP_MULTICAST_IF` on `127.0.0.1` accepted, datagram round trip).
  On a machine with no link that list came back **empty**, silently disabling every discovery loop
  built on it. Fixed the same day, `#if defined(__linux__)`-scoped, with the harness assertion
  aligned (hence 48 and no longer 47). See `src/Net/AGENTS.md § Network Interfaces`.
- [x] ✅ **Windows, the last datum on this subject — answered 2026-08-28** by the Windows run
  (`net_check.exe`, 47/0/2, exit 0). `loopback is kept (single-machine multicast)` → **OK**:
  `enumerateMulticastCapable()` returns two entries there, `Ethernet 192.168.1.58` **and**
  `Loopback Pseudo-Interface 1 127.0.0.1`, and the mDNS join succeeds on the latter
  (`joined 224.0.0.251 on Loopback Pseudo-Interface 1 (127.0.0.1)`) — so `127.0.0.1` really is
  joinable in multicast on Windows, not a dead entry.
  **Windows sets the multicast flag on its own loopback** (`enumerate()` reports
  `up loopback multicast`), so the Linux `IFF_MULTICAST` exemption is **not needed** there, merely
  harmless. Keeping that fix Linux-scoped is therefore right, and for the stated reason: the
  exemption exists because Linux lies about the flag, and Windows does not.
  **The multicast subject is closed on the data side.**
- [x] **Windows — done 2026-08-28**, through app_system's JS path (`--mode=test`, dev-check mDNS
  fixture over CDP): bind `0.0.0.0:5353`, TTL 255 + loopback, join on the real NIC, DNS-SD
  enumeration answered by **6 LAN hosts**, idempotent re-join, tolerant drop, `close(cb)` +
  `"close"` event. The `DWORD` branch of `MulticastOptionValue` is therefore exercised. It also
  **measured the `close()` fix** (62 ms with a `receive()` parked on 3000 ms, and the same 62 ms at
  1000 ms) and the deadline-based timeout accounting (+0.4% at worst) — both of which this item had
  only inferred for Winsock.
  ⚠️ Two Windows-specific lessons, recorded in `app_system/src/WebModules/UDPModule/AGENTS.md`: a
  double-locked mutex in app_system's job registry had to be fixed before anything could run, and
  **an APIPA interface makes `addMembership` fail** — a consumer joining all interfaces inside one
  `try` block loses every interface after the first dead one, and Windows always has dead ones.

## ⚠️ Traps (paid on Linux, re-checked on macOS)

- ⚠️⚠️ **A socket option that feeds `bind()` or `recvmsg()` is armed at `open()`, NEVER
  lazily**: `IP_PKTINFO` armed late yields a CORRECT destination but an `ifindex` of **0** — a
  plausible structure with a false index. Re-verified on the BSD path (`IP_RECVDSTADDR` +
  `IP_RECVIF`): 14/14 datagrams carried a non-zero index.

## Owner decisions (taken before writing the code)

Membership list with state rather than mapping `EADDRINUSE`; `SO_REUSEPORT` unconditional;
interface enumeration IN this batch; `IP_PKTINFO` NOW as a `receive()` overload; macOS "we'll
see" — that last one is now settled.
