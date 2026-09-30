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

- ✅ 2026-09-30 (triad 6c): `getNode()` / `getNodePhysics()` write their vectors through `writeJSONVector()`
  (`Scenes/Manager.console.cpp`: `null` for a non-finite component, 9 significant digits — the float round trip),
  the same rule as `getFrameDiagnostics()`' local `number` lambda.
- The other hand-streamed answers (`CommandResult::json(<stream>.str())`), census 2026-09-30:
  `Graphics/Renderer.console.cpp` ×3 (lines ~487, 558, 577), `Net/APIClient.console.cpp` ×4 (~211, 249, 327, 343),
  `Window.console.cpp` ×1 (~68). Check each for a float; apply the rule.
- Then consolidate: ONE console helper for "a float as a JSON number" instead of the two local copies.

## References

- Leftover of the typed command contract work (2026-09-27, item `console-command-contract`, closed).
- An optional `outputSchema` per command was the other leftover; it is tracked as a possible extension
  in `native-mcp-server`.
