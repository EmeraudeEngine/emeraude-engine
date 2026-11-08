## Web API client (`Net::APIClient`)

**Files**: `APIClient.hpp/.cpp` (+ `APIClient.console.cpp`), `APIRequestItem.hpp`, `Types.hpp`.

**Identity**: `class APIClient final : public ServiceInterface, public Base::ObservableTrait,
public Console::ControllableTrait`, `ClassId = "NetAPIClientService"` (console path
`Core.NetAPIClientService`). Constructed in `PrimaryServices` with the `Settings` and the shared
`Base::ThreadPool`; registered to the console by `Core` next to the download manager.

### Why it is a separate service, and not `Net::Manager`

The owner ruled a distinct service on 2026-08-28. Four behaviours differ, each on purpose — a
future session must not "harmonise" them:

| | `Net::Manager` (downloads) | `Net::APIClient` (APIs) |
|---|---|---|
| **Cache / dedup** | URL-keyed disk cache; the same URL in flight folds onto one ticket | **None.** Two identical `POST`s are two calls; a polling `GET` must actually go out |
| **Retry** | A URL that failed retries on the same ticket | **None.** Replaying a write is how a payment gets charged twice |
| **Where the body lands** | Streamed to a file, never held whole | **Held in memory** — hence the retention ceiling below |
| **A non-2xx** | `Error` — a 404 is not a file | **`Done`** — an API says what went wrong in the body, and `responseStatusCode()` tells the caller |

### Contract

```cpp
int ticket = apiClient.post(URI{"https://api.example/actors"}, R"({"name":"paladin"})");
// … observe ResponseReceived, then, on the main thread:
if ( apiClient.requestStatus(ticket) == APIRequestStatus::Done )
{
    const auto httpStatus = apiClient.responseStatusCode(ticket);   // 201, 404, 422 …
    Json::Value document;
    if ( apiClient.responseJSON(ticket, document) ) { /* parsed on the worker */ }
    apiClient.release(ticket);                                       // ⚠️ MANDATORY, see below
}
```

- **Ticket lifetime — `release()` is part of the contract.** A response body sits in RAM. The
  caller calls `release(ticket)` once it has read it. A ceiling
  (`Core/Net/API/MaxRetainedTickets`, default 64) drops the **oldest terminal** tickets when the
  caller forgets, and logs **once** that it had to. ⚠️ That is a safety net, not a policy: the
  ticket it drops may be one nobody read yet. A ticket still in flight is **never** dropped.
- **A ticket that is not held is `APIRequestStatus::Unknown`** (2026-11-08): never issued, released, or dropped by
  the ceiling. It was `Error` before, and `requestFailure()` then answered `Success`: an evicted response read as a
  failed call whose reason was "Success" (found by the runtime run below). `requestFailure()` is meaningful only
  for an `Error` ticket. A consumer's `switch` over `APIRequestStatus` must handle `Unknown` (a `default:` already
  does: a downstream application that polls the status treats it as before).
- **`cancel(ticket)` frees the caller, not the socket.** `HTTPSClient` is synchronous and has no
  cancellation point: a call already on the wire runs to completion on its worker and its response
  is thrown away on arrival. A cancelled ticket emits **no** notification.
- **Authentication is set at runtime**, `setDefaultHeader("Authorization", "Bearer …")`, merged
  under the per-call headers (a per-call header of the same name wins). ⚠️ **Deliberately not read
  from `Settings`**: a token in `settings.json` would sit in cleartext on disk and be dumped by
  anything printing the settings.
- **Every header is validated twice** — once here, so a malformed one costs an `InvalidTicket` the
  caller sees immediately, and once in `HTTPSClient::run()`. See
  `HTTPSClient::isRequestHeaderAcceptable()`: RFC 9110 token names, no CR/LF/NUL/C0 in values, and
  the framing headers (`Host`, `Content-Length`, `Connection`, `Transfer-Encoding`,
  `Accept-Encoding`) refused outright.
- **JSON is parsed on the worker**, never on the main thread, and only when the server declared a
  JSON media type (`+json` suffixes included). A body that is not JSON is not an error: it stays
  readable through `responseBody()`.
- **One TLS connection per call** — `HTTPSClient` has no keep-alive reuse yet. Known cost on an
  API hit in rapid succession; fixing it is a base-level change to `TLSConnection`.

### Notifications

`RequestStarted` (payload: ticket) on the 0→1 in-flight edge, `ResponseReceived` (payload: ticket)
per completed call, `RequestsFinished` (no payload) on the 1→0 edge. All emitted from
`dispatchCompleted()`, which `Core` calls at the top of every main-loop cycle — **that is what puts
the observers on the main thread**. ⚠️ The batch edge is read from `m_inFlight`, not from the event
count: a cancelled ticket queues no event, and counting events would leave the batch "running"
forever after a batch that ended on a cancellation.

⚠️ `Core` registers the service to the console but does **not** observe it: unlike a download, an
API response is the business of whoever asked for it. Consumers observe `Net::APIClient` directly.

**The console's recorder** (owner, 2026-11-08): `watchEvents(1)` attaches a private observer
(`APIClient::NotificationLog`, a `Base::ObserverTrait`) and clears its log; `events()` returns what it received —
`sequence`, `elapsedMS`, `code`, `ticket` (none for `RequestsFinished`), and `onOwnerThread` (received on the thread
that constructed the service, the main thread); `watchEvents(0)` detaches it. Opt-in, so Core's "nobody wakes on
a response unless asked" holds. A ring of 64 entries without allocation (`dropped` counts the overwritten ones);
main thread only, no lock: `dispatchCompleted()` and the console's `poll()` both run there.

