## Cross-platform status — read before trusting anything here

**All three platforms have now executed this directory** (2026-08-28). That is new — it had been
Linux-only for its whole life — and it cost four bugs in two days. Read the two subsections below
before touching anything here.

| Leg | Linux | macOS | Windows |
|---|---|---|---|
| `UDPClient` multicast / mDNS round trip | ✅ 2026-08-26 | ✅ 2026-08-28 | ✅ 2026-08-28 (6 LAN hosts answered) |
| `NetworkInterfaces` IPv4+IPv6+MAC | ✅ | ✅ 2026-08-28 (`AF_LINK`) | ✅ 2026-08-28 (`GetAdaptersAddresses`) |
| Trust store + hermetic & live TLS suites | ✅ | ✅ 2026-08-28 | ✅ 2026-08-28 (78/78, 43 CAs from `ROOT`) |
| `Net::Manager` downloader, `ExternalData` chain | ✅ | ❌ not yet | ⚠️ download ✅, `ExternalData` chain ❌ |
| `SerialPort` against real hardware | ✅ | ❌ no adapter available | ❌ no adapter, but see below |

⚠️ Depth is **not** uniform, and the table flattens that. macOS was exercised through an
out-of-tree harness on this directory's sources; Windows through a downstream application's own JS path
(`--mode=test`, dev-check fixtures over CDP), which is the layer macOS has never run. Neither
substitutes for the other.

### What macOS taught us (2026-08-28)

Three bugs, all fixed in the same sitting. The first two are **not macOS quirks** — they were
latent everywhere and only macOS made them visible:

1. ⚠️⚠️ **`shutdown()` does not wake a reader on an unconnected datagram socket.** POSIX makes it
   fail with `ENOTCONN`; **Linux is the lenient outlier** that wakes the reader anyway. Measured:
   `close()` waited out the receive() timeout **in full** (10 s) instead of returning. Since
   a downstream application binds `UDP.close()` as a *synchronous* WebModule method, that stall lands on the
   renderer's main thread. `UDPClient` no longer trusts the kernel for the wake-up — `close()`
   raises a flag and `receive()` polls in 50 ms slices (`PollSliceMs`). **Confirmed on Windows the
   same day, by measurement rather than inference**: the pre-fix symptom was visible there
   (~107 ms `close()`, one drain-loop block), and after the fix `close()` returns in **62 ms with a
   `receive()` parked on a 3000 ms timeout — and the same 62 ms at 1000 ms**, i.e. bounded by the
   poll slice and independent of the receive timeout, which is exactly the contract.
   `TCPClient` is *not* concerned (it shuts down a **connected** socket, which works everywhere),
   and neither is `TCPServer` (Asio `acceptor->cancel()`). Verified on this machine: unconnected
   UDP → not woken; connected UDP → woken in 203 ms; listening TCP → not woken.
2. ⚠️ **A moved-from `UDPClient` crashed on destruction**, on every platform: the move handed away
   `m_handleMutex`, and `close()` — unlike `TCPClient::close()` — dereferenced it with no null
   check. Caught by AddressSanitizer. `close()`/`send()`/`receive()` now guard, and `open()`
   re-arms the guards so a moved-from instance is reusable rather than a silent trap.
