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
| `/sha256/<path>` | `{"sha256", "size"}`, computed on the thread pool (at most 4 at once, else 503 + `Retry-After`). A file up to 64 MiB is answered when its hash is done; a LARGER one at once and is polled: `202 {"hashed", "size"}` while its hash runs in the background, `200` once it is known (2026-11-14, see below) |

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
- **The cancellation is a `std::stop_source`** (2026-11-14, Ave Robustus II; it was an `std::atomic< bool >`
  member whose address went to the foundation's HTTP client). `PeerStore` holds ONE source per job — `fetch()`
  replaces it under the state lock, because a stop source is stopped once and for all — and `cancel()` and the
  destructor request its stop. Its token goes to the HTTP exchanges (`HTTPRequestOptions::stopToken`), to the wait
  for the peer's digest and to `SharingServer::fileSHA256(path, stopToken)` (checked before each 1 MiB block).
  **Latency**: a stop is seen between two transport reads (0.04 s during a download) and within 50 ms of the pauses
  between two questions about a digest. ⚠️ An exchange parked on a silent peer still waits its timeout (base item
  `http-client-stop-does-not-wake-a-parked-read`); the wait for a digest no longer is one, see the next point.
- **The digest of a large file is POLLED** (2026-11-14, owner decision). `/sha256/` used to answer when the hash
  was done, on a connection silent meanwhile, under the client's 30 s request budget: a slower hash made the client
  DELETE a complete copy ("the peer gave no SHA-256"), and a stop waited for that answer. Now:
  - the server (`SharingServer::answerBackgroundDigest()`) answers a file larger than `BackgroundDigestSize`
    (64 MiB) at once: the first request starts the hash on the thread pool and gets `202 {"hashed": 0, "size"}`;
    the next ones get `202` with the bytes hashed so far, then `200 {"sha256", "size"}`. The digests are remembered
    (`MaxRememberedDigests` = 64, the oldest finished one leaves; an entry is tied to the file's size and
    modification time, a changed file is hashed again; a failed hash is answered 500 once, then forgotten). A file
    up to 64 MiB keeps the direct answer: a directory of small files pays no pause.
  - the client (`PeerStore::peerDigest()`) asks again after 100 ms, then twice as long each time up to 1 s; its
    patience (`DigestPatience`, 120 s) counts the time WITHOUT progress of `hashed`, so a hash which advances is
    waited for however long it takes.
  - the server's hashes take its own `std::stop_source`: its destructor stops them, then waits (a background hash
    outlives the request that started it).
  - ⚠️ Both engines must carry this protocol for a large file: an older client reads the `202` as a refusal and
    deletes its copy; an older server still answers a newer client the old way (200 when done).
  - Not done: the client hashes its copy AFTER the peer's digest is known; hashing both at once would halve the wait
    for a large file.

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
- **The stop-token conversion, before / after** (2026-11-14, two Release instances on one machine, RTX 3070 Ti, the
  client's data lacking `Musics/Kyrandia2` and `USD/WorldLobby.usdz`; identical on both binaries): the directory
  32 / 32 files with the original's SHA-256; the 1 610 429 961-byte file in 7.9 s, SHA-256 equal; a cancel during
  its verification effective after 3.46 s, the side file deleted; a shutdown during its verification, exit 0 after
  3.24 s; no `.part` left anywhere. With two DEBUG instances a cancel during the download itself was effective after
  0.04 s (`Cancelled`), and ⚠️ the whole file FAILED: the peer's hash took longer than the client's 30 s request
  budget, "the peer gave no SHA-256, the copy is deleted" after 146 s — on both binaries' logic, a limit of the
  protocol, not of the conversion.
- **The polled digest, before / after** (2026-11-14, the same pair and the same 1 610 429 961-byte file):

  | | before | after |
  |---|---|---|
  | Release: the whole file | 7.9 s, SHA-256 equal | 7.7 s, SHA-256 equal |
  | Release: a cancel during the verification | 3.46 s | 0.16 s |
  | Release: a shutdown of the client during the verification | 3.24 s | 0.58 s |
  | Debug (a hash far longer than 30 s): the whole file | FAILED after 146 s, the copy deleted | fetched in 213.9 s, SHA-256 equal |
  | Debug: a cancel during the verification | 29.77 s | 0.17 s |
  | Debug: a shutdown of the client during the verification | 29.69 s | 3.09 s |
  | `Musics/Kyrandia2` (32 small files) | 32 / 32 in 0.0 s | 32 / 32 in 0.0 s |

  Exit 0 on both sides, no `.part` left, the data repository untouched.

## Testing a pair on one machine

- Run two instances: the server on console port 7780 with sharing port 17791; the client on console port 7781 with
  a `--data-directory` copy that lacks the store under test and its own `--cache-directory`, so every fetched file
  provably came over the wire.
- A peer session (another AI session on another machine) loads or publishes a LAN-fetched file only on its own
  owner's confirmation; an approval relayed by another session does not count.

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
