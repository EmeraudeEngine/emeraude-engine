# net-check — runtime check for `EmEn::Net`, without building the engine

Two standalone programs that compile the **real** `src/Net/` sources out-of-tree. No CMake, no
Vulkan, no engine build: a compiler and this directory are enough. They exist because the Net
utilities deliberately depend on nothing but `emeraude_export.hpp`, and because a compile check
proves nothing about a socket option the kernel silently refuses.

| File | Answers |
|---|---|
| `net_check.cpp` | Does `UDPClient` + `NetworkInterfaces` actually work on this OS? 47 assertions. |
| `shutdown_semantics.cpp` | Does `shutdown()` wake a reader on *this* kernel? Three socket shapes. |

## Build & run

```bash
# Linux / macOS
c++ -std=c++20 -O2 -Wall -Wextra -I../../src \
    net_check.cpp ../../src/Net/UDPClient.cpp ../../src/Net/NetworkInterfaces.cpp -o net_check
./net_check

c++ -std=c++20 -O2 -Wall -Wextra shutdown_semantics.cpp -o shutdown_semantics
./shutdown_semantics
```

```bat
:: Windows, from an MSVC developer prompt.
:: ⚠️ Iphlpapi.lib is REQUIRED — NetworkInterfaces calls GetAdaptersAddresses, and without it the
:: link fails with LNK2019 on __imp_GetAdaptersAddresses. ws2_32 comes from a #pragma in the source.
cl /std:c++20 /EHsc /W4 /I..\..\src net_check.cpp ..\..\src\Net\UDPClient.cpp ..\..\src\Net\NetworkInterfaces.cpp /link Iphlpapi.lib
net_check.exe
```

Run it under sanitizers at least once per platform — the moved-from and close-race cases are
exactly the kind that pass by luck:

```bash
c++ -std=c++20 -O1 -g -fsanitize=address,undefined -I../../src \
    net_check.cpp ../../src/Net/UDPClient.cpp ../../src/Net/NetworkInterfaces.cpp -o net_check_asan
./net_check_asan
```

## Reading the result

`pass / fail / warn` on the last three lines; exit code is non-zero on any failure. A `[WARN]` is
never a failure — it flags an environment-dependent observation (no mDNS device on the LAN, a
kernel that accepts more than it documents).

What each section is really testing:

| § | What a failure would mean |
|---|---|
| 1-2 | `NetworkInterfaces` — the platform leg (`AF_LINK` / `AF_PACKET` / `GetAdaptersAddresses`) is wrong. Watch the MAC: **empty** on loopback, repeated identically across one interface's addresses, never `00:00:…`. |
| 3 | The kernel refuses the option width `MulticastOptionValue` picks for this platform. A silent TTL of 1 — discovery finds only the local link. |
| 4 | `receive(0)` parks instead of polling, or a timeout is not honoured. |
| 5 | ⚠️ `close()` does not return a parked `receive()`. This is the macOS bug of 2026-08-28; `UDP.close()` is a *synchronous* WebModule binding in app_system, so this stalls the renderer's main thread. |
| 5b | A moved-from instance crashes or is unusable. |
| 6 | The mDNS round trip: sharing port 5353 with the system responder, joining on each interface, and — the subtle one — a **non-zero interface index on every datagram**, which only holds because the ancillary option is armed in `open()` and never lazily. |
| 7 | SSDP discovery. Depends on what is on the LAN. |

## What this does NOT cover

- **The packaged application.** Everything here runs from a terminal. On macOS 15+ multicast needs
  the *Local Network* authorisation, and a terminal already granted it proves nothing about a
  double-clicked bundle. Same for the app's own JS path (`--mode=test` → the dev-check mDNS card).
- **`SerialPort`.** It needs a real adapter; a pty is not one. The rate that matters is **250000**
  (Marlin's default) — the one that used to open at 9600 while reporting success on macOS.
- **TLS / the download manager.** Those live in emeraude-base: `EmeraudeBaseUnitTests`, and
  `Core.NetManagerService.*` from the remote console.

The full per-OS run-list is in
[`emeraude-base/docs/todo/tls-stack-windows-macos-validation.md`](../../dependencies/emeraude-base/docs/todo/tls-stack-windows-macos-validation.md).
