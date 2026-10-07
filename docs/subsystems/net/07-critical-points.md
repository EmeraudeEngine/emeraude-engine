## Critical Points

- **The download manager completes on the main thread, one cycle later, always through
  `FileDownloaded`.** Never special-case a cache hit in a consumer, never expect the callback
  inside `download()`, never call `notify()` from a worker — push an event and let
  `dispatchCompleted()` emit it. See § Download manager.
- **Hardware utilities are standalone**: `UDPClient`, `NetworkInterfaces`, `SerialPort`,
  `WiFiScanner` have no dependency on `Net::Manager` nor on Asio. `TCPClient`/`TCPServer` use
  Asio internally (connect / accept only) behind a blocking-with-timeout API — consumers never
  see Asio types, and since 2026-08-27 **`TCPServer.hpp` no longer includes `asio.hpp`** (the
  io_context and the acceptor live in a private `Impl` defined in the TU).
- **noexcept everywhere**: all hardware utility functions are noexcept, errors return
  false/empty containers (Asio is configured with `ASIO_NO_EXCEPTIONS`). ⚠️ That guarantee is
  honest because every Asio call in the cascade uses the `error_code` overloads — the legacy
  `Base::Network::download()`, which used the throwing ones (abort on a DNS failure), was removed
  from emeraude-base on 2026-08-27.
- ⚠️⚠️ **Socket options that feed `bind()` or `recvmsg()` must be set at `open()` time, never lazily.** Measured on Linux 6.x: arming `IP_PKTINFO` at the first `receive(DatagramInfo)` instead of at `open()` still yields a correct destination address — it sits in the IP header — but the receiving interface index comes back as **0** for any datagram already queued. The structure looks perfectly plausible and the index is silently wrong, which is exactly what breaks per-interface attribution on a multi-homed host. Same family as `SO_REUSEADDR`/`SO_REUSEPORT`, which `bind()` reads once and ignores forever after.
- ⚠️⚠️ **Never stream a `std::filesystem::path` into a trace — on Windows it is a `terminate`, not
  a cosmetic issue.** `path::operator<<` emits `quoted(p.string())`, and `string()` throws on MS-STL
  for content the ANSI code page cannot represent; the whole cascade is built `-fno-exceptions`, so
  the throw becomes an abort. The cache lives under the user's profile, so a Windows account name
  with a non-ANSI character was enough: `Manager.cpp` had **four** such sites, one of them on the
  init path (`isDirectoryUsable` failing), i.e. a crash at startup rather than during a download.
  Fixed 2026-08-28 by wrapping each in `IO::toU8String()`. Reported from Windows, found and fixed on
  Linux — a green Linux run can never surface this class of defect, only a reading of the code can.
  Side effect worth knowing: `operator<<` supplied its own quotes, so those messages used to be
  doubly quoted; they now carry only the ones the format string writes.
- ⚠️⚠️ **A URL is traced through `URI::redacted()`, never streamed whole** (2026-10-07). A presigned URL carries its
  credentials in the query — app_system's crash report PUTs to an S3 URL holding `X-Amz-Security-Token` and
  `X-Amz-Signature` — and a userinfo its password; `Net::APIClient` traced them all ("Calling PUT '<url>'", "answered
  HTTP …"), into the journals the next crash report uploads. Every trace of `APIClient.cpp` and `Manager.cpp`, and the
  `list` console command, now print `scheme://host:port/path?<redacted>`. `operator<<` / `to_string()` stay for what
  goes on the wire and for the download cache key (the `listCache` console command still shows full URLs: a presigned
  GET would expose its query there), and the console echoes a typed command verbatim (`Executing command: …`) — that
  one is the operator's own text. Measured 2026-10-07 (Linux): `get(https://stub.mango3d.net/?probe=secret)` traced
  `Calling GET 'https://stub.mango3d.net/?<redacted>'` and `… answered HTTP 503`, `list()` showed the same.
- ⚠️ **A green compile proves nothing about a socket.** The multicast surface and the
  IPv4/IPv6/MAC enumeration were validated by out-of-tree binaries compiled straight from
  `UDPClient.cpp` / `NetworkInterfaces.cpp` (they depend on nothing but `emeraude_export.hpp`),
  doing a real round-trip, a real DNS-SD exchange, a real enumeration. Reuse that technique
  rather than trusting the build.

### Triad section 4 (2026-09-30)

- **Directory walks never throw**: the `.part` sweep and `clearCache()` (and `SerialPort::listPorts()` on
  `/sys/class/tty`, the unplug case lot 4 aimed at) walk through `Base::IO::forEachDirectoryEntry()` / an explicit
  `increment(error_code)` — a range-for's `operator++` throws even when the iterator was built with an `error_code`.
- **The cache index is data on disk**: an entry whose file name leaves the cache directory (`"../../x"`, absolute) is
  DROPPED at load (`Base::IO::confinedPath()`) — an eviction would otherwise `remove()` a file anywhere. Measured: a
  tampered index with a relative and an absolute name, a 1-byte budget forcing an eviction at startup → both entries
  dropped, the victim file intact, the legitimate entry evicted, the `.part` swept.
- **Windows serial ids**: `std::from_chars` (hex) for the USB VID / PID, never `std::stoul` (it throws); the
  registry string is read with a bounded `strnlen`. WLAN SSID lengths are clamped to their 32-byte array.
- ⚠️ The CEF helper processes start this manager too, on the SAME cache: engine item
  `net-cache-managed-by-cef-helper-processes`.
- I/O methods (`UDPClient::bind` / `set*`, `SerialPort::read` / `write`) stay NON-const on purpose: they mutate the
  socket / port (clang-tidy `make-member-function-const` proposes otherwise; its fix-it also desynchronised the
  Windows definitions it cannot see).