### Console

`Core.NetAPIClientService.{request,get,post,status,header,list,release,cancel,setHeader,removeHeader,headers,watchEvents,events,isEnabled}`.

> [!CAUTION]
> **Owner decision, 2026-08-28: this console surface is FULLY EXPOSED.** `request()` accepts an
> arbitrary URL, method and body, and `headers()` prints the default headers — **bearer token in
> clear**. Redaction was offered and the owner chose full exposure for debugging comfort. **Do not
> "fix" this into redaction without asking**: it is a decision, not an oversight. What contains it
> is that the remote console is closed by default and binds `127.0.0.1`
> (`Core/Console/EnableRemoteListener`).

### Settings

| Key | Default | What it does |
|---|---|---|
| `Core/Net/API/Enabled` | `true` | Whether calls are performed at all |
| `Core/Net/API/TimeoutSeconds` | `30` | Total budget of one call, redirects included |
| `Core/Net/API/MaxRetainedTickets` | `64` | Terminal tickets kept before the oldest are dropped; `0` disables the ceiling |
| `Core/Net/API/MaxResponseBytes` | `16 MiB` | Ceiling of ONE response body — it is held whole in RAM |
| `Core/Net/CABundleFile` | `""` | **Shared with the download manager**: a corporate CA is a property of the machine |

### ⚠️ Cross-platform status

`APIClient` itself is pure C++ over `HTTPSClient` — no syscall, no platform leg. What it rides on
(`HTTPSClient`, `TLSConnection`, `TrustStore`) is verified on Linux, macOS **and** Windows, the `Net::Manager`
`ExternalData` chain included (emeraude-base `docs/plans/network-tls/cross-platform-validation.md`, closed
2026-10-07).

**The service has run on the three OS since 2026-10-07, but only part of it** (corrected 2026-10-13: this
paragraph said "only ever run on Linux", the board item said "never executed"; both were stale):
- a downstream application uploads through it on Linux, macOS and Windows: a JSON `POST` then a `PUT` of an
  archive, both answered 2xx, plus rejected calls on Linux against a test server. That path exercises `request()`,
  the polled `requestStatus()` / `responseStatusCode()` / `responseBody()` / `responseJSON()` (the worker-side
  parse) / `requestFailure()`, and `release()`;
- the console on Linux (2026-10-07, [`07-critical-points.md`](07-critical-points.md) § the redacted URL): `get()`
  answered `HTTP 503` and `list()` showed the ticket.

**The whole service ran on Linux on 2026-11-08** (test instance, ports 7780 / 17779, a settings copy with
`MaxRetainedTickets = 8` and `CABundleFile` = a scratch test CA; a local Python HTTPS server for what a public server
cannot give on demand; 0 `VUID-`):
- **notifications**, through `watchEvents()` / `events()`: one call → `RequestStarted`, `ResponseReceived`,
  `RequestsFinished`; a batch of two → ONE `RequestStarted`, two `ResponseReceived` in completion order, ONE
  `RequestsFinished`; every one `onOwnerThread: true`; a 70-call burst wrapped the 64-entry ring exactly (210
  received, 146 dropped, sequences 147-210 kept);
- **`cancel()`** of the only call in flight (a 3 s reply): `Cancelled`, NO `ResponseReceived`, `RequestsFinished`
  still emitted when the exchange ended (3.0 s); a second `cancel()` refused; `release()` refused while in flight;
  cancelling one of two: the other `ResponseReceived`, ONE `RequestsFinished` after both;
- **JSON**: a JSON body → `jsonParsed: true`; a body declaring `application/json` that is not → `Done` 200,
  `jsonParsed: false`;
- **strict reception** (base, 2026-11-07): a non-token header name, a control character in a header value or in the
  reason phrase → `Error`, reason `Protocol`, the base refusal in the log;
- `404` → `Done` 404; `post()` and `request(PUT, …)` → 201 with the default `Authorization` sent, `removeHeader()`
  stopped it; `header(ticket, ETag)`; `list()`; `release()` (a second one refused);
- **public servers**: GitHub raw → 200 (`text/plain`); `expired.badssl.com` → `Error` `TLSFailure`; the GitHub API
  → 200, `jsonParsed: true`, `ETag` read; a missing GitHub endpoint → `Done` 404;
- **retention**: two bursts of 12 at a ceiling of 8 → 8 held, the oldest dropped, the warning logged ONCE;
- **`Unknown`**: a never-issued, a released and an evicted ticket → `"status":"Unknown"` with a note (before the
  fix: `"status":"Error","reason":"Success"`);
- console conformance 5247/5247, MCP conformance 2019/2019.

`APIClient` is pure C++: the full run was not replayed on macOS and Windows (owner, 2026-11-08). The peers built
`cb642962` (0 warning, MSVC Release + Debug, AppleClang) and ran a test instance (0 `VUID-`): `status(9999)` →
`Unknown` with its note, and `events()` after a GitHub GET → `RequestStarted`, `ResponseReceived`, `RequestsFinished`,
all `onOwnerThread: true`. MCP conformance 2019/0 on both; console conformance 5247/0 on Windows, 5235/0 on macOS
(the 12-check gap is by design, not a defect: without an acceleration-structure builder the M6 never registers
the four ray-traced occupants, so `PostProcess.listEffects()` prints 15 outputs instead of 19, 3 checks each — see
[`ai-runtime-control.md`](../../ai-runtime-control.md) § The conformance bench).
