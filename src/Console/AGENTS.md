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
2. **Receive exactly one response per command** — ONE JSON object on ONE line, in request order,
   even for a failure or a command with no output (no Tracer noise)

**Wire format (owner decision 2026-09-27, `RemoteProtocol.hpp`):**
`{"ok":bool,"outputs":[{"severity":"Info","kind":"text|json|binary","message":"…"}]}` — a `binary`
output adds `mimeType` and a Base64 `data`; the welcome banner is a response with a top-level
`"protocol":1`. Full description: [`docs/ai-runtime-control.md`](../../docs/ai-runtime-control.md) § Wire format.

- **Only `RemoteProtocol` writes to a client** (`serializeResponse`, `serializeError`,
  `serializeWelcome`); `RemoteListener::respond()` appends the newline, nothing else. Never write raw
  text to a socket, never send anything unsolicited (the former `broadcast()` was deleted: a client
  matches each line to its request in order).
- **One write at a time**: `m_writeMutex` serializes the network thread (transport errors) and the main
  thread (responses) — two interleaved writes corrupt a line. `stop()` deliberately closes the sockets
  WITHOUT that lock (it must unblock a stuck write; taking it under `m_clientsMutex` would also invert
  the lock order of `respond()`).
- **Per-client share of the queue**: `MaxPendingCommandsPerClient` = 256 / 8 = 32. A client above it
  gets a last error line and is disconnected (`disconnect()`), so it can never make another client's
  command overflow, and no refusal ever overtakes an answer still queued. A line over 8192 bytes is
  handled the same way (the socket is now really closed; it used to be only forgotten).
- **Clients**: `tools/emeraude_console.py` (the one implementation: `Console.call()` → parsed response,
  `.run()` → text), `tools/remote-console.py` (CLI: text, `--json` for raw lines, exit 1 on failure).
  `nc` prints the raw JSON lines. ⚠️ An engine built before 2026-09-27 answers raw text: the current
  client refuses it with a protocol error.
- **Conformance**: `tools/console-conformance.py` against a live instance checks the wire format and
  the typed contract of every command (via `describeCommands()`), non-destructively.

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
python3 tools/remote-console.py "listObjects"

# List sub-objects of Core
python3 tools/remote-console.py "Core.lsobj()"

# List commands on a service
python3 tools/remote-console.py "Core.RendererService.lsfunc()"
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
| `describeCommands()` | Every command as JSON: path, help, typed, parameters (name, type, arity, description, default), hints |

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
python3 tools/remote-console.py "Core.ArgumentsService.getJson()"
python3 tools/remote-console.py "Core.FileSystemService.getJson()"
python3 tools/remote-console.py "Core.SettingsService.getJson()"
python3 tools/remote-console.py "Core.WindowService.getState()"
python3 tools/remote-console.py "Core.RendererService.getStatus()"
```

### Modify settings
```bash
python3 tools/remote-console.py "Core.SettingsService.set(Core/Video/Window/Width, 1920)"
python3 tools/remote-console.py "Core.SettingsService.save()"
```

### Window control
```bash
python3 tools/remote-console.py "Core.WindowService.resize(1920, 1080)"
```

### Music playback
```bash
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.play()"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.pause()"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.volume(50)"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlist()"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlist(play, 3)"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.status()"
```

### Resource discovery
```bash
# List all resource containers (skyboxes, meshes, materials, etc.)
python3 tools/remote-console.py "Core.ResourcesManagerService.listContainers()"
# Returns JSON: [{"id":"SkyBoxResource","name":"...","loaded":3,"available":5}, ...]

# List available resources in a specific container
python3 tools/remote-console.py "Core.ResourcesManagerService.listResources(SkyBoxResource)"
# Returns JSON: ["Miramar","DNCity","CloudyDay", ...]
```

### Screenshot and visual verification
```bash
# Take screenshot — returns the file path
python3 tools/remote-console.py "Core.RendererService.screenshot()"
# Screenshot saved: "/home/user/.local/share/LNIsle/projet-alpha/captures/<timestamp>.png"
```

## 5. AI Scene Creation (Critical)

The AI can create complete 3D scenes autonomously via the console. This is the foundation for AI-driven 3D content creation.

### Complete scene creation sequence

```bash
# Step 1: Create the scene with skybox and camera
# Camera is placed at Y=-2 (below ground level Y=0) to verify ground visibility
python3 tools/remote-console.py "Core.SceneManagerService.createScene(IAScene, 1024.0, Observer, 0.0, -2.0, 0.0, Miramar)"

# Step 2: Add ground AFTER scene creation
python3 tools/remote-console.py "Core.SceneManagerService.setGround(default)"

# Step 3: Take screenshot to verify the scene
python3 tools/remote-console.py "Core.RendererService.screenshot()"
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
python3 tools/remote-console.py "Core.SceneManagerService.createNode(MyObject, 5.0, 0.0, 5.0)"

# Move a node
python3 tools/remote-console.py "Core.SceneManagerService.setNodePosition(Observer, 0.0, 10.0, 20.0)"

# Orient a node to look at a point (convention under investigation)
python3 tools/remote-console.py "Core.SceneManagerService.setNodeLookAt(Observer, 50.0, 0.0, 50.0)"

# Inspect a node
python3 tools/remote-console.py "Core.SceneManagerService.getNode(Observer)"

# Destroy a node
python3 tools/remote-console.py "Core.SceneManagerService.destroyNode(MyObject)"

# Attach components to nodes
python3 tools/remote-console.py "Core.SceneManagerService.attachCamera(MyNode, MyCamera)"
python3 tools/remote-console.py "Core.SceneManagerService.attachMicrophone(MyNode, MyMic)"
```

### Scene inspection

```bash
python3 tools/remote-console.py "Core.SceneManagerService.getSceneInfo()"
python3 tools/remote-console.py "Core.SceneManagerService.listScenes()"
python3 tools/remote-console.py "Core.SceneManagerService.listNodes()"  # requires targetActiveScene() first
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
                        (succeeded, Outputs) → RemoteProtocol::serializeResponse() → respond() to that client (ALWAYS, one line)
```

### Thread safety

- `RemoteListener` uses `std::mutex` for the command queue and client list
- `Controller::poll()` is called on the **main thread** only
- All command execution happens on the **main thread** — safe to access engine state
- Responses are sent to the requesting client only, one JSON line each, under `m_writeMutex`

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
- **Bounded everywhere (2026-08-27 hardening)** — `MaxPendingCommands = 256` shared, `MaxPendingCommandsPerClient = 32` each (2026-09-27: a client above its share gets a last error line and is disconnected — it used to be answered `ERROR: command queue full` and kept, which let its refusal overtake answers still queued and let one client starve the others), `MaxLineLength = 8192` (the read buffer is capped: an unbounded `streambuf` let a peer that never sends a newline grow the process until the OOM killer fired, on an unauthenticated port; a longer line gets a last error line and is disconnected), `MaxClients = 8` (further connections get an error line). Every one of these answers is a `RemoteProtocol` JSON line, `SendTimeoutMilliseconds = 2000` (`SO_SNDTIMEO` on every accepted socket)
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
