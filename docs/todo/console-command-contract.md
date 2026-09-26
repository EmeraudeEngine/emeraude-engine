---
id: console-command-contract
title: Console commands — declared typed signatures and typed results
status: in-progress
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

## Done (2026-09-27)

The contract exists and is documented (`src/Console/AGENTS.md` § 3 "every new command is TYPED"
and § 7; `docs/ai-runtime-control.md` § "Argument syntax and validation"): typed `bindCommand()`
with types deduced from the lambda (owner choice over a declarative descriptor, 2026-09-27),
`Parameter` / `CommandSignature` / `CommandResult` / `CommandHint`, `Output` kinds Text / Json /
Binary, generated usage, per-parameter help lines, quote-aware parser, `listUntypedCommands()`.
Compile-time guards verified with scratch TUs (arity, return type, parameter type, trailing vector,
double default). Migrated: `WindowService` (2), `RendererService` (12), `SceneManagerService.PostProcess`
(6), `SettingsService` (4), `ResourcesManagerService` (4), `Core` (8 incl. aliases), projet-alpha
`Act` (13) — **52 typed, 85 untyped**.

## What remains

1. **Migrate the 85 untyped commands** (`listUntypedCommands()` lists them): `Scenes/Manager.console.cpp`
   (33 — the scene-creation vocabulary), `Audio/TrackMixer.console.cpp` (16 — `playlist(play, 3)`-style
   sub-commands: an action string + `std::optional` operands), `Net/APIClient.console.cpp` (12),
   `Input/Manager.cpp` (6), `Net/Manager.console.cpp` (5), `FileSystem`/`Arguments` (3 each),
   `Scenes/AVConsole` (2). Then delete the untyped `bindCommand` overload and `listUntypedCommands()`,
   and close this item.
2. **Escape the JSON the migrated commands build by hand** (`listResources`, `listContainers`,
   `remoteConsoleStatus`): a name with a `"` produces invalid JSON, which a machine client will reject.
3. **Binary result of `screenshot()`** — the kind exists (`CommandResult::binary`, `Output::binary`),
   no command uses it yet. Decide with `native-mcp-server` where the PNG bytes come from (see Traps).
4. **Declared result kind / output schema** — not done: results self-describe at runtime. Only worth
   adding if the MCP server needs an `outputSchema`.

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
