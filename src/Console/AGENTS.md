# Console System - AI Context

## 1. Overview

The Console system provides **runtime command execution** for the engine. It is the **primary channel for AI-driven control** of a running application. An AI agent can create scenes, manipulate cameras, control audio, modify settings, and visually verify results — all through TCP commands.

### Remote Console (TCP)

A TCP server listens on a configurable port (default **7777**, setting: `Core/Console/RemoteListenerPort`).

> [!WARNING]
> **Closed by default.** `Controller::onInitialize()` creates the `RemoteListener` only when
> **`Core/Console/EnableRemoteListener`** is `true` (default **`false`**, 2026-08-27). It then binds
> to **`Core/Console/RemoteListenerAddress`** (default **`127.0.0.1`** — loopback only; `0.0.0.0`,
> a NIC address or `::` to accept other machines; an unparsable value falls back to loopback, never
> to any-address). All three keys are written to `settings.json` on first run even when the console
> stays off, so an operator can find them. **Live activation: Shift+F10** (engine-level shortcut,
> replaced the old "suspend 3 s") — closed: a native `TextInput` dialog pre-filled with the configured
> port accepts `7777`, `0.0.0.0:7777` or `[::1]:7777`, then `Controller::startRemoteListener()` runs
> for **this session only** (settings untouched); open: a Yes/No dialog stops it. From the console
> itself: `Core.remoteConsoleStatus()` and `Core.restartRemoteConsole(port | address:port)` — the
> move is applied at the start of the next `poll()` so the response reaches the client first; that
> connection then closes and you reconnect on the new endpoint. When disabled the log says
> `Remote console disabled (Core/Console/EnableRemoteListener = false)` — an AI seeing
> `Connection refused` must **enable the key and relaunch**, not retry. Why: the channel has **no
> authentication** and reaches `Core.quit()`, settings, scene loading and screenshots; applications
> built on the engine ship to end users.

Once enabled, any AI agent or external tool on an allowed host can connect and:

1. **Send commands** — one per line, newline-terminated (`\n`)
2. **Receive clean responses** — command outputs are sent directly to the requesting client (no Tracer noise)

**Connection — Cross-platform (Python, recommended):**

Use `tools/remote-console.py` — works on Windows, Linux, and macOS:
```bash
# Send a single command
python tools/remote-console.py "Core.SettingsService.getJson()"

# Interactive mode (REPL)
python tools/remote-console.py
```

**Connection — Linux/macOS only (nc):**
```bash
# Send a command and get the response (nc -q 1 for quick disconnect)
echo "Core.SettingsService.getJson()" | nc -q 2 localhost 7777

# Multiple commands in one session
(
echo "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar)"
sleep 1
echo "Core.SceneManagerService.setGround(default)"
sleep 1
echo "Core.RendererService.screenshot()"
sleep 2
) | nc localhost 7777
```

**Note:** `nc` (netcat) is not available on Windows. Always use `tools/remote-console.py` for cross-platform compatibility.

**Response format:** Clean text or JSON — no `[Info][...]` prefixes, no ANSI codes. Each command response is terminated by `\n`. The Tracer is NOT broadcast to TCP clients.

**Lifecycle:** The listener starts in `Controller::onInitialize()` and stops in `Controller::onTerminate()`. It runs on a dedicated network thread. Commands are queued and executed on the **main thread**.

## 2. Service Hierarchy

All services are registered under `Core` as a single entry point:

```
Core
├── ArgumentsService          — Launch arguments (getJson, get, print)
├── AudioManagerService       — Audio system
│   └── TrackMixerService     — Music playback (play, pause, stop, volume, playlist, etc.)
├── FileSystemService         — File paths (getJson, get, print)
├── RendererService           — Graphics (screenshot, getStatus)
├── ResourcesManagerService   — Resource discovery (listContainers, listResources)
├── SceneManagerService       — Scene creation and manipulation (see §5)
│   └── PostProcess           — The ACTIVE scene's post-process chain (listEffects, getStatus,
│                               select, disable, setLightingMode). ⚠️ Registered by
│                               Scenes::Manager on scene activation and unregistered on
│                               deactivation: the node exists only while a scene that declared
│                               a stack is active.
├── SettingsService           — Configuration (getJson, set, save, print)
└── WindowService             — Window control (resize, getState)
```

### Discovering commands

```bash
# List top-level objects
echo "listObjects" | nc -q 1 localhost 7777

# List sub-objects of Core
echo "Core.lsobj()" | nc -q 1 localhost 7777

# List commands on a service
echo "Core.RendererService.lsfunc()" | nc -q 1 localhost 7777
```

