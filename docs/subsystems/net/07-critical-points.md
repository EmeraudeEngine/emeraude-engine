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
- ⚠️ **A green compile proves nothing about a socket.** The multicast surface and the
  IPv4/IPv6/MAC enumeration were validated by out-of-tree binaries compiled straight from
  `UDPClient.cpp` / `NetworkInterfaces.cpp` (they depend on nothing but `emeraude_export.hpp`),
  doing a real round-trip, a real DNS-SD exchange, a real enumeration. Reuse that technique
  rather than trusting the build.
