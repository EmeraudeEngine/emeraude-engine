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

## What remains

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