## 3. Built-in Commands

### Top-level (no dot in the command)

| Command | Effect |
|---------|--------|
| `help`, `help()`, `lsfunc()` | **Recursive dump** of every registered object with each command's description — the single entry point for discovering the whole API |
| `listObjects`, `lsobj()` | List top-level controllable object names only (no recursion) |
| `exit`, `quit`, `shutdown` | Graceful shutdown (saves settings) |
| `hardExit` | Immediate shutdown (no save) |
| `listUntypedCommands()` | Counts typed and untyped commands, lists the untyped (legacy) ones still to migrate |

### Per-object (available at any depth in the tree)

| Command | Effect |
|---------|--------|
| `<path>.help()` | Recursive dump of that object's sub-tree (commands + nested objects) |
| `<path>.lsfunc()` | List commands bound at that level only, in `name() - description` form |
| `<path>.lsobj()` | List sub-objects at that level only |

Example: `Core.RendererService.help()` shows every command reachable under `RendererService`, while `Core.RendererService.lsfunc()` shows only commands directly bound there.

### Rule: registration lifecycle (parent tracking)

`ControllableTrait::registerToObject(parent)` stores a **back-pointer to the parent**, and
the destructor removes the object from the parent's sub-object map in both directions
(2026-08-05). Before that, a destroyed sub-object left a **dangling raw pointer** in the
parent's map: the identifier stayed "taken" forever (`Sub object named 'X' already exists`)
and any command dispatched to it executed on freed memory (live segfault on
`SceneManagerService.Act.*` after an act switch in projet-alpha).

- `unregisterFromParent()` (public) detaches a still-alive object — use it when an object
  is *deactivated* but kept (e.g. a game act cycling active/inactive), so the replacement
  can register under the same identifier.
- `registerToObject()` is **idempotent** for the same parent, and re-registering under a
  new parent detaches from the previous one first.
- `onRegisterToConsole()` (command binding) runs **once per object lifetime**, not per
  registration — re-registering never re-binds (no `Command already exists` spam).

### Rule: every new command is TYPED (declared signature, 2026-09-27)

A command is registered with a **typed** `bindCommand()`: its parameters are **deduced from the
lambda's C++ signature**, only their names, descriptions and defaults are written by hand, and the
lambda returns a `Console::CommandResult`. The declaration can no longer drift from the code that
runs, the help line and its "Usage: …" are generated, and the same signature is what the future
MCP server turns into a tool schema (`docs/todo/native-mcp-server.md`). Full pattern in § 7.

- **Parameter types**: `bool`, `int32_t`, `float`, `std::string`, `Console::Argument` (any scalar —
  `SettingsService.set(key, value)`), by value or const reference; `std::optional< T >` for an
  omittable argument; a trailing `std::vector< T >` for zero or more (`openFiles`). Anything else,
  a `Parameter` count that differs from the arity, a non-`CommandResult` return or a vector that is
  not last **does not compile** (static_assert with a readable message).
- **Validation happens before the lambda runs**: a missing argument, one too many, or a value that
  does not convert answers an error naming the parameter (`Argument 'width' expects an integer, got
  'abc'.`) and the lambda never sees it. Conversions: an integer parameter accepts `5` and `5.0`,
  refuses `5.5`; a float accepts an integer; a boolean accepts `true`/`false`/`1`/`0` and **refuses
  any other integer** (the legacy `asBoolean()` read `2` as true); a string parameter receives the
  token **as typed** (`01` stays `"01"` — `Argument::source()`).
- **Defaults**: `Parameter{"frameCount", "…", 5}` makes a plain `T` omittable and shows `= 5` in the
  usage. A `double` literal is refused on purpose (write `1.0F`). A required parameter after an
  omittable one, a default on an `std::optional`, or a default of the wrong type is refused **at
  registration** (traced error, command not bound).
- **Description**: what the command does, capital, final period, no "Usage:" (generated). Each
  `Parameter` description states the meaning **and the unit** ("in metres", "in EV").
- **Hints** (`Console::CommandHint::ReadOnly | Destructive | Idempotent`): how a machine client may
  treat the command. Shown in the `help` dump.
- **Results**: `CommandResult::success/info/warning/error/json/binary`, `.add(Output)` to append,
  `fromOutputs(outputs, succeeded)` for a multi-line report with mixed severities. Only `error()`
  (or `fromOutputs(…, false)`) is a failure. `json()` marks the message as a JSON document — keep it
  valid JSON (escape names), a machine client will parse it.
