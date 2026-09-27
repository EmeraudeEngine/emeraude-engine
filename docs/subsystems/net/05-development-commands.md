## Development Commands

> [!WARNING]
> **There is no test target in the engine.** The engine `CMakeLists.txt` declares no
> `add_test()`; the only unit suite in the cascade lives in **emeraude-base**
> (`EmeraudeBaseUnitTests`).
>
> To exercise a `Net` utility, compile it out-of-tree: the hardware utilities depend on
> nothing but `emeraude_export.hpp`, so a standalone binary works and gives a real runtime
> check rather than a compile check.
>
> ```bash
> g++ -std=c++20 -I<engine>/src my_check.cpp \
>     <engine>/src/Net/UDPClient.cpp <engine>/src/Net/NetworkInterfaces.cpp -o my_check
> ```
>
> **Do not write that harness from scratch — one is in the tree**:
> [`tools/net-check/`](../../../tools/net-check/README.md) builds on Linux, macOS and Windows and runs
> 48 assertions over `UDPClient` + `NetworkInterfaces` (interfaces and MACs, multicast option width,
> non-blocking receive, `close()` waking a parked reader, moved-from safety, a real mDNS round trip,
> SSDP). `shutdown_semantics.cpp` next to it answers "does `shutdown()` wake a reader on this
> kernel?" for the three socket shapes — that is the probe that found the 2026-08-28 bug.
