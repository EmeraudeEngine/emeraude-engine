# clang-tidy ledger — emeraude-engine

> Owner decision 2026-09-30, plan Ave Robustus § 3 (projet-alpha `docs/plans/ave-robustus.md`). The configuration is
> this repository's `.clang-tidy`. Gate on every change: ZERO NEW finding on the touched translation units; a finding
> is fixed, never silenced. A FULL run is a long process ordered by the owner; its results are recorded here.

## How to run

**Version: clang-tidy 21.1.6** (owner decision 2026-09-30). clang-tidy 19 (Debian's) SEGFAULTS in
`modernize-use-designated-initializers` on engine `src/CoreTypes.hpp` (`EngineContext`: a reference member
brace-initialised with a forward-declared type) — 8 of the 12 Console TUs, most of the engine. 21 does not crash.
Without sudo, from PyPI (the official LLVM binaries) in a scratch virtual environment:

```bash
python3 -m venv <scratch>/ctvenv && <scratch>/ctvenv/bin/pip install "clang-tidy==21.1.6"
# One translation unit, with the compile database of a Claude build directory (never cmake-build-*) — cleaned of the
# GCC-only flags (-flto*, -fno-fat-lto-objects, -fuse-linker*) and with clang++ as the driver:
<scratch>/ctvenv/bin/clang-tidy -p <cleaned db dir> --quiet <file.cpp> > tidy.log 2>&1; echo EXIT=$?
```

Redirect, never pipe. Count the findings by check (`[check-name]` at the end of each warning line), keeping only
those whose file is inside the module (the header filter also reports every included header).

## Last full run per module

| Module | Date | Findings by check | Notes |
|---|---|---|---|
| `src/Console` (+ `MCP/`), 12 TUs | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 24, all ON PURPOSE (below) — 21 misc-no-recursion, 1 avoid-c-arrays, 1 cppcoreguidelines-use-enum-class, 1 pro-type-reinterpret-cast. Before: 64 (clang-tidy 19, designated-initializers blind) / 66 (21). | Triad section 1, `docs/todo/triad-engine-pass.md` |
| `src/Resources`, 6 TUs (+ `Container.hpp` templates) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 5, all ON PURPOSE (below) — 2 cppcoreguidelines-use-enum-class, 2 misc-no-recursion, 1 cppcoreguidelines-avoid-do-while. Before: 20. | Triad section 2 |
| `src/Scenes/Loaders`, 4 TUs (+ `SceneDataConsumer.cpp`) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 172, all ON PURPOSE (below) — 141 pro-type-union-access, 18 pro-bounds-constant-array-index, 7 pro-type-reinterpret-cast, 3 avoid-const-or-ref-data-members, 1 Padding, 1 use-enum-class, 1 enum-size. Before: 221. | Triad section 3 |
| `src/Net`, 10 Linux TUs (`*.windows.cpp`, `*.mac.mm` read by hand) | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 39, all ON PURPOSE (below) — 20 reinterpret-cast, 7 make-member-function-const, 5 pro-type-vararg, 2 use-enum-class, 1 each array-to-pointer-decay, macro-usage, avoid-c-arrays, interfaces-global-init, constant-array-index. Before: 103. | Triad section 4 |
| `src/Input`, 6 TUs | 2026-09-30 | clang-tidy 21.1.6 after the triad pass: 40, all ON PURPOSE (below) — 28 pro-bounds-constant-array-index, 8 use-enum-class, 4 misc-confusable-identifiers. Before: 47. | Triad section 5 |

## Findings kept ON PURPOSE

Each with its file:line, its check and the reason (an owner decision).

Known on purpose before this ledger: Saphir — the six warnings left on purpose
(`docs/subsystems/saphir/25-clang-tidy-the-six-warnings-that-are-left-on-purpose.md`).

### `src/Console` (owner decisions 2026-09-30)

- **misc-no-recursion ×21** — (a) the walks of the console tree (`Controller::dumpControllable`,
  `describeControllable`, MCP `appendTools`): the depth is the engine's own controllable hierarchy (~5 levels), never
  an input; (b) the asio async chains (`readRequest` → its completion handler → `readRequest`, `doRead`, `writeNext`,
  `startStream`, …): each call RETURNS before the next one runs, the stack does not grow.
- **avoid-c-arrays ×1** — `ControllableTrait::bindCommand(..., const Parameter (& parameters)[N], ...)`: the C-array
  reference is what deduces N from a brace list at the call site; `std::array` and `std::span` cannot.
- **cppcoreguidelines-use-enum-class ×1** — `Controller::NotificationCode`: the engine-wide Observer convention (24
  unscoped `NotificationCode` enums compared as `int` in `onNotification()`); changing one alone breaks the contract.
- **pro-type-reinterpret-cast ×1** — `RemoteListener.cpp` `setsockopt(..., SO_SNDTIMEO, reinterpret_cast< const char
  * >(&timeout), ...)`: the Windows signature of the system call.

### `src/Resources` (2026-09-30)

- **cppcoreguidelines-use-enum-class ×2** — `ResourceTrait::NotificationCode`, `Container::NotificationCode`: the
  Observer convention (see Console).
- **misc-no-recursion ×2** — `ResourceTrait::dependencyLoaded()` → `checkDependencies()` → a parent's
  `dependencyLoaded()`: the upward notification of a finished load. Its depth is the dependency chain, bounded by the
  resource TYPES (scene → mesh → material → texture → image), not by data. (The data-driven cycle check is iterative
  since 2026-09-30.)
- **cppcoreguidelines-avoid-do-while ×1** — `Manager::unloadUnusedResources()`: "one pass, then again while a pass
  frees something" (freeing a resource frees its dependencies on the next pass) — owner decision 2026-09-30.

### `src/Scenes/Loaders` (2026-09-30)

- **pro-type-union-access ×141** — FBXLoader only: the ufbx API is unions (`ufbx_vec3::x`, …).
- **pro-bounds-constant-array-index ×18** — each one checked bounded: FBX (`texIdx` checked, `k < 3`, `wi < take ≤ 4`,
  ufbx indices consistent by construction), glTF (texture index checked, literal loops), USD (literal 4×4 loops), WAD
  (counts derived from the lump size; cross-references checked).
- **pro-type-reinterpret-cast ×7** — binary buffers (a WAD name, the file read, meshopt / Draco input — its offset is
  bounded upstream —, the USDZ asset map, Tydra's raw normal / UV buffers — sized by the vertex count check).
- **avoid-const-or-ref-data-members ×3** — each loader holds the `Resources::Manager &` it was built with: a
  short-lived, non-copyable object by design.
- **clang-analyzer-optin.performance.Padding ×1** — a lambda CLOSURE's layout (GLTFLoader).
- **use-enum-class + enum-size** — `LoaderCapabilityBits`: a bit-flag set combined with `|`, `uint32_t` on purpose.

### `src/Net` (2026-09-30)

- **pro-type-reinterpret-cast ×20** — the BSD socket API (`sockaddr_in` ↔ `sockaddr`, option buffers).
- **make-member-function-const ×7** — `UDPClient::bind` / `setBroadcast` / `setMulticast*`, `SerialPort::read` /
  `write`: they mutate the socket / port (logical mutation). Their fix-it also broke the Windows definitions.
- **pro-type-vararg ×5** — `ioctl()` / `fcntl()`, the POSIX system calls.
- **use-enum-class ×2** — `NotificationCode` (the Observer convention).
- **macro-usage, avoid-c-arrays** — `SerialPort.linux.cpp`'s private mirror of the kernel `termios2` (`BOTHER`, the
  `c_cc` array): it must match the kernel ABI byte for byte.
- **array-to-pointer-decay ×1** — a `char[]` buffer handed to a C API (`NetworkInterfaces.cpp`).
- **interfaces-global-init ×1** — `TCPServer::DefaultBacklog{asio::socket_base::max_listen_connections}`: that is
  `SOMAXCONN`, a constant expression (a false positive).
- **pro-bounds-constant-array-index ×1** — `UDPClient.cpp` NUL after `recvfrom(…, size - 1, …)`: bounded.

### `src/Input` (2026-09-30)

- **pro-bounds-constant-array-index ×28** — every device-state subscript now follows an explicit range check (the
  public-API rule of `docs/subsystems/input/01-…`).
- **use-enum-class ×8** — `Key`, `ModKey`, `MouseButton`, `Joystick*`, `Gamepad*`: they carry the GLFW integer codes
  and are compared / combined as such (a scoped enum would need a cast at every GLFW call).
- **misc-confusable-identifiers ×4** — `KeyI` / `Key1`, `KeyO` / `Key0` (and their `…String`): GLFW's own key names.