- **Untyped (legacy) commands** — `bindCommand(name, binding, help)` with a raw
  `(Arguments, Outputs) -> bool` binding — still compile, but **none is left in the engine nor in
  projet-alpha** (all 137 migrated on 2026-09-27; `listUntypedCommands()` answers `137 typed, 0
  untyped`). Never add one. Whether the overload itself is deleted is an open owner decision
  (`docs/todo/console-command-contract.md`): another application built on the engine may still use it.
- **JSON answers**: build them with `Json::Value` + `Base::FastJSON::stringify()` (escapes every
  string; ⚠️ it writes floats with 5 significant digits — for coordinates keep a stream and escape
  only the strings with `Json::valueToQuotedString()`, as `getNode()` does).

## 4. Common AI Operations

### Query engine state (JSON)
```bash
echo "Core.ArgumentsService.getJson()" | nc -q 2 localhost 7777
echo "Core.FileSystemService.getJson()" | nc -q 2 localhost 7777
echo "Core.SettingsService.getJson()" | nc -q 2 localhost 7777
echo "Core.WindowService.getState()" | nc -q 2 localhost 7777
echo "Core.RendererService.getStatus()" | nc -q 2 localhost 7777
```

### Modify settings
```bash
echo "Core.SettingsService.set(Core/Video/Window/Width, 1920)" | nc -q 1 localhost 7777
echo "Core.SettingsService.save()" | nc -q 1 localhost 7777
```

### Window control
```bash
echo "Core.WindowService.resize(1920, 1080)" | nc -q 1 localhost 7777
```

### Music playback
```bash
echo "Core.AudioManagerService.TrackMixerService.play()" | nc -q 1 localhost 7777
echo "Core.AudioManagerService.TrackMixerService.pause()" | nc -q 1 localhost 7777
echo "Core.AudioManagerService.TrackMixerService.volume(50)" | nc -q 1 localhost 7777
echo "Core.AudioManagerService.TrackMixerService.playlist()" | nc -q 1 localhost 7777
echo "Core.AudioManagerService.TrackMixerService.playlist(play, 3)" | nc -q 1 localhost 7777
echo "Core.AudioManagerService.TrackMixerService.status()" | nc -q 1 localhost 7777
```

### Resource discovery
```bash
# List all resource containers (skyboxes, meshes, materials, etc.)
echo "Core.ResourcesManagerService.listContainers()" | nc -q 2 localhost 7777
# Returns JSON: [{"id":"SkyBoxResource","name":"...","loaded":3,"available":5}, ...]

# List available resources in a specific container
echo "Core.ResourcesManagerService.listResources(SkyBoxResource)" | nc -q 2 localhost 7777
# Returns JSON: ["Miramar","DNCity","CloudyDay", ...]
```

### Screenshot and visual verification
```bash
# Take screenshot — returns the file path
echo "Core.RendererService.screenshot()" | nc -q 2 localhost 7777
# Screenshot saved: "/home/user/.local/share/LNIsle/projet-alpha/captures/<timestamp>.png"
```

## 5. AI Scene Creation (Critical)

The AI can create complete 3D scenes autonomously via the console. This is the foundation for AI-driven 3D content creation.

### Complete scene creation sequence

```bash
# Step 1: Create the scene with skybox and camera
# Camera is placed at Y=-2 (below ground level Y=0) to verify ground visibility
echo "Core.SceneManagerService.createScene(IAScene, 1024.0, Observer, 0.0, -2.0, 0.0, Miramar)" | nc -q 3 localhost 7777

# Step 2: Add ground AFTER scene creation
echo "Core.SceneManagerService.setGround(default)" | nc -q 2 localhost 7777

# Step 3: Take screenshot to verify the scene
echo "Core.RendererService.screenshot()" | nc -q 2 localhost 7777
# → Read the PNG to visually confirm: ground (grey) should be visible above the camera
```

### createScene parameters

```
createScene(name, boundary, cameraNodeName, camX, camY, camZ [, backgroundName [, groundMaterial]])
```

| Parameter | Description |
|-----------|-------------|
| `name` | Scene name (e.g., "IAScene") |
| `boundary` | Half-size of the cubic scene volume (e.g., 1024.0) |
| `cameraNodeName` | Name of the camera node (e.g., "Observer") |
| `camX, camY, camZ` | Camera position. **Y=-2 to see ground from below** |
| `backgroundName` | Optional. SkyBox resource (e.g., "Miramar", "DNCity") |
| `groundMaterial` | Optional. "default" for basic grey, or a material name — resolved from the Standard container first, then PBR (material merge Lot 2, transitional until the legacy material is removed) |

