---
id: udp-receive-timeout-indistinguishable
title: "UDPClient::receive() — a timeout and a zero-length datagram are the same result"
status: open
priority: medium
scope: src/Net
opened: 2026-08-28
tags: [network, udp, contract]
---

# `UDPClient::receive()` — a timeout and a zero-length datagram are the same result

## Why

A zero-length UDP datagram is legal and is really used (keepalives, liveness probes, NAT
punching). Today a consumer cannot tell one from a timeout, at any layer of the cascade.

Raised as a hypothesis from Windows on 2026-08-28 and **measured on Linux the same day** through
the dev-check UDP card. Three cases, one socket, `udp.Socket`:

| Case | `bytes` | `address` | `port` |
|---|---|---|---|
| Datagram `"hello"` | 5 | `"127.0.0.1"` | 35305 |
| Datagram of length **0** | 0 | `""` | **0** |
| **Timeout** (nobody sends) | 0 | `""` | **0** |

The last two rows are identical, field for field. Only the wall-clock timing differs (0 ms vs the
full timeout), and timing is not a contract.

⚠️ Note the second row: the sender address **is** normally filled — it is *discarded* precisely in
the case that would have discriminated.

## Root cause

Two independent things, both in `UDPClient::receive(void *, size_t, std::string &, uint16_t &,
uint32_t)`:

1. **The address is behind a `bytesRead > 0` guard.** A zero-length datagram has `bytesRead == 0`,
   so `senderAddress` / `senderPort` are never written, and the caller sees the timeout shape.
2. **The return value cannot express the difference.** The timeout path returns `0`
   ("Timeout, `close()` from another thread, or error") and a zero-length datagram also returns
   `0`. The ambiguity is structural, not a missing `if`.

## What remains

Both layers have to move together, which is why this is not a one-line change:

- [ ] Engine: fill the address for any successful `recvfrom` (`bytesRead >= 0`), and surface the
  timeout out-of-band rather than through the byte count. **The codebase already has the pattern**
  — `TCPServer::m_lastAcceptTimedOut` + `accept()` returning `std::nullopt`. Mirror it
  (`lastReceiveTimedOut()`) rather than inventing a third convention.
- [ ] app_system: `UDPModule::receiveData` writes the new flag, `UDPReceiveResult` gains
  `timedOut`, and its doc comment stops saying *"Length 0 on timeout / no data"* as if the two were
  interchangeable.
- [ ] The `dgram` shim: Node delivers a zero-length datagram as a `"message"` event with an empty
  buffer and a valid `rinfo`. Today it cannot, for the same reason.

## ⚠️ Traps

- **Do not "fix" this by treating `bytes === 0` as a timeout in the wrapper.** That is the current
  behaviour and it is what silently drops a legal datagram.
- ⚠️ **`receive(maxLength, timeout)` — that order.** `receive(3000)` sets *maxLength* to 3000 and
  leaves the timeout at its default **1000**, which looks exactly like "the timeout is not
  honoured". This cost a false lead on Windows before the signature was read; the timeout was being
  honoured perfectly.
- The three platforms validated this contract in 2026-08-28 without noticing, because mDNS and SSDP
  always carry a payload. Nothing in the discovery paths is affected — this is a correctness gap,
  not a shipping blocker, hence `medium`.

## References

- `src/Net/UDPClient.cpp` — `receive()`, the `bytesRead > 0` guard and the `return 0` timeout path
- `src/Net/TCPServer.cpp` — `m_lastAcceptTimedOut`, the pattern to mirror
- `app_system/src/WebModules/UDPModule/` — the binding, `@types/UDP.d.ts`, the `dgram` shim
