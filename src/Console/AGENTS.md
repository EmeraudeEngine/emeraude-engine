# Console System - AI Context

## 1. Overview

The Console system provides **runtime command execution** for the engine. It is the primary channel for **AI-driven control** of a running application.

### Remote Console (TCP)

A TCP server listens on **port 7777** (when `ASIO_ENABLED`). Any AI agent or external tool can connect and:

1. **Send commands** -- one per line, newline-terminated (`\n`)
2. **Receive logs** -- all Tracer output is broadcast to connected clients in real-time

**Connection example:**
```bash
# Connect interactively
nc localhost 7777

# Send a single command
echo "quit" | nc -w 2 localhost 7777

# From code: open a TCP socket to localhost:7777, write "command\n", read responses
```

**Lifecycle:** The listener starts in `Controller::onInitialize()` and stops in `Controller::onTerminate()`. It runs on a dedicated network thread.

## 2. Command Syntax

### Built-in commands (no dot)

| Command | Effect |
|---------|--------|
| `help`, `lsfunc()` | List available commands |
| `listObjects`, `lsobj()` | List registered controllable objects |
| `printArguments` | Print launch arguments |
| `printFileSystem` | Print filesystem paths |
| `printCoreSettings` | Print engine settings |
| `exit`, `quit`, `shutdown` | Graceful shutdown |
| `hardExit` | Immediate shutdown |

### Object commands (dot notation)

```
objectName.commandName(arg1, arg2, ...)
```

Objects register to the console via `ControllableTrait`. Each object exposes named commands with arguments.

**Examples:**
```
Core.exit()
SceneManager.listScenes()
SceneManager.targetScene(myScene)
SceneManager.listNodes()
SceneManager.moveNodeTo(nodeName, 1.0, 2.0, 3.0)
```

## 3. Architecture

| File | Role |
|------|------|
| `Controller.hpp/cpp` | Service. Manages registered objects, dispatches commands, hosts RemoteListener |
| `ControllableTrait.hpp/cpp` | Interface. Any class inheriting this can register commands |
| `Command.hpp` | Stores a `Binding` (callback) + help string |
| `Expression.hpp` | Parses `object.command(args)` syntax |
| `Argument.hpp` | Typed argument extraction from expressions |
| `Output.hpp` | Command response with severity level |
| `RemoteListener.hpp/cpp` | TCP server (ASIO). Accepts connections, queues commands, broadcasts responses |

### Command flow

```
TCP client --> RemoteListener (network thread, queues command)
                    |
Controller::poll() (main thread, dequeues)
                    |
Controller::executeCommand(string)
                    |
        +-----------+-----------+
        |                       |
  Built-in command?       Object command?
  (no dot in string)      (dot notation)
        |                       |
  executeBuiltInCommand()  Expression parser
                                |
                           ControllableTrait::execute()
                                |
                           Binding callback
                                |
                           Outputs --> Tracer --> broadcast to TCP clients
```

### Thread safety

- `RemoteListener` uses `std::mutex` for the command queue and client list
- `Controller::poll()` is called on the **main thread** only (in `Core::mainLoop`)
- All command execution happens on the **main thread** -- no concurrent access to engine state

## 4. Adding New Commands

### For AI agents: how to request a new command

When you identify a need for a new console command during your work, **formulate a request** to the project owner with:

1. **Command name** -- what you want to type (e.g. `screenshot`, `getFrameStats`)
2. **Which object** -- where it should live (e.g. `Renderer`, `SceneManager`, `Core`)
3. **Arguments** -- what parameters it needs
4. **Return/output** -- what information it should produce
5. **Why** -- the concrete use case that motivates it

**Example request:**
> I need a `Renderer.getStatus()` command so I can query frame rate and resolution
> from a remote TCP session to monitor rendering performance.

The project owner validates the design, the AI implements it.

### Implementation pattern

Commands are registered in `onRegisterToConsole()` overrides using `bindCommand()`:

```cpp
// In YourService.console.cpp
void
YourService::onRegisterToConsole () noexcept
{
    // Single command name
    this->bindCommand("doSomething", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
        // arguments[0], arguments[1], etc. for parameters
        // outputs.emplace_back(Severity::Info, "result message");
        return true; // success
    }, "Description of what this command does.");

    // Multiple aliases
    this->bindCommand("exit,quit,shutdown", [this] (...) {
        ...
    }, "Quit the application.");
}
```

**Rules:**
- The class must inherit `ControllableTrait`
- Call `registerToConsole()` during initialization to make the object visible
- Commands execute on the **main thread** -- safe to access engine state
- Return `true` for success, `false` for failure
- Write results to `outputs`, they will be logged and broadcast to TCP clients
- Convention: separate `*.console.cpp` file for console registration (e.g. `Core.console.cpp`)

## 5. Making the Engine More AI-Friendly

The remote console is the foundation for autonomous AI workflows. To maximize its value:

**Commands that AIs need most:**
- **Inspection** -- query engine state without side effects (scene graph, render stats, entity properties)
- **Action** -- trigger operations (screenshot, load scene, toggle debug views)
- **Configuration** -- change settings at runtime (resolution, post-process toggles, camera position)

**Design principles for AI commands:**
- Prefer **structured output** (parseable) over human-readable prose
- Commands should be **idempotent** when possible
- Use **consistent naming**: `get*` for queries, `set*` for mutations, verbs for actions
- Always return **confirmation or error** -- silent success is hostile to automation
- Document the command in the help string -- AIs read those too

## Critical Points

- **Do not confuse** with AVConsole (`src/Scenes/AVConsole/`) which is the Audio/Video virtual device system
- **Port 7777** is hardcoded -- will need to be configurable via settings
- **No authentication** -- the remote console is currently open. Input validation and command sanitization are planned
- **Broadcast loop risk** -- the Tracer sink broadcasts to TCP clients; a command that produces heavy logging will flood connected clients