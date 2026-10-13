---
id: apiclient-runtime-verification
title: "Net::APIClient — the half of the service no run has exercised (notifications, retention, cancel, console)"
status: open
priority: high
scope: src/Net
opened: 2026-08-28
tags: [network, api, verification]
---

# Net::APIClient — the half of the service no run has exercised (notifications, retention, cancel, console)

## Why

`Net::APIClient` landed on 2026-08-28 (owner-decided service, ticket + observable notification).
What it stands on is verified: `HTTPSClient::request()` carries 18 dedicated unit tests and the
emeraude-base suite is **2028/2028** on Linux, and the whole HTTPS stack cleared steps 1-3 of the
cross-platform handover on Linux, macOS **and** Windows.

**The service has run, but only part of it** (corrected 2026-10-13: this item said "never executed, on any
platform", the subsystem doc said "only ever run on Linux"; both were stale). Since 2026-10-07:
- a downstream application uploads through it on Linux, macOS and Windows — a JSON `POST`, then a `PUT` of an
  archive, both 2xx, plus rejected calls on Linux against a test server. It polls: `request()`,
  `requestStatus()`, `responseStatusCode()`, `responseBody()`, `responseJSON()` (the worker-side parse of a JSON
  body), `requestFailure()`, `release()`;
- the console on Linux: `get()` answered `HTTP 503`, `list()` showed the ticket (`docs/subsystems/net/07-critical-points.md`,
  the redacted-URL entry).

Everything below has still never run anywhere — unverified behaviour, not verified behaviour:

- the ticket lifecycle and its three notifications (`RequestStarted`, `ResponseReceived`,
  `RequestsFinished`) reaching an observer **on the main thread**;
- the retention ceiling actually dropping the oldest terminal tickets, and warning once;
- `cancel()` on a call in flight — the result must be dropped and NO notification emitted, while
  the batch edge still fires;
- the worker-side JSON parsing of a body that declares JSON and is not (a valid JSON body was parsed);
- the console surface (12 commands) end to end (only `get` and `list` ran, on Linux).

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
- The HTTPS stack below it is now verified end to end on the three OS, `ExternalData` chain included (the base
  handover item closed in base `6cf4d7e`, Windows W8 all rows pass on 2026-10-07; record:
  `emeraude-base/docs/plans/network-tls/cross-platform-validation.md`). Only the service layer above it is
  unrun.

## References

- [`../subsystems/net/03-web-api-client.md`](../subsystems/net/03-web-api-client.md) — contract and why it is a
  separate service from `Net::Manager`
- [`../ai-runtime-control.md`](../ai-runtime-control.md) § Calling a web API — the console commands
- projet-alpha `docs/caution-points.md` § Web API calls — the traps a consumer meets
