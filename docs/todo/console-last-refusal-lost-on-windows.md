---
id: console-last-refusal-lost-on-windows
title: The remote console's last refusal line can be lost on Windows (reset instead of a graceful close)
status: open
priority: unranked
scope: Console/RemoteListener
opened: 2026-10-01
tags: [console, network, windows]
---

# The remote console's last refusal line can be lost on Windows (reset instead of a graceful close)

## Why

The Windows peer (2026-10-01, engine `1ee4a6c6`, NVIDIA and AMD) ran `console-conformance` six times. Three runs
failed ONE check, never the same one twice in a row:

- "a flood gets a last refusal then a disconnection (after 0 answers): refusal=False disconnected=True";
- "an over-long line answers a last error line: [WinError 10054] connection forcibly closed by the remote host".

Linux and macOS pass both checks every time.

`RemoteListener::disconnect()` (`src/Console/RemoteListener.cpp`) writes the refusal line, then calls
`shutdown(shutdown_both)` and `close()` on a socket whose receive buffer still holds what the client sent: the
2000-line flood, or the rest of the over-long line. Closing a TCP socket with unread data sends a RST, not a FIN
(RFC 1122 § 4.2.2.13). On Windows, a RST that reaches the client before it reads makes `recv()` fail with 10054 and
discards the refusal already queued on its side. Both failing checks expect that last line, so they fail.

## What remains

- [ ] Reproduce on Windows with the conformance tool and confirm the RST, for example with a capture showing the
  refusal segment followed by a RST.
- [ ] Choose the graceful close (an owner decision), for example:
  - `shutdown(shutdown_send)` after the last write, then drain the receive side for a short bounded time (or until
    the client closes) before `close()`;
  - or `SO_LINGER` with a short timeout.

  The disconnect must stay bounded: a hostile client must not keep the socket open.
- [ ] Re-run `console-conformance` six times on Windows: 0 failures.

## ⚠️ Traps

- `disconnect()` runs under `m_writeMutex`, so a drain loop there must not block the other clients' writes.
- `stop()` has its own close loop with the same pattern; at shutdown losing the line is acceptable, but the order
  explained by its comment (close before joining) must stay.

## References

- `src/Console/RemoteListener.cpp`: `disconnect()` and the over-long-line path (`MaxLineLength`).
- The checks: `tools/console-conformance.py`, the flood and the over-long-line cases.
