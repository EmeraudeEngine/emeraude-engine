# Resource sharing — an engine serves its data stores, a peer engine fetches what it lacks (2026-10-04)

The three development machines do not hold the same assets (`WorldLobby.usdz`, 1.6 GB, is ignored by the data
repository and lived on Linux only). The owner asked (2026-10-04) for the engine to serve its resources itself,
**off by default**. Two classes, both in `src/Resources/`:

| Class | Side | Role |
|---|---|---|
| `SharingServer` | server | Read-only HTTP view of the data stores, on emeraude-base `Network::HTTPServer` |
| `PeerStore` | client | Reads a peer's index at start-up; copies a peer's files into the local data stores |

`Resources::Manager` owns both: `connectPeer()` runs in `onInitialize()` BEFORE the containers capture their
stores, `startSharingServer()` after them; `onTerminate()` stops the server first, then the peer.

## Owner decisions (2026-10-04)

1. **Scope — files + peer stores.** References: Godot's remote filesystem (`--remote-fs`: the editor serves the
   project files to a device), Unreal's network file server / cook-on-the-fly.
2. **One HTTP server in emeraude-base.** The HTTP/1.1 code that lived in `Console/MCP/Server.cpp` became
   `Base::Network::HTTPServer`; the MCP server and the sharing server both use it (base
   `docs/subsystems/source-tree/24-network-http-server.md`).
3. **Security — the MCP rule.** Off by default, `127.0.0.1` by default; a non-loopback address refuses to start
   without a bearer token. Read-only; every path from the network confined (`IO::confinedPath()`).
4. **Transport for a peer — cleartext to private addresses only** (`HTTPSClientOptions::allowPrivateCleartext`).
   The token crosses the LAN in clear: a home-network development tool. TLS with a pinned certificate is item
   `resource-sharing-tls`.
5. **Coverage — index resources + explicit file fetch.** Resources named in the peer's index are added as
   `ExternalData` downloads (lazy, `Net::Manager` cache); a file loaded BY PATH (`WorldLobby.usdz`, FBX / glTF
   scenes) is copied by the `fetchFromPeer()` command — never by a lookup that would block a loader thread.

## Settings

| Key | Default | Meaning |
|---|---|---|
| `Core/Resources/Sharing/Enabled` | `false` | Serve this engine's data stores |
| `Core/Resources/Sharing/Address` | `127.0.0.1` | Bind address (a LAN one needs the token) |
| `Core/Resources/Sharing/Port` | `17790` | TCP port |
| `Core/Resources/Sharing/BearerToken` | `""` | Mandatory off loopback |
| `Core/Resources/Peer/URL` | `""` | A peer's server, `http://host:port`; empty = none |
| `Core/Resources/Peer/BearerToken` | `""` | The peer's token |

## The server's endpoints (GET; HEAD where a body exists)

