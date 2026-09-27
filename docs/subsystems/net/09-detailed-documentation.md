## Detailed Documentation

Related systems:
- [`../Resources/AGENTS.md`](../../../src/Resources/AGENTS.md) - Fail-safe loading system, `ExternalData` source type, the `ServiceAccess` firewall
- [`../../docs/resource-management.md`](../../resource-management.md) - Resources architecture
- emeraude-base `src/Network/` (`HTTPSClient`, `TLSConnection`, `TrustStore`, `URI`) - the HTTPS stack the manager runs on
- emeraude-base `src/Network/HTTPSClient.hpp` `DownloadProgress` - the hook behind the Progress notification
- emeraude-base `src/Network/HTTPSClient.hpp` `HTTPRequestOptions` / `request()` / `isRequestHeaderAcceptable()` - the API-traffic entry point `Net::APIClient` runs on
