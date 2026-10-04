## 7b. The MCP Server (`MCP/`, 2026-09-27)

Every typed command is also a Model Context Protocol tool, served over Streamable HTTP by
`MCP::Server` (owned by `Controller`, started in `onInitialize()` when `Core/MCP/Enabled`). Operator
view (settings, connecting Claude Code, security): [`docs/ai-runtime-control.md`](../../ai-runtime-control.md)
§ The MCP server. Plan and owner decisions: [`docs/todo/native-mcp-server.md`](../../todo/native-mcp-server.md).

| File | Role |
|---|---|
| `MCP/Protocol.hpp/.cpp` | Everything that is not a socket: tool names, `collectTools()`, `toolDefinition()` (schema from the signature), `jsonToArguments()`, `callResult()` (image reduction), JSON-RPC envelopes, the safe JSON accessors |
| `MCP/Server.hpp/.cpp` | The MCP protocol on emeraude-base `Network::HTTPServer` (HTTP/1.1, limits, Host / Origin / bearer checks, SSE streams — extracted there 2026-10-04 for resource sharing, base `docs/subsystems/source-tree/24-network-http-server.md`): one endpoint `/mcp`, dual-era dispatch, the main-thread queue, the stream registry by connection id |

**Rules that keep it solid — each one answers a defect found while building it:**

- **Threads**: every socket operation on the server's network thread; the console tree only on the main
  thread. `tools/list` and `tools/call` are queued (`enqueue()`, ≤ 64) and run by
  `processPendingRequests()` from `Controller::poll()`; the answer is `asio::post`ed back. `initialize`,
  `server/discover`, `ping`, `subscriptions/listen` are answered on the network thread (no tree needed).
  A tool result with an image is BUILT on the network thread (the PNG is read and reduced there, never
  between two frames).
- **Client JSON**: only through `member()`/`stringMember()`/`boolMember()` — jsoncpp aborts the process
  on a type-mismatched access (`docs/caution-points.md` § Console / MCP).
- **Stale pointers**: a command may change the tree; the batch rebuilds its tool list when
  `Controller::consoleTreeRevision()` moved.
- **Names**: `toolName()` — `Core.` and `Service` suffixes dropped, `.` → `_`; ≤ 49 characters of
  `A-Za-z0-9_-` (Claude Code's `mcp__emeraude__` prefix + 64-character limit). An invalid or colliding
  name is left out and traced once. Aliases are merged: `Command::primaryName()` is the first name of the
  list the command was bound with; `Command::description()` is the description without the usage line.
- **Named → positional arguments**: `jsonToArguments()` leaves an `ArgumentType::Undefined` HOLE for an
  omitted argument followed by a supplied one; `selectArgument()` and the `std::optional` extractor read
  a hole as "not supplied". The command line never produces one.
- **Images**: `Output::image(path, mime, text)` / `CommandResult::image(…)` (kind `Image`) references a
  FILE; a text channel prints the message (TCP 7777 adds `path`), the MCP channel reduces it to
  1568 px (`Processor::downsample`, area filter) and inlines it. `Renderer.screenshot()` uses it.
- **List changes**: `Controller::markConsoleTreeChanged()` (called by `ControllableTrait` registration,
  binding and `Controller::add()/remove()`) increments `consoleTreeRevision()`; the server announces
  `notifications/tools/list_changed` on modern (`subscriptions/listen`, tagged with the subscription
  id) and handshake-era (GET) streams.
- **Shutdown**: the destructor posts, on the network thread, a graceful closure of each modern stream
  (its final response) and the closing of every socket, waits at most 3 s, then stops the loop — the
  same "close before join" order as `RemoteListener`.
- **Verification**: `tools/mcp-conformance.py` (both eras, headers, security, malformed JSON, every
  tool's validation, screenshot, keep-alive/pipelining/concurrency, streams; `--trigger-list-change`).
  The official TypeScript SDK clients v1 (1.30.1) and v2 (2.1.0) were run against it on 2026-09-27.