### Critical rules for scene creation

1. **Ground MUST be added via `setGround()` after `createScene()`** — inline ground in createScene may not render
2. **Camera Y=-2 is the verification position** — below ground, you see the grey ground above you
3. **Camera Y=0 won't see the ground** — same level as ground plane
4. **Camera is created BEFORE `enableScene()`** — this prevents the engine from creating a default camera that overrides yours
5. **Lighting is automatic** — `createScene` adds static directional lighting
6. **Always verify with screenshot** — take a screenshot and read the PNG to confirm visual output

### Node manipulation

```bash
# Create a node at a position
echo "Core.SceneManagerService.createNode(MyObject, 5.0, 0.0, 5.0)" | nc -q 1 localhost 7777

# Move a node
echo "Core.SceneManagerService.setNodePosition(Observer, 0.0, 10.0, 20.0)" | nc -q 1 localhost 7777

# Orient a node to look at a point (convention under investigation)
echo "Core.SceneManagerService.setNodeLookAt(Observer, 50.0, 0.0, 50.0)" | nc -q 1 localhost 7777

# Inspect a node
echo "Core.SceneManagerService.getNode(Observer)" | nc -q 1 localhost 7777

# Destroy a node
echo "Core.SceneManagerService.destroyNode(MyObject)" | nc -q 1 localhost 7777

# Attach components to nodes
echo "Core.SceneManagerService.attachCamera(MyNode, MyCamera)" | nc -q 1 localhost 7777
echo "Core.SceneManagerService.attachMicrophone(MyNode, MyMic)" | nc -q 1 localhost 7777
```

### Scene inspection

```bash
echo "Core.SceneManagerService.getSceneInfo()" | nc -q 1 localhost 7777
echo "Core.SceneManagerService.listScenes()" | nc -q 1 localhost 7777
echo "Core.SceneManagerService.listNodes()" | nc -q 1 localhost 7777  # requires targetActiveScene() first
```

### Visual verification workflow

1. Create scene with camera at Y=-2
2. Add ground with `setGround()`
3. Screenshot → read PNG → confirm grey ground is visible above camera
4. Adjust camera position/orientation as needed
5. Screenshot again to verify each change

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
                        Outputs → respond() directly to requesting TCP client
