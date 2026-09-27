---
id: console-json-non-finite-numbers
title: Console JSON answers written with a stream can contain NaN/Inf, which is not JSON
status: open
priority: unranked
scope: Console / Scenes/Manager.console.cpp
opened: 2026-09-27
tags: [console, mcp, json]
---

# Console JSON answers written with a stream can contain NaN/Inf, which is not JSON

## Why

Most console JSON answers are now built with jsoncpp (`FastJSON::stringify`, or `MCP::serialize` for
full precision), which escapes strings. A few still stream numbers by hand to keep full precision —
`SceneManager.getNode()` and `getNodePhysics()` (positions, velocity). A non-finite float there would
print `nan`/`inf`, which is invalid JSON: the MCP server then drops the `structuredContent` and a client
parsing the text fails. `getFrameDiagnostics()` already writes `null` for a non-finite value.

## What remains

1. Write those numbers through one helper that emits `null` for a non-finite value (or build them with
   jsoncpp and a full-precision writer, `Console::MCP::serialize()`).
2. Grep for other hand-streamed JSON answers (`R"(\"…\":)" <<`, `json << "`) and apply the same rule.

## References

- Leftover of the typed command contract work (2026-09-27, item `console-command-contract`, closed).
- An optional `outputSchema` per command was the other leftover; it is tracked as a possible extension
  in `native-mcp-server`.
