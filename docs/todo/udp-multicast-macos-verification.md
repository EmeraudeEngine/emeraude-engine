---
id: udp-multicast-macos-verification
title: UDP multicast / mDNS — macOS never executed
status: open
priority: medium
scope: Net
opened: 2026-08-26
tags: [network, macos, mdns]
---

# UDP multicast / mDNS — macOS never executed

## Why

IPv4 multicast was added to `EmEn::Net::UDPClient` plus the new `EmEn::Net::NetworkInterfaces`
module (engine, `develop`, commit `bdb1ebca`, pushed), for mDNS/DNS-SD discovery.

`NetworkInterfaces` was then extended to IPv4+IPv6 with the hardware address (2026-08-27):
the `AF_LINK` (MAC) and IPv6 paths of `NetworkInterfaces.cpp` are in the same standing as the
multicast surface — **verified on Linux, compile-only on macOS**.

**Verified on Linux 6.x (2026-08-26)**: full multicast round trip, bind of **5353 alongside
avahi-daemon**, join of `224.0.0.251` on 2 NICs, a real DNS-SD query, answers from 4 LAN devices,
interface index resolved on every datagram. Cascade build exit 0, zero warning.

**macOS: compiles (BSD path `IP_RECVDSTADDR` + `IP_RECVIF`), ZERO execution.**

## What remains — and what else must be run in the same macOS/Windows session (2026-08-27)

The whole network stack was hardened on 2026-08-27 and **none of that work has executed on
Windows or macOS**. The companion checklist, which this item is part of, is
[`emeraude-base/docs/todo/tls-stack-windows-macos-validation.md`](../../dependencies/emeraude-base/docs/todo/tls-stack-windows-macos-validation.md)
— run both in one sitting, they share the same machines.

Specific to this item, beyond the multicast round trip:

- [ ] `NetworkInterfaces::enumerate()` now returns **IPv4 + IPv6 + the MAC**. The Windows leg
  (`GetAdaptersAddresses(AF_UNSPEC)`, netmask derived from `OnLinkPrefixLength`, index read per
  family) and the macOS leg (`AF_LINK` for the hardware address) have never run. Check a
  multi-homed host: one entry per address, MAC repeated across an interface's addresses, empty MAC
  on loopback (never the `00:00:…` placeholder — that belongs to the JS projection).
- [ ] `UDPClient::receive(timeoutMs = 0)` is **non-blocking** now, and `close()` is two-phase with a
  `shared_mutex`. Verify a `close()` from another thread wakes a parked `receive()` on Winsock
  (`shutdown(SD_BOTH)`) as it does on POSIX.
- [ ] `ssdpDiscover()` sets the multicast TTL with the platform's byte type — **that is the fix
  that matters here**: as an `int` the option was refused on BSD/macOS, the TTL stayed 1, and every
  device more than one hop away silently went missing. Confirm devices at 2 hops now answer.

## Original scope

- [ ] Run it on macOS. Two distinct risks:
  1. sharing port 5353 with `mDNSResponder`;
  2. **since macOS 15 (Sequoia), the "Local Network" authorisation**
     (`NSLocalNetworkUsageDescription` + a Privacy entry) — field reports describe multicast
     working from the Terminal but not from a double-clicked bundle.
- [ ] ⚠️ **Any macOS spike must run a SIGNED AND PACKAGED binary** — a console test proves
  nothing about the bundle case.

## ⚠️ Traps (paid on Linux)

- ⚠️⚠️⚠️ **A socket option that feeds `bind()` or `recvmsg()` is armed at `open()`, NEVER
  lazily**: `IP_PKTINFO` armed late yields a CORRECT destination but an `ifindex` of **0** — a
  plausible structure with a false index.

## Owner decisions (taken before writing the code)

Membership list with state rather than mapping `EADDRINUSE`; `SO_REUSEPORT` unconditional;
interface enumeration IN this batch; `IP_PKTINFO` NOW as a `receive()` overload; macOS "we'll
see".
