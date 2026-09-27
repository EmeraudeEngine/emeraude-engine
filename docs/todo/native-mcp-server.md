---
id: native-mcp-server
title: Native MCP server in the engine, projected from the console command registry
status: in-progress
priority: unranked
scope: Console
opened: 2026-09-27
tags: [console, mcp, network, ai-runtime]
---

# Native MCP server in the engine, projected from the console command registry

## Why

The remote console is "the AI's hands on the running engine" (`docs/ai-runtime-control.md`), but an
AI reaches it through ad-hoc Python over raw TCP. A Model Context Protocol server makes every
console command a typed tool that any MCP client (Claude Code first) discovers and calls directly,
with captures returned **inline as images** instead of a path to go and read.

## Owner decisions (2026-09-27)

- **Approach**: the command contract first (`console-command-contract`), then a native server
  that is a pure **projection** of the registry — a command added in `onRegisterToConsole()`
  becomes an MCP tool with no second declaration to maintain. An external stdio bridge was
  rejected: it would inherit the untyped, unframed protocol.
- **Granularity**: one MCP tool per typed command (plus one generic tool for the commands not yet
  migrated). Clients load MCP tools on demand, so the count does not cost context.
- **Coexistence**: TCP 7777 stays, same registry, same gate (`remote-console-response-framing`).
- **Security**: loopback only, like the console, closed by default; a Bearer token becomes
  mandatory the moment a non-loopback bind address is allowed.
- **Location**: in the engine (LGPLv3), as a capability of the free runtime — functionality, not
  convenience, so it does not belong to projet-alpha.

## Owner decisions for the server (2026-09-27, second round)

- **Both protocol eras**: 2026-07-28 (stateless, `server/discover`, `subscriptions/listen`) AND the
  handshake era (2025-11-25 / 2025-06-18: `initialize`, optional GET notification stream). Verified in
  the Claude Code docs (https://code.claude.com/docs/en/mcp): its v1 client runtime only speaks the
  handshake era, and its v2 runtime asks an HTTP server for 2026-07-28 and falls back otherwise.
- **Short tool names**: drop `Core.` and each `Service` suffix, dots become `_`
  (`Core.SceneManagerService.PostProcess.select` → `SceneManager_PostProcess_select`). Claude Code
  replaces any character outside `A-Za-z0-9_-` with `_`, and the callable name
  `mcp__<server>__<tool>` must fit 64 characters: a tool name above 49 characters (server name
  `emeraude`) or colliding with another is refused at startup, traced. One tool per command: the
  aliases (`exit,quit,shutdown`) are merged into the first name.
- **Settings `Core/MCP/*`**, independent of TCP 7777: `Enabled` (false), `Address` (127.0.0.1),
  `Port` (7778), `BearerToken` (empty; MANDATORY for a non-loopback address — the server refuses to
  start without it). `Origin` validation always on.
- **Screenshot over MCP = a reduced image + the path**: a PNG scaled to 1568 px on its long edge
  (inline image, far below Claude Code's `MAX_MCP_OUTPUT_TOKENS` = 25 000) plus the full-resolution
  file path for pixel measurement. TCP 7777 keeps answering the path only; the reduction is done
  only when the channel needs the image.

## Prerequisites delivered (2026-09-27)

- The typed command contract (`console-command-contract`): all 137 commands declare their parameters
  (types, arity, defaults, descriptions) and hints; `describeCommands()` exports them as JSON — the
  source the `tools/list` schemas are built from.
- The framed TCP 7777 (one JSON response per request, in order, per-client queue share, serialized
  writes — `src/Console/RemoteProtocol.hpp`) and the conformance bench `tools/console-conformance.py`.
  The MCP server does not go through TCP 7777 (it projects the registry in-process), but it must
  reuse the same guarantees: one answer per request, never an unsolicited write on a request stream.

## Delivered (2026-09-27)

`src/Console/MCP/` (Protocol + Server), wired in `Controller`, settings `Core/MCP/*`. Both eras,
every typed command as a tool (127 on `coordinates-debug`), inline reduced screenshot,
`list_changed` on both stream kinds, Origin/Host/token checks, bounded sizes and queues, safe JSON
reads, stale-pointer rebuild. Verified: build with 0 warnings, `tools/mcp-conformance.py` 885 checks
(shown to fail against a lying server), the official TypeScript SDK clients v1 1.30.1 (handshake era)
and v2 2.1.0 (`versionNegotiation: auto` → 2026-07-28) — list, call, image, list_changed —, 0 VUID.
Documented: `docs/ai-runtime-control.md` § The MCP server, `src/Console/AGENTS.md` § 7b,
`docs/caution-points.md` § Console / MCP.

## What remains

1. **Cross-platform validation** by the macOS (Clang) and Windows (MSVC) peers, conformance bench
   included — until then this item stays open.
2. **The owner's first real session** with Claude Code (`claude mcp add --transport http emeraude
   http://127.0.0.1:7778/mcp`): check the image reaches the model and the tool search copes with 127
   tools.
3. **Progress notifications** for long calls (`temporalCapture(N)`, a heavy `openFiles`) — not
   implemented: a `tools/call` answers `application/json` only. Needs an SSE response and a progress
   hook in the command contract. Owner to decide whether it is worth it.
4. **Resources** (read-only state without a tool call: settings, scene graph, log tail) — not
   implemented, candidates to confirm with the owner.
5. The legacy handshake era accepts no `Mcp-Session-Id` (none is minted) — conforming, but a legacy
   client that insists on a session is untested.

## ⚠️ Traps

- **`Origin` MUST be validated** on every request (HTTP 403 otherwise) — DNS rebinding. CEF runs
  inside the same process and renders web pages: no page may be able to POST to the endpoint and
  reach `Core.quit()`, the settings or scene loading.
- Commands execute on the **main thread** (existing queue in `RemoteListener`); the HTTP side
  never runs a binding itself and never blocks the network thread on a 5 s capture.
- `-fno-exceptions` everywhere — rules out any SDK that throws; the C++ SDKs found
  (gopher-mcp, fastmcpp) are to be read as references, not vendored, unless one qualifies
  (licence compatible with LGPLv3, no exceptions, no new heavy dependency).
- A broken SSE stream loses the in-flight request (no resumability in 2026-07-28): a long command
  must stay safe to re-issue.

## References

- MCP 2026-07-28 changelog: https://modelcontextprotocol.io/specification/2026-07-28/changelog
- Streamable HTTP transport: https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http
- Tools: https://modelcontextprotocol.io/specification/2026-07-28/server/tools
- C++ implementations to read: https://github.com/GopherSecurity/gopher-mcp ,
  https://github.com/0xeb/fastmcpp
