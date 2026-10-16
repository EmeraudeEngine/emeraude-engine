## 6. Architecture

| File | Role |
|------|------|
| `Controller.hpp/cpp` | Service. Manages registered objects, dispatches commands, hosts RemoteListener |
| `ControllableTrait.hpp/cpp` | Interface. Any service inheriting this can register commands |
| `Command.hpp` | Stores a `Binding` (callback) + help string |
| `Expression.hpp/cpp` | Parses `object.command(args)` syntax |
| `Argument.hpp/cpp` | Typed argument extraction (Boolean, Integer, Float, String) |
| `Output.hpp` | Command response with severity level |
| `RemoteListener.hpp/cpp` | TCP server (ASIO). Accepts connections, queues commands, sends direct responses |

### Command flow

```
TCP client → RemoteListener (network thread, queues command + client socket)
                  |
Controller::poll() (main thread, dequeues)
                  |
Controller::executeCommand(string, outputs)
                  |
         +--------+--------+
         |                  |
   Built-in command?   Object command?
   (no dot in string)  (dot notation)
         |                  |
   executeBuiltInCommand  Expression parser
                             |
                        ControllableTrait::execute() [recursive through hierarchy]
                             |
                        Binding callback
                             |
                        (succeeded, Outputs) → RemoteProtocol::serializeResponse() → respond() to that client (ALWAYS, one line)
```

### Identity: a controllable is never copied nor moved (2026-10-16, Ave Robustus II P4)

`ControllableTrait` deletes its copy and move: the console tree and the parent hold the object's ADDRESS, and the bound
commands capture the derived object's `this`. Before, the defaulted copy / move gave a moved-to object the parent link
while the tree still pointed at the original, and the copied commands still ran on the original. Its destructor
removes it from the console and the parent and orphans its children (unchanged). No class of the cascade copied one
(the whole build, engine and projet-alpha, compiled with the deletion).

No global console instance (2026-10-16, owner decision): `Controller::instance()` and its static are gone. An object
registers at the root with `registerToConsole(Controller &)` (`Core` with its `m_consoleController`, an application
through `consoleController()`); the console's `add()` records itself in the object (`m_console`), and both sides
detach at their end — the object's destructor removes it from the console, the console's destructor (and `remove()`)
clears the object's link. Children register under a parent (`registerToObject()`), as before. The tree revision
(`markConsoleTreeChanged()`, read by the MCP server for `tools/list_changed`) stays a static atomic counter.

### Thread safety

- `RemoteListener` uses `std::mutex` for the command queue and client list
- `Controller::poll()` is called on the **main thread** only
- All command execution happens on the **main thread** — safe to access engine state
- Responses are sent to the requesting client only, one JSON line each, under `m_writeMutex`
