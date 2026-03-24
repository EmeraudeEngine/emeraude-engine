# Console System - AI Context

## 1. Overview

The Console system provides **runtime command execution** for the engine. It is the **primary channel for AI-driven control** of a running application. An AI agent can create scenes, manipulate cameras, control audio, modify settings, and visually verify results — all through TCP commands.

### Remote Console (TCP)

A TCP server listens on a configurable port (default **7777**, setting: `Core/Console/RemoteListenerPort`). Any AI agent or external tool can connect and:

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

| Command | Effect |
|---------|--------|
| `help`, `lsfunc()` | List available commands |
| `listObjects`, `lsobj()` | List registered controllable objects |
| `exit`, `quit`, `shutdown` | Graceful shutdown (saves settings) |
| `hardExit` | Immediate shutdown |

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
| `groundMaterial` | Optional. "default" for basic grey, or a PBR material name |

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

Commands are registered in `onRegisterToConsole()` overrides using `bindCommand()`:

```cpp
// In YourService.console.cpp
void
YourService::onRegisterToConsole () noexcept
{
    this->bindCommand("doSomething", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
        // arguments[0].asString(), arguments[1].asFloat(), etc.
        // outputs.emplace_back(Severity::Info, "result message");
        // outputs.emplace_back(Severity::Info, jsonString); // for JSON responses
        return true; // success
    }, "Description of what this command does.");
}
```

**Rules:**
- Only **services** inherit `ControllableTrait` (not runtime objects like scenes or entities)
- Services register via `registerToObject(parentService)` to build the hierarchy
- Commands execute on the **main thread** — safe to access engine state
- Return `true` for success, `false` for failure
- For JSON responses, put the JSON string in a single output message
- Convention: separate `*.console.cpp` file (e.g., `Settings.console.cpp`)

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
- **Port 7777** is configurable via `Core/Console/RemoteListenerPort` setting
- **No authentication** — the remote console is currently open
- **Clean responses** — TCP responses contain only command output, no Tracer logs
- **JSON routing** — lines starting with `{` bypass the command parser and go to the JSON handler
- **AI Runtime Control** — See [`docs/ai-runtime-control.md`](../../docs/ai-runtime-control.md) for the complete AI operator reference
