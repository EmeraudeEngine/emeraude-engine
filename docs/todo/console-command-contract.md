---
id: console-command-contract
title: Console commands — declared typed signatures and typed results
status: open
priority: unranked
scope: Console
opened: 2026-09-27
tags: [console, mcp, ai-runtime]
---

# Console commands — declared typed signatures and typed results

## Why

A console command today is `bindCommand(name, binding, help)`
(`src/Console/ControllableTrait.hpp:168`). Nothing declares its parameters: the argument types
(`Boolean`/`Integer`/`Float`/`String`, `src/Console/Argument.hpp`) are **guessed while parsing** the
command line, and the signature exists only as prose inside the help string ("Usage: …"). A result
is a list of `Output` = severity + text (`src/Console/Output.hpp`): `screenshot()` answers a file
PATH, a JSON answer is a string that happens to parse.

That is enough for a human or an AI typing lines, not for a machine contract. It is the first step
of the owner-approved MCP plan (2026-09-27, see `native-mcp-server`): an MCP tool IS an
`inputSchema`, optionally an `outputSchema`, and its result can carry an image inline. A server
projected from commands with no schema would only be a generic `console_exec` in disguise.

## What remains

1. **Declared signature at registration.** Extend `bindCommand` with a signature descriptor:
   ordered parameters (name, type, one-line description, optional + default value), and the result
   kind. The existing positional call syntax keeps working — the parser validates against the
   declaration instead of guessing, and a wrong type or arity is an error that names the parameter.
2. **Help generated from the signature.** The "Usage: …" part of the help string is produced from
   the declaration, never typed by hand again, so it can no longer drift from the binding.
3. **Typed results.** `Output` (or a result object beside it) gains a structured payload (JSON
   value, via the jsoncpp already vendored) and a binary payload (bytes + MIME type — a PNG
   capture first). The text rendering stays, for TCP 7777 and the local console.
4. **Behaviour hints** per command: read-only / destructive / idempotent. MCP carries them as tool
   annotations; they also say which commands may run without confirmation.
5. **Incremental migration.** 116 `bindCommand` sites in the engine + 13 in projet-alpha
   (2026-09-27 count). A command without a declared signature stays callable exactly as today, and
   is reported as "untyped" by a listing command, so the migration can be tracked to zero.
   Start with the commands an AI uses in every session: `RendererService.screenshot/temporalCapture`,
   the camera (`targetActiveScene`, `Act.setPosition/lookAt/setExposure`), `PostProcess.*`,
   `SettingsService.*`, `ResourcesManagerService.*`.
6. **Document it**: `src/Console/AGENTS.md` (§ "mandatory help string" becomes the signature
   rule), `docs/ai-runtime-control.md` (how to add a command).

## ⚠️ Traps

- `-fno-exceptions`: the descriptor and its validation return `bool`/`std::optional`, never throw.
- A command registered by projet-alpha must be able to declare its signature the same way — the
  contract lives in the engine, it is not an engine-only privilege.
- `screenshot()` blocks the main thread up to 5 s waiting for the frame
  (`src/Graphics/Renderer.console.cpp:56`). A binary result must not force a second copy of the
  image through the disk; decide whether the PNG bytes come from `FrameCapture` directly.
- Keep the command names as they are: renaming 129 commands for MCP naming rules is not part of
  this item (dots, letters, digits, `_` and `-` are all valid MCP tool-name characters).

## References

- MCP 2026-07-28 — Tools (`inputSchema`, `outputSchema`, `structuredContent`, image content,
  annotations): https://modelcontextprotocol.io/specification/2026-07-28/server/tools
- Follow-up items: `remote-console-response-framing`, `native-mcp-server`.
