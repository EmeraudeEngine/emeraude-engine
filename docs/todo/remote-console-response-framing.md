---
id: remote-console-response-framing
title: Remote console (TCP 7777) — delimit each response on the wire
status: open
priority: unranked
scope: Console/RemoteListener
opened: 2026-09-27
tags: [console, network, ai-runtime]
---

# Remote console (TCP 7777) — delimit each response on the wire

## Why

`RemoteListener::respond()` writes `message + "\n"` and stops
(`src/Console/RemoteListener.cpp:272`). A message routinely contains newlines itself (`help`, a JSON
dump, a multi-`Output` answer), so the client **cannot know where a response ends**. The Python
clients guess it from silence: `tools/emeraude_console.py` reads until a quiet period and EXTENDS
its deadline whenever bytes arrive. A slow frame (a 5 s `screenshot()`, a scene load) therefore
either truncates an answer or makes every command pay the full timeout, and two commands sent
back-to-back can have their answers merged.

The owner decided (2026-09-27) to **keep TCP 7777 beside the future MCP endpoint**, on the same
command registry and behind the same `Core/Console/EnableRemoteListener` gate: it stays the
simplest channel for benchmark scripts. It must then become a request/response protocol.

## What remains

1. Choose the framing (decision to present to the owner with trade-offs): an end-of-response
   marker line, or a length prefix, or one JSON object per line (`{"ok":…,"outputs":[…]}`). The
   last one also carries the severity of each `Output`, which the text stream loses today.
2. Keep interactive use possible (`nc`, the REPL of `tools/remote-console.py`): the framing must
   stay readable, or be negotiated per connection (e.g. a first `mode json` line).
3. Update `tools/emeraude_console.py` / `tools/remote-console.py` to read up to the delimiter —
   and delete the quiet-period heuristic and its explanation instead of keeping both.
4. Update `docs/ai-runtime-control.md` § Connection and `src/Console/AGENTS.md` § 1 (response
   format).

## ⚠️ Traps

- **A command with no output sends NOTHING** (`Controller::poll()`, `Controller.cpp` — the response is
  written only `if ( !outputs.empty() )`): the client cannot tell "no answer" from "slow answer" and
  waits for its whole timeout. The framing must answer every request, even an empty one (seen
  2026-09-27 while testing `console-command-contract`).
- The welcome banner and the `ERROR: …` lines written by the network thread
  (`RemoteListener.cpp:74`, `:124`, `:334`, `:410`) must follow the same framing, or a client
  mis-reads them as the answer to its next command.
- Every external script that parses the current text answers (benches under `tools/`, projet-alpha
  `tools/`) must be listed and migrated in the same change.

## References

- Independent of `console-command-contract`; a prerequisite of nothing, but it removes the same
  "silence = end" guess the MCP transport must never make.
