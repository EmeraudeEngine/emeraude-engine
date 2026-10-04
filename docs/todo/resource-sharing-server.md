---
id: resource-sharing-server
title: Resource sharing — an engine serves its data stores, a peer engine fetches what it lacks
status: in-progress
priority: unranked
scope: src/Resources, src/Console/MCP, src/Net
opened: 2026-10-04
tags: [network, resources, tooling]
---

# Resource sharing — an engine serves its data stores, a peer engine fetches what it lacks

## Why

The three development machines do not hold the same assets: `WorldLobby.usdz` (1.6 GB) is ignored by
the data repository and lived on the Linux machine only, so the Windows peer could not validate the
USD loader fix (2026-10-04) until a hand-started `python3 -m http.server` served the file over the
LAN. The owner asked for that to be an engine feature, **off by default**.

## Owner decisions (2026-10-04)

1. **Scope — files + peer stores.** The server publishes the data-store files read-only, and a JSON
   index of every store resource (name, type, size, SHA-256, URL). Another engine adds that index as a
   remote store. References: Godot's remote filesystem (`--remote-fs`, the editor serves the project
   files to a device), Unreal's network file server / cook-on-the-fly.
2. **HTTP layer — one server in emeraude-base.** The HTTP/1.1 server embedded in
   `src/Console/MCP/Server.cpp` moves to `EmEn::Base::Network::HTTPServer` (item `http-server` in
   emeraude-base); the MCP server and the sharing server both build on it. The MCP server is re-checked
   with `tools/mcp-conformance.py`.
3. **Security — the MCP rule.** Off by default, bound to `127.0.0.1`; a non-loopback address refuses to
   start without a bearer token. Read-only, confined to the data stores (`IO::confinedPath()`), files
   streamed in chunks, never loaded whole.
4. **Transport for a peer — cleartext to private addresses only.** The base client gains a cleartext
   HTTP path restricted to loopback, RFC 1918, link-local and IPv6 ULA addresses (item
   `cleartext-http-private-addresses` in emeraude-base). Public URLs stay HTTPS-only. TLS for the
   sharing server is a later item (`resource-sharing-tls`).
5. **Coverage — index resources + explicit file fetch.** (a) The remote index points every entry at the
   server; the client merges the names it lacks locally; the existing `ExternalData` path downloads them
   lazily into the `Net::Manager` cache. (b) A file loaded by path (`WorldLobby.usdz`, FBX/glTF scenes)
   is fetched from the peer into the local data stores by an explicit console/MCP command — never by a
   lookup that would block a loader thread for minutes.

## What remains

Implemented and proven on Linux 2026-10-04 (`docs/subsystems/resources/12-resource-sharing.md` § Measurements;
base `Network::HTTPServer`, the cleartext path, the MCP server moved onto it — conformance 1802/0).

- Validation on macOS and Windows: a peer fetching `WorldLobby.usdz` from the Linux server (SHA-256 equal), the
  index merge, a lazy `loadResource()` of a peer music, the MCP conformance on the moved server.

## ⚠️ Traps

- The MCP server's Host check accepts only loopback names when bound to loopback; a LAN binding is
  protected by its token instead. Keep that split in the shared server.
- A 1.6 GB file must never sit in memory on either side: the server streams it, the client writes it to
  a file (`HTTPSClient::download()` already does).
- The data-store paths come from the NETWORK on the server side: `IO::confinedPath()` on every one.

## References

- `src/Console/MCP/Server.cpp` — the HTTP/1.1 server to extract.
- `src/Net/Manager.hpp` — the download cache (HTTPS-only until decision 4).
- `src/Resources/BaseInformation.cpp` — `ExternalData` entries.
