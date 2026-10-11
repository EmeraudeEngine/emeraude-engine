---
id: apiclient-runtime-verification
title: "Net::APIClient — the service has never executed, on any platform"
status: open
priority: high
scope: src/Net
opened: 2026-08-28
tags: [network, api, verification]
---

# Net::APIClient — the service has never executed, on any platform

## Why

`Net::APIClient` landed on 2026-08-28 (owner-decided service, ticket + observable notification).
What it stands on is verified: `HTTPSClient::request()` carries 18 dedicated unit tests and the
emeraude-base suite is **2028/2028** on Linux, and the whole HTTPS stack cleared steps 1-3 of the
cross-platform handover on Linux, macOS **and** Windows.

**The service itself has only ever been compiled.** Not one call has left it at runtime. Everything
below is unverified behaviour, not verified behaviour:

- the ticket lifecycle and its three notifications (`RequestStarted`, `ResponseReceived`,
  `RequestsFinished`) reaching an observer **on the main thread**;
- the retention ceiling actually dropping the oldest terminal tickets, and warning once;
- `cancel()` on a call in flight — the result must be dropped and NO notification emitted, while
  the batch edge still fires;
- the worker-side JSON parsing, including a body that declares JSON and is not;
- the console surface (12 commands) end to end.

⚠️ The precedent is `Net::Manager`, which was a semi-stub that never completed a download for an
unknown number of sessions while looking perfectly plausible. A service that compiles proves
nothing.

## What remains

**Nothing blocks it any more (2026-10-11).** The two former blockers are gone:
- The remote console no longer needs the owner's `settings.json`. A test instance opens it for one run with
  `--enable-remote-console=PORT` (and `--enable-mcp=PORT`), and the settings are never written (a3d828953).
- Launching projet-alpha is allowed: say which demo is being launched (owner, 2026-09-07).

Launch a test instance on other ports (`--enable-remote-console=7780 --enable-mcp=17779`,
`.claude/rules/build-and-run.md`), then run from the console:

```
Core.NetAPIClientService.isEnabled()
Core.NetAPIClientService.get(https://raw.githubusercontent.com/EmeraudeEngine/emeraude-base/main/README.md)
Core.NetAPIClientService.status(1)          # Done + httpStatus 200 + body
Core.NetAPIClientService.get(https://expired.badssl.com/)
Core.NetAPIClientService.status(2)          # Error + "reason":"TLSFailure"  <- the trust store working
Core.NetAPIClientService.get(https://api.github.com/repos/EmeraudeEngine/emeraude-base)
Core.NetAPIClientService.status(3)          # jsonParsed:true  <- the worker-side parse
Core.NetAPIClientService.get(https://api.github.com/nonexistent-endpoint)
Core.NetAPIClientService.status(4)          # ⚠️ Done, httpStatus 404 — NOT Error
Core.NetAPIClientService.header(3, ETag)
Core.NetAPIClientService.list()
Core.NetAPIClientService.release(1)
Core.NetAPIClientService.setHeader(Authorization, Bearer test) ; Core.NetAPIClientService.headers()
```

Then a `post()` against a real endpoint, and a `cancel()` issued while a slow call is in flight.

## ⚠️ Traps

- **`Done` is not success.** A 404 is `Done`. Only a call that never completed is `Error`. A test
  that asserts "Done ⇒ it worked" verifies nothing.
- **Read the log next to the console output.** The retention warning is emitted **once** per
  session; miss it and an evicted response looks like a client bug.
- The remaining `ExternalData` step of the sibling handover item
  (`emeraude-base/docs/todo/tls-stack-windows-macos-validation.md`) is the same class of gap: the
  service layer above the HTTPS stack is what nobody has run.

## References

- [`../../src/Net/AGENTS.md`](../../src/Net/AGENTS.md) § Web API client — contract and the four
  deliberate differences with `Net::Manager`
- [`../ai-runtime-control.md`](../ai-runtime-control.md) § Calling a web API — the console commands
- projet-alpha `docs/caution-points.md` § Web API calls — the traps a consumer meets
