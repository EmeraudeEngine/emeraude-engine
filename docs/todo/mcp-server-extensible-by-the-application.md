---
id: mcp-server-extensible-by-the-application
title: The MCP server must be extensible by the application built on the engine, not only by console commands
status: open
priority: unranked
scope: Console/MCP
opened: 2026-10-13
tags: [console, mcp, ai-runtime, api]
---

# The MCP server must be extensible by the application built on the engine, not only by console commands

## Why

The engine's MCP server is delivered (2026-09-27, `docs/subsystems/console/08-7b-the-mcp-server.md`) and the owner ran a
first real Claude Code session against it (2026-10-13). The owner's goal now: any program built on emeraude-engine —
projet-alpha or another downstream application — must be able to extend the server for its own domain, without patching
the engine.

What an application can do today: register its own `Console::ControllableTrait` (projet-alpha's `StageConsole`,
`src/StageConsole.hpp`, and `Act`), whose typed commands become tools automatically, with `tools/list_changed` when they
appear or vanish. What it cannot do:

- the server's identity is the engine's: `serverInfo.name` is the constant `MCP::ServerName` = `"emeraude-engine"`
  (`src/Console/MCP/Protocol.hpp:68`) and the version is the engine's (`src/Console/MCP/Server.cpp:423-424`);
- the `instructions` sent to the model are a fixed engine text (`MCP::instructions()`, `src/Console/MCP/Protocol.cpp`,
  sent at `Server.cpp:363` and `:431`): an application cannot add the guidance its own tools need (its domain, its
  units, which tool to call first);
- nothing but a console command can become an MCP capability: no application-provided resources or prompts (the
  set-aside features of 08-7b would be extension points, not engine features).

## What remains

1. Design with the owner (options, trade-offs, recommendation) the application-side contract. Candidates: an
   application identity and an instructions SUPPLEMENT given at start-up (engine text + application text), a
   registration object for resources / prompts that the application owns (a move-only token whose destructor
   unregisters, Ave Robustus II), and the same main-thread and list-changed guarantees as the console tools.
2. Implement it in the engine; document the application recipe in `08-7b-the-mcp-server.md` and
   `docs/ai-runtime-control.md` § The MCP server.
3. Prove it from a downstream application: projet-alpha declares its identity / instructions supplement (and one
   resource or prompt if the design keeps them), checked with `tools/mcp-conformance.py` and a real client session.

## ⚠️ Traps

- The engine repository is public: a downstream application's domain text never goes into the engine; it is the
  application's own string.
- Tool names are capped at 49 characters and must stay unique across engine and application commands (a colliding name
  is left out and traced once): an application prefix may be needed — decide it in step 1.

## References

- `docs/subsystems/console/08-7b-the-mcp-server.md` (the server, owner decisions, set-aside features, traps).
- `docs/ai-runtime-control.md` § The MCP server (operator view).
- MCP specification 2026-07-28: https://modelcontextprotocol.io/specification/2026-07-28/
