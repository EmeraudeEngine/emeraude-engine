## Critical Points

- **Do not confuse** with AVConsole (`src/Scenes/AVConsole/`) which is the Audio/Video virtual device system
- **Closed by default** — `Core/Console/EnableRemoteListener` (default `false`) gates the whole listener; **bind address** `Core/Console/RemoteListenerAddress` (default `127.0.0.1`); **port** `Core/Console/RemoteListenerPort` (default 7777)
- **Live control** — `Controller::startRemoteListener(address, port)` / `stopRemoteListener()` / `isRemoteListenerRunning()` / `remoteListenerEndpoint()`; `requestRemoteListenerRestart()` defers to the next `poll()` (never replace the listener while draining its own queue: the response socket dies). `parseEndpoint()` is the single parser for the dialog and the command. Shift+F10 in `Core::toggleRemoteConsoleFromKeyboard()`; the loop is paused around the modal native dialogs like `displayCoreMessages()`
- **No authentication** — which is exactly why the two defaults above are off and loopback. Anyone who can reach the socket owns the application
- **Bounded everywhere (2026-08-27 hardening)** — `MaxPendingCommands = 256` shared, `MaxPendingCommandsPerClient = 32` each (2026-09-27: a client above its share gets a last error line and is disconnected — it used to be answered `ERROR: command queue full` and kept, which let its refusal overtake answers still queued and let one client starve the others), `MaxLineLength = 8192` (the read buffer is capped: an unbounded `streambuf` let a peer that never sends a newline grow the process until the OOM killer fired, on an unauthenticated port; a longer line gets a last error line and is disconnected), `MaxClients = 8` (further connections get an error line). A disconnected client's last line ends with a FIN, never a RST (2026-10-06): `disconnect()` removes it from the set under the write lock (`respond()` checks the membership there, `doRead()` stops) and hands the socket to emeraude-base's `Network::GracefulCloser` (shutdown(send), drain ≤ 1 s / 64 KiB, ≤ `MaxClients` lingering) — closing over the flood still unread was a RST that lost the line on Windows (item `console-last-refusal-lost-on-windows`). Every one of these answers is a `RemoteProtocol` JSON line, `SendTimeoutMilliseconds = 2000` (`SO_SNDTIMEO` on every accepted socket)
- **Shutdown order is load-bearing** — the acceptor and every client socket are closed **before** `io_context::stop()` and the thread join. `stop()` does not interrupt a handler already running, and a write to a peer that stopped reading only ends when its socket dies: closing after the join made the whole process hang on exit, forever, because of one frozen client. Verified: exit in ~1.6 s with a client that never reads
- **No blocking write can hang a thread** — the welcome banner runs on the io thread and `respond()` runs on the **main** thread; the send timeout bounds both. (The banner also used to send a stray NUL: `asio::buffer` on a `char[]` includes the terminator — pass a `std::string_view`.)
- **`accept()` never re-arms blindly** — the error is logged, and a non-transient one (`EMFILE`, out of memory) stops the loop instead of spinning a core at 100 %
- **Clean responses** — TCP responses contain only command output, no Tracer logs
- **JSON routing** — lines starting with `{` bypass the command parser and go to the JSON handler
- **Quoted arguments are literal (2026-09-27)** — inside `"…"` or `'…'` a comma, a parenthesis or a
  dot no longer splits the argument (`openFiles("/tmp/a,b(1)/x.glb")` is ONE path). Before that,
  `Expression` cut on every `,` and `)` regardless of quotes
- **Every command is typed** — the untyped form was deleted (2026-09-27); `describeCommands()` exports
  every signature as JSON
- **A float argument is FINITE and in range, on every OS (triad, 2026-09-30)** — `convertArgument(float)` refuses a
  NaN / an infinity (the single choke point of every typed float); the MCP bridge refuses a JSON number beyond the
  float range (`1e300`: "outside the 32-bit floating point range" — converting it was UB); the macOS `strtof` branch
  of `Expression` refuses an overflow (`1.0e999`, ERANGE) like `from_chars` elsewhere; `Argument::asInteger()` answers
  a warning and 0 for a float outside the int32 range. Measured: 1e300 / 3.5e38 refused, 20000 accepted, `1.0e999`
  refused on the TCP console
- **asio error codes: the out-parameter only (`ASIO_NO_DEPRECATED`, 2026-09-30)** — the synchronous asio calls return
  `void`; every setup step reads its `ec` (a failed `reuse_address`, `SO_SNDTIMEO` or `TCP_NODELAY` is a warning, no
  longer overwritten or dropped); a best-effort teardown close says so in a comment
- **AI Runtime Control** — See [`docs/ai-runtime-control.md`](../../ai-runtime-control.md) for the complete AI operator reference