| Path | Answer |
|---|---|
| `/index.json` | `{"FormatVersion", "Stores": {store: [entry]}}` — a local file as `{"Name", "Path", "Size"}` (`Path` relative to `data-stores/`, never this machine's absolute path), an external or direct entry as it is. Sorted: two calls give the same document. |
| `/files/<path>` | The file, streamed in 256 KiB chunks, `Accept-Ranges: bytes`, a single `Range` honoured (206 / 416) |
| `/list/<directory>` | `{"files": [{"path", "size"}], "truncated"}`, recursive, at most 100 000 entries |
| `/sha256/<path>` | `{"sha256", "size"}`, computed on the thread pool (at most 4 at once, else 503 + `Retry-After`) |

A path is percent-decoded, then refused if it holds a NUL or a backslash (a separator on Windows only: one request
would mean two things on two OS), then confined under each data directory's `data-stores/`; the first data
directory holding it wins, as for a local load. Every refusal is a 404 (a traversal included).

## The peer side

- **Index merge** (`Manager::mergePeerIndex()`, start-up, bounded: 3 s connect, 30 s total): a name the local
  store already has is skipped (a local resource always wins); a `Path` entry becomes
  `{"Source": "ExternalData", "Data": "<peer>/files/<path>"}`. The peer is registered in `Net::Manager`
  (`registerPeer()`): its `http://` URLs become downloadable and carry its token; every other `http://` URL stays
  refused.
- **File fetch** (`PeerStore::fetch()`, thread pool, one at a time): the peer lists the directory (or the path is
  taken as a file); into the first data directory that has a `data-stores/`; a file present with the peer's size
  is kept; otherwise written to `<file>.part`, its SHA-256 compared with the peer's `/sha256/`, then renamed. A
  mismatch, a cancel or an error deletes the side file. Cancel (`cancelPeerFetch()`, shutdown) stops at the next
  transport read or during the verification hash.

## Console / MCP (`Core.ResourcesManagerService.*`)

| Command | Use |
|---|---|
| `sharingStatus()` | Server URL (when serving) and peer URL |
| `fetchFromPeer(path)` | Copy `path` (a file or a directory, relative to `data-stores/`) from the peer, in the background |
| `peerFetchStatus()` | Files listed / fetched / kept / failed, the current file's bytes, the errors |
| `cancelPeerFetch()` | Stop the running fetch |

## Measurements (Linux, 2026-10-04, two instances on one machine over 192.168.1.10)

- Index: 25 stores, 14 396 entries, 2.17 MB, 52 ms. The client (Musics and USD stores removed) merged 484
  resources in 70 ms; `loadResource('MusicResource', 'AirWolf8')` downloaded it from the peer with the token and
  loaded it; a local playlist pulled 33 Kyrandia MIDI files through the same path (names with spaces and
  apostrophes).
- `fetchFromPeer('USD/WorldLobby.usdz')`: 1 610 429 961 bytes in 8 s including both SHA-256 passes; the copy's
  `sha256sum` equals the original (`9228517f…1f047c`). A second fetch kept it; `Musics/Kyrandia2` fetched 32 files;
  a cancel left no `.part`.
- The server's first `/sha256/` of that file was WRONG: emeraude-base's SHA-1/256/512 wrote only the low 32 bits of
  the bit length, so every input of 512 MiB or more hashed wrong. Fixed in base (failing-then-passing
  `Hash.lengthFieldPastFourGigabits`, base `docs/caution-points.md` § Hash).
- Refusals: no token / wrong token 401, POST 405, `../../etc/passwd`, `%2e%2e/…`, `..%2f`, `%2fetc`, `%5c` all 404.
- Shutdown with a fetch in flight: 6.0 s, the same as without one (6.6 s).

## ⚠️ Limits

- A multi-file resource reached through the index (a `.gltf` with its `.bin` beside it) is downloaded alone into
  the cache, under a hashed name: its siblings are not there. Copy such a resource with `fetchFromPeer()` (a
  directory fetch takes the siblings).
- One peer only (`Core/Resources/Peer/URL`).
- The index is merged once, at start-up: a resource the peer gains later needs a restart.

## Validated on the three OS (2026-10-04)

- **Windows**: build 0 warnings; base 2342 + 3 skipped; MCP conformance 1826/0; a Windows peer of the Linux server
  merged 4505 resources (the Linux data repository's git-ignored `data-stores/ExternalData/`, 4504 entries — the 9892
  others were already local) in 110 ms; `WorldLobby.usdz` fetched in 15.8 s, SHA-256 equal; a cancel 0.3 s later left
  nothing; as a server, every index `Path` uses '/', a Range 206 byte-exact, `../` and `%5C` 404.
- **macOS M2**: build 0 warnings; base Release 2342 + 3, ASan 2345/2345; MCP conformance 1826/0; peer: 4505 merged
  in 97 ms; `Musics/Kyrandia2` 32 kept; `WorldLobby.usdz` fetched in about 26 s, byte-identical. The fetch target
  there is the bundle's `Contents/Resources/data/data-stores` (a link to the data repository).
- `currentBytes` is the per-file progress; `bytesReceived` adds a file once verified. The last seconds of a large
  file are the two SHA-256 passes, then the rename.
