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

### Console

`Core.NetAPIClientService.{request,get,post,status,header,list,release,cancel,setHeader,removeHeader,headers,isEnabled}`.

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
(`HTTPSClient`, `TLSConnection`, `TrustStore`) cleared steps 1-3 of the handover on Linux, macOS
**and** Windows (2026-08-28). **The service itself has only ever run on Linux**, exactly like
`Net::Manager` — see the handover item, it is the same gap.
