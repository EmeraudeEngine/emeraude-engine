## Module Overview

Three things live under `src/Net/`:

1. **`Net::Manager`, the download manager** — a service that fetches `https://` files into a
   URL-keyed cache through emeraude-base's `HTTPSClient`, for the `Resources` system
   (`"Source": "ExternalData"` entries) and for anything else that needs a file from the network.
   Drivable from the console (`Core.NetManagerService.*`). Rebuilt 2026-08-27 on the owner's
   decision — until then it was a semi-stub that never completed a download.
2. **`Net::APIClient`, the web API client** — a service that performs arbitrary HTTPS exchanges
   (`GET`/`POST`/`PUT`/`PATCH`/`DELETE`, caller headers, request body) and hands the response back
   **in memory**, on the main thread, addressed by a ticket. Drivable from the console
   (`Core.NetAPIClientService.*`). Added 2026-08-28. ⚠️ **It is not `Net::Manager` with a different
   verb** — see § Web API client for the four deliberate differences.
3. **The hardware & discovery utilities** (`UDPClient`, `NetworkInterfaces`, `TCPClient`,
   `TCPServer`, `SerialPort`, `WiFiScanner`) — standalone, `noexcept`, cross-platform classes
   with no dependency on the manager, consumed by downstream applications through scripting
   bridges (TCP/UDP/serial/WiFi modules). Documented in § Hardware & Discovery Utilities.

**Not multiplayer**: nothing here is gameplay networking, and nothing should become it without
an owner decision.
