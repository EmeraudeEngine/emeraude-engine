---
id: udp-multicast-macos-verification
title: UDP multicast / mDNS — macOS run; packaged bundle + a Linux re-run left
status: open
priority: low
scope: Net
opened: 2026-08-26
tags: [network, macos, mdns]
---

# UDP multicast / mDNS — macOS run; packaged bundle + a Linux re-run left

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

⚠️ **The Linux leg therefore changed from `unsigned char` to `int` (2026-08-28) and has NOT been
re-run on Linux** — it is the type `ip(7)` documents and the form nearly all Linux multicast code
uses, and both widths were measured as accepted, so the risk is very low; it is still a change to
a platform that was green. All three branches were compile-checked. Re-run the Linux mDNS fixture
at the next occasion.

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

- [ ] ⚠️ **A SIGNED AND PACKAGED binary on macOS.** Everything above ran from the Terminal, which
  the original scope already flagged as proving nothing: since macOS 15 (Sequoia) multicast needs
  the **"Local Network" authorisation** (`NSLocalNetworkUsageDescription` + a Privacy entry), and
  field reports describe multicast working from a terminal and failing from a double-clicked
  bundle. No permission prompt appeared during this run — Terminal is already authorised on this
  machine, which is precisely why it proves nothing.
- [ ] The app's own JS path on macOS (`--mode=test` → the dev-check mDNS card). Only the engine
  layer was exercised.
- [ ] ⚠️ **Linux, a RE-RUN**: the multicast option width changed there on 2026-08-28
  (`unsigned char` → `int`, see the TTL section above). It is the type `ip(7)` documents and the
  risk is very low, but Linux was green and has not run since.
- [ ] The Windows leg of everything above — still nothing at all (see the companion checklist).

## ⚠️ Traps (paid on Linux, re-checked on macOS)

- ⚠️⚠️ **A socket option that feeds `bind()` or `recvmsg()` is armed at `open()`, NEVER
  lazily**: `IP_PKTINFO` armed late yields a CORRECT destination but an `ifindex` of **0** — a
  plausible structure with a false index. Re-verified on the BSD path (`IP_RECVDSTADDR` +
  `IP_RECVIF`): 14/14 datagrams carried a non-zero index.

## Owner decisions (taken before writing the code)

Membership list with state rather than mapping `EADDRINUSE`; `SO_REUSEPORT` unconditional;
interface enumeration IN this batch; `IP_PKTINFO` NOW as a `receive()` overload; macOS "we'll
see" — that last one is now settled.
