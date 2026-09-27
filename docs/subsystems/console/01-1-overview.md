## 1. Overview

The Console system provides **runtime command execution** for the engine. It is the **primary channel for AI-driven control** of a running application. An AI agent can create scenes, manipulate cameras, control audio, modify settings, and visually verify results — all through TCP commands.

### Remote Console (TCP)

A TCP server listens on a configurable port (default **7777**, setting: `Core/Console/RemoteListenerPort`).

> [!WARNING]
> **Closed by default.** `Controller::onInitialize()` creates the `RemoteListener` only when
> **`Core/Console/EnableRemoteListener`** is `true` (default **`false`**, 2026-08-27). It then binds
> to **`Core/Console/RemoteListenerAddress`** (default **`127.0.0.1`** — loopback only; `0.0.0.0`,
> a NIC address or `::` to accept other machines; an unparsable value falls back to loopback, never
> to any-address). All three keys are written to `settings.json` on first run even when the console
> stays off, so an operator can find them. **Live activation: Shift+F10** (engine-level shortcut,
> replaced the old "suspend 3 s") — closed: a native `TextInput` dialog pre-filled with the configured
> port accepts `7777`, `0.0.0.0:7777` or `[::1]:7777`, then `Controller::startRemoteListener()` runs
> for **this session only** (settings untouched); open: a Yes/No dialog stops it. From the console
> itself: `Core.remoteConsoleStatus()` and `Core.restartRemoteConsole(port | address:port)` — the
> move is applied at the start of the next `poll()` so the response reaches the client first; that
> connection then closes and you reconnect on the new endpoint. When disabled the log says
> `Remote console disabled (Core/Console/EnableRemoteListener = false)` — an AI seeing
> `Connection refused` must **enable the key and relaunch**, not retry. Why: the channel has **no
> authentication** and reaches `Core.quit()`, settings, scene loading and screenshots; applications
> built on the engine ship to end users.

Once enabled, any AI agent or external tool on an allowed host can connect and:

1. **Send commands** — one per line, newline-terminated (`\n`)
2. **Receive exactly one response per command** — ONE JSON object on ONE line, in request order,
   even for a failure or a command with no output (no Tracer noise)

**Wire format (owner decision 2026-09-27, `RemoteProtocol.hpp`):**
`{"ok":bool,"outputs":[{"severity":"Info","kind":"text|json|binary","message":"…"}]}` — a `binary`
output adds `mimeType` and a Base64 `data`; the welcome banner is a response with a top-level
`"protocol":1`. Full description: [`docs/ai-runtime-control.md`](../../ai-runtime-control.md) § Wire format.

- **Only `RemoteProtocol` writes to a client** (`serializeResponse`, `serializeError`,
  `serializeWelcome`); `RemoteListener::respond()` appends the newline, nothing else. Never write raw
  text to a socket, never send anything unsolicited (the former `broadcast()` was deleted: a client
  matches each line to its request in order).
- **One write at a time**: `m_writeMutex` serializes the network thread (transport errors) and the main
  thread (responses) — two interleaved writes corrupt a line. `stop()` deliberately closes the sockets
  WITHOUT that lock (it must unblock a stuck write; taking it under `m_clientsMutex` would also invert
  the lock order of `respond()`).
- **Per-client share of the queue**: `MaxPendingCommandsPerClient` = 256 / 8 = 32. A client above it
  gets a last error line and is disconnected (`disconnect()`), so it can never make another client's
  command overflow, and no refusal ever overtakes an answer still queued. A line over 8192 bytes is
  handled the same way (the socket is now really closed; it used to be only forgotten).
- **Clients**: `tools/emeraude_console.py` (the one implementation: `Console.call()` → parsed response,
  `.run()` → text), `tools/remote-console.py` (CLI: text, `--json` for raw lines, exit 1 on failure).
  `nc` prints the raw JSON lines. ⚠️ An engine built before 2026-09-27 answers raw text: the current
  client refuses it with a protocol error.
- **Conformance**: `tools/console-conformance.py` against a live instance checks the wire format and
  the typed contract of every command (via `describeCommands()`), non-destructively.

**Lifecycle:** The listener starts in `Controller::onInitialize()` and stops in `Controller::onTerminate()`. It runs on a dedicated network thread. Commands are queued and executed on the **main thread**.
