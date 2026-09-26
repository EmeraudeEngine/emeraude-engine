---
id: native-mcp-server
title: Native MCP server in the engine, projected from the console command registry
status: blocked
priority: unranked
scope: Console
opened: 2026-09-27
blocked-by: [console-command-contract]
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

## Prerequisites delivered (2026-09-27)

- The typed command contract (`console-command-contract`): all 137 commands declare their parameters
  (types, arity, defaults, descriptions) and hints; `describeCommands()` exports them as JSON — the
  source the `tools/list` schemas are built from.
- The framed TCP 7777 (one JSON response per request, in order, per-client queue share, serialized
  writes — `src/Console/RemoteProtocol.hpp`) and the conformance bench `tools/console-conformance.py`.
  The MCP server does not go through TCP 7777 (it projects the registry in-process), but it must
  reuse the same guarantees: one answer per request, never an unsolicited write on a request stream.

## What remains

1. **Transport: Streamable HTTP on loopback** (stdio is out — the engine is a GUI process started
   on its own, and its stdout carries the logs). Built on what is vendored: standalone asio
   (`Net::TCPServer`) and jsoncpp. A minimal HTTP/1.1 server (POST, JSON or SSE response) is the
   only new piece.
2. **Protocol revision**: implement **2026-07-28** (stateless: no `initialize`, no session,
   `server/discover` mandatory, version + client capabilities in every request's `_meta`,
   `Mcp-Method`/`Mcp-Name` headers validated against the body, `resultType` on every result,
   `ttlMs`/`cacheScope` on list results). Decide whether to also answer the **2025-11-25**
   handshake era (`initialize`, GET → 405, no session minted) — required as long as the clients in
   use have not moved to 2026-07-28; **check what Claude Code speaks before writing the server.**
3. **Tools** from the registry: name = the command path (`Core.RendererService.screenshot` — dots
   are valid), `inputSchema` from the declared signature, annotations from the behaviour hints,
   deterministic order. Results: text + `structuredContent` + image content (`screenshot`,
   `temporalCapture`).
4. **Dynamic tree**: `SceneManagerService.PostProcess` appears and disappears with the active
   scene → `notifications/tools/list_changed` on a `subscriptions/listen` stream.
5. **Long operations** (scene load, `temporalCapture(N)`): `notifications/progress` on the
   request's SSE stream; closing that stream = cancellation.
6. **Resources** (read-only state, no tool call needed): settings JSON, active scene graph,
   post-process status, recent log lines — candidates to confirm with the owner.
7. **Settings keys** beside the console ones (`Core/Console/…` or a `Core/MCP/…` section — owner
   to decide), and the Shift+F10-style live activation question.
8. **Docs**: `docs/ai-runtime-control.md` (connection via MCP, `claude mcp add --transport http …`),
   `src/Console/AGENTS.md`, projet-alpha `AGENTS.md` § 3b, and the `.claude/rules/` mirror.

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