```

### Thread safety

- `RemoteListener` uses `std::mutex` for the command queue and client list
- `Controller::poll()` is called on the **main thread** only
- All command execution happens on the **main thread** — safe to access engine state
- Responses are sent directly to the requesting client via `respond()`, not broadcast

## 7. Adding New Commands

Commands are registered in `onRegisterToConsole()` overrides with a **typed** `bindCommand()`
(rules: § 3 "every new command is TYPED"; machinery: `TypedBinding.hpp`, `Parameter.hpp`,
`CommandSignature.hpp`, `CommandResult.hpp`):

```cpp
// In YourService.console.cpp
void
YourService::onRegisterToConsole () noexcept
{
	// With parameters: one Parameter per lambda argument, in order.
	this->bindCommand("setLens", "Sets the lens of the active camera.",
		{
			{"focal", "Focal length, in millimetres."},
			{"sensorWidth", "Sensor width, in millimetres; unchanged when omitted."}
		},
		[this] (float focal, std::optional< float > sensorWidth) {
			if ( !this->hasCamera() )
			{
				return Console::CommandResult::error("No active camera !");
			}

			// ...
			return Console::CommandResult::success("Lens set.");
		}, Console::CommandHint::Idempotent);

	// Without parameters: no list at all.
	this->bindCommand("getState", "Returns the state as JSON.", [this] () {
		return Console::CommandResult::json(this->stateAsJson());
	}, Console::CommandHint::ReadOnly);
}
```

**Rules:**
- Only **services** inherit `ControllableTrait` (not runtime objects like scenes or entities)
- Services register via `registerToObject(parentService)` to build the hierarchy
- Commands execute on the **main thread** — safe to access engine state
- Convention: separate `*.console.cpp` file (e.g., `Settings.console.cpp`)
- Layout: the lambda body is indented one level deeper than the `[this] (…) {` line, and the
  closing `}, hints);` sits at the lambda's level (see `Window.console.cpp`, `Renderer.console.cpp`)
- A lambda capturing a helper returns errors as `CommandResult::error(reason)` — never write into
  a shared `Outputs` (see `src/Act.cpp` `activeCamera`)
- An argument that is out of range is an **error**, not silently clamped (`temporalCapture(0)`)

## 8. CEF Integration (JavaScript)

All console commands are also accessible from JavaScript in CEF pages via:

```javascript
window.engine.execute("Core.SettingsService.getJson()");
```

Pages implement `onEngineResponse(command, outputs)` to receive results:

```javascript
function onEngineResponse(command, outputs) {
    if (command.indexOf("getJson") !== -1 && outputs.length > 0) {
        const data = JSON.parse(outputs[0].message);
        // Use data...
    }
}
```

## 9. JSON Scene Input

TCP lines starting with `{` are routed to a registered JSON handler (not the normal command parser). This enables complete scene creation from a single JSON document.

- **Registration:** `Controller::setJsonHandler()` sets a `std::function< bool (const std::string &, Outputs &) >` callback
- **Setup:** `Core` registers the handler in `initializeSecondaryLevel()`, routing to `SceneManager::loadSceneFromJson()`
- **Flow:** TCP input -> `poll()` detects `{` prefix -> `m_jsonHandler(json, outputs)` -> scene built and enabled
- **Format:** See [`docs/ai-runtime-control.md`](../../docs/ai-runtime-control.md) section 4 for the full JSON scene specification

## Critical Points

- **Do not confuse** with AVConsole (`src/Scenes/AVConsole/`) which is the Audio/Video virtual device system
- **Closed by default** — `Core/Console/EnableRemoteListener` (default `false`) gates the whole listener; **bind address** `Core/Console/RemoteListenerAddress` (default `127.0.0.1`); **port** `Core/Console/RemoteListenerPort` (default 7777)
- **Live control** — `Controller::startRemoteListener(address, port)` / `stopRemoteListener()` / `isRemoteListenerRunning()` / `remoteListenerEndpoint()`; `requestRemoteListenerRestart()` defers to the next `poll()` (never replace the listener while draining its own queue: the response socket dies). `parseEndpoint()` is the single parser for the dialog and the command. Shift+F10 in `Core::toggleRemoteConsoleFromKeyboard()`; the loop is paused around the modal native dialogs like `displayCoreMessages()`
- **No authentication** — which is exactly why the two defaults above are off and loopback. Anyone who can reach the socket owns the application
- **Bounded everywhere (2026-08-27 hardening)** — `MaxPendingCommands = 256` (overflow answers the client `ERROR: command queue full`, it is not left waiting), `MaxLineLength = 8192` (the read buffer is capped: an unbounded `streambuf` let a peer that never sends a newline grow the process until the OOM killer fired, on an unauthenticated port; a longer line gets `ERROR: line too long` and is disconnected), `MaxClients = 8` (further connections get `ERROR: too many clients`), `SendTimeoutMilliseconds = 2000` (`SO_SNDTIMEO` on every accepted socket)
- **Shutdown order is load-bearing** — the acceptor and every client socket are closed **before** `io_context::stop()` and the thread join. `stop()` does not interrupt a handler already running, and a write to a peer that stopped reading only ends when its socket dies: closing after the join made the whole process hang on exit, forever, because of one frozen client. Verified: exit in ~1.6 s with a client that never reads
- **No blocking write can hang a thread** — the welcome banner runs on the io thread and `respond()` runs on the **main** thread; the send timeout bounds both. (The banner also used to send a stray NUL: `asio::buffer` on a `char[]` includes the terminator — pass a `std::string_view`.)
- **`accept()` never re-arms blindly** — the error is logged, and a non-transient one (`EMFILE`, out of memory) stops the loop instead of spinning a core at 100 %
- **Clean responses** — TCP responses contain only command output, no Tracer logs
- **JSON routing** — lines starting with `{` bypass the command parser and go to the JSON handler
- **Quoted arguments are literal (2026-09-27)** — inside `"…"` or `'…'` a comma, a parenthesis or a
  dot no longer splits the argument (`openFiles("/tmp/a,b(1)/x.glb")` is ONE path). Before that,
  `Expression` cut on every `,` and `)` regardless of quotes
- **Typed vs untyped** — `listUntypedCommands()` (top-level built-in) reports the migration state;
  on 2026-09-27: 137 typed, 0 untyped
- **AI Runtime Control** — See [`docs/ai-runtime-control.md`](../../docs/ai-runtime-control.md) for the complete AI operator reference