3. ⚠️ **`SerialPort.mac.mm` ran at 9600 bauds without telling anyone.** `toBaudConstant()` fell
   back to `B9600` in its `default:` branch and `open()` returned **true** — measured on a pty:
   `open(250000)` → `true`, line at 9600. macOS stops at `B230400` where Linux carries constants
   to `B4000000`, so **250000 (Marlin's default) hit that branch every time**. Now mirrors the
   Linux `BOTHER` design with `ioctl(IOSSIOSPEED)` (`<IOKit/serial/ioss.h>`, applied **after**
   `tcsetattr` or it is undone), and `open()` returns **false** when the adapter refuses the rate.
   The macOS switch also gained `B7200`/`B14400`/`B28800`/`B76800`, which it simply lacked.

### What Windows added (2026-08-28, same day, a downstream application's JS path)

Windows was validated through `--mode=test` and the dev-check fixtures, i.e. the layer **macOS has
never run**. What it contributed to this directory:

- **The `close()` fix is measured, not inferred** — see point 1 above.
- **The deadline-based timeout accounting holds**: 201/200, 1001/1000, 3013/3000 ms
  (+0.4%, +0.1%, +0.4%). None of the slice-counting drift it was written to avoid.
- **`MulticastOptionValue`'s `DWORD` branch works** — TTL 255 and loopback both took.
- ⚠️ **`SerialPort.windows.cpp` needs no baud work at all**, and this closes the question the other
  two platforms opened: it assigns `dcb.BaudRate = config.baudRate` directly — an arbitrary `DWORD`,
  with **no `CBR_*` constant table** to fall out of. The Linux `termios2`/`BOTHER` and macOS
  `IOSSIOSPEED` problem structurally cannot arise there. Still untested against real hardware, but
  there is no lookup to be silently wrong.
- ⚠️ **A dead interface aborts a naive join loop, and Windows always has dead interfaces.**
  `IP_ADD_MEMBERSHIP` fails on an APIPA address (`169.254.x.x`), and a Windows host routinely
  exposes 4+ of them (disconnected Wi-Fi, Bluetooth PAN, Hyper-V/VPN) where a Linux dev box has
  none. `joinMulticastGroup()` correctly returns `false`; it is the **consumer** that must treat
  that as "skip this interface", never "abort discovery". Enumerating on
  `family == IPv4 && !internal` does **not** exclude them — APIPA is not flagged internal.
- ⚠️ **The long-standing Windows trap "`TCPServer` binding `"::"` opens a v6-only socket" was
  never real on this code path — measured 2026-08-28, and the claim is retracted.** The pre-fix
  acceptor already accepted an IPv4 peer on `"::"` there (4/4): Asio clears `IPV6_V6ONLY` on every
  `AF_INET6` socket it creates on Windows, and the option reads 0 before any `bind()`. A raw
  `::socket(AF_INET6)` on Windows *does* default to 1, which is where the trap genuinely lives —
  raw Winsock, not `TCPServer`. The explicit call added that day is kept for stating the intent
  instead of depending on an Asio internal, but it fixes nothing observable. Recorded so nobody
  hunts the symptom again.

> [!NOTE]
> The bug the Windows run actually surfaced first was **not in this directory**: a downstream application's
> `SharedDataManager::createJob<>()` locked a non-recursive mutex twice, which MS-STL turns into a
> `std::system_error` inside a `noexcept` binding — instant renderer death on every
> `JobInterface` module. glibc self-deadlocks instead of throwing, so Linux would have hung rather
> than crashed. Fixed in a downstream application; recorded here only because it is why the Windows network run
> could not start.

### What the Linux replay closed (2026-08-28)

Linux was re-run right after the macOS sitting, because that sitting had changed shared code on a
platform that was green. `tools/net-check` — **48 pass / 0 fail / 1 warn**, same result under
`-fsanitize=address,undefined`:

- ✅ **The `MulticastOptionValue` width change is good**: `int` accepted, TTL reads back 255.
- ✅ **The shared-path changes replay too**, which the commit message did not claim and nobody had
  measured here: `close()` returns a parked `receive()` (300 ms, bounded by the poll slice), the
  deadline-based accounting shows **0 % drift over 1200 ms**, and the moved-from instance is safe
  and reusable. `shutdown_semantics` re-confirms Linux as the lenient outlier — `ENOTCONN` on an
  unconnected UDP socket **and the reader woken anyway**, which is exactly why the bug could hide
  here for the directory's whole life.
- ⚠️ **It also found one defect of its own**, unrelated to the macOS work and present since
  `NetworkInterfaces` existed: `enumerateMulticastCapable()` dropped `lo` on Linux — see the
  loopback trap under [§ Network Interfaces](#network-interfaces-netnetworkinterfaces). Fixed the
  same day, Linux-scoped.

> [!NOTE]
> The lesson is the one the macOS sitting already paid for once, in the other direction: **a fix
> proven on one platform is a change on every other one.** Two of the three items above are shared
> code that no per-platform reasoning would have flagged, and the third was found only because the
> replay ran the whole harness instead of the one assertion the commit pointed at.

Two findings that are **not** bugs but invalidate a written assumption:

- The `IP_MULTICAST_TTL` byte-type story does **not** reproduce on macOS 26 / arm64: this kernel
  accepts the `int` form as readily as the `unsigned char` one. "The TTL silently stayed at 1 and
  devices one hop away went missing" is not an explanation that holds here — do not hunt that ghost.
  Since that leniency is an undocumented implementation detail, the width is now taken from each
  platform's own manual through **`MulticastOptionValue`** (top of `UDPClient.cpp`), applied to
  `IP_MULTICAST_TTL` **and** `IP_MULTICAST_LOOP` in `setMulticastTTL()`, `setMulticastLoopback()`
  and `ssdpDiscover()`: `unsigned char` on macOS/BSD (`ip(4)`), `int` on Linux (`ip(7)`), `DWORD`
  on Windows. This **changed the Linux leg** from `unsigned char` to `int`; ✅ **replayed on Linux
  2026-08-28** — the `int` width is accepted and the TTL reads back as 255 through it. That kernel
  also accepts both widths, exactly like macOS 26, which is why the leniency is treated as an
  implementation detail rather than a contract.
- `SerialPort.mac.mm` never claims the port: Linux does `ioctl(TIOCEXCL)` ("a second process
  opening the same adapter mid-print is a corrupted stream nobody can diagnose"), macOS does not,
  though the ioctl exists there. Left alone deliberately — exclusive open is a behaviour change,
  not a bug fix.

### The remaining handover

Two todo items carry the checklist, and they are meant to be run in one sitting on those machines:

- [`emeraude-base/docs/todo/tls-stack-windows-macos-validation.md`](../../../dependencies/emeraude-base/docs/todo/tls-stack-windows-macos-validation.md)
  — the trust store per platform, the hermetic and live suites, the downloader from the console, the
  `ExternalData` chain, and the traps (MS-STL's throwing `path::string()` under `-fno-exceptions`,
  the MSVC-only `#pragma comment(lib, …)`, the `IOKit` link that arrives through hwloc, the
  `v6_only` default on Windows).
- [`docs/todo/udp-multicast-macos-verification.md`](../../todo/udp-multicast-macos-verification.md)
  — multicast/mDNS, the IPv6+MAC enumeration, the non-blocking receive and the SSDP TTL type, plus
  the macOS 15 *Local Network* permission (test a **signed, packaged** binary, never a console run).
