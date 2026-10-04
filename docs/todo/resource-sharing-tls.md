---
id: resource-sharing-tls
title: Resource sharing over TLS — a pinned self-signed certificate instead of cleartext
status: open
priority: unranked
scope: src/Resources, emeraude-base src/Network
opened: 2026-10-04
blocked-by: [resource-sharing-server]
tags: [network, tls, security]
---

# Resource sharing over TLS — a pinned self-signed certificate instead of cleartext

## Why

The owner chose cleartext HTTP restricted to private addresses for the first version of resource
sharing (`resource-sharing-server`, decision 4, 2026-10-04). The bearer token and the assets therefore
cross the LAN in clear: acceptable on a home network for a development tool, not beyond.

## What remains

- Server-side TLS in emeraude-base (`asio::ssl` server context on `Network::HTTPServer`; base has the client
  side only). Base's test helpers already generate a server certificate (`src/Testing/TLSTestHelpers.hpp`,
  `generateServerCredentials()`): a starting point for the generation below.
- A self-signed certificate generated on first start and kept beside the settings.
- The peer pins the certificate's SHA-256 fingerprint in its settings (SSH `known_hosts`, Syncthing
  device IDs), so no certificate authority is involved.
- Then the cleartext path can be dropped or kept as an explicit opt-in.

## References

- `docs/todo/resource-sharing-server.md` — the decisions.
- emeraude-base `src/Network/TLSConnection.hpp` — the client side.
