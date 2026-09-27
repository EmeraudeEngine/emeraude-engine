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

### Thread safety

- `RemoteListener` uses `std::mutex` for the command queue and client list
- `Controller::poll()` is called on the **main thread** only
- All command execution happens on the **main thread** — safe to access engine state
- Responses are sent to the requesting client only, one JSON line each, under `m_writeMutex`
