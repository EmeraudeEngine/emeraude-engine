# AI Runtime Control Guide

## Purpose

This document is **THE reference** for any AI agent controlling the Emeraude-Engine at runtime via the Remote Console (TCP). It covers connection, resource discovery, scene creation (live commands and JSON), spatial orientation, visual verification, audio control, and the complete command reference.

The AI connects to a running engine instance via TCP and sends commands to discover resources, create scenes, place cameras, add objects, take screenshots, play music, modify settings, and verify results visually.

## Philosophy — the Remote Console is the AI's hands

> [!CRITICAL]
> The Remote Console (the `RemoteListener` / `ConsoleController` TCP service) is **the AI's hands
> on the running engine**. It is **designed by the AI, for the AI** — it exists so an autonomous
> agent can drive, inspect, and verify the engine at runtime without a human in the loop.
>
> **Consequently, the AI is entitled to extend it.** When a task needs a capability the console
> does not yet expose (trigger a GPU capture, dump a state, toggle a debug view, force a rebuild
> of some cache…), the correct move is to **add the command to the relevant service's
> `onRegisterToConsole()`** rather than work around its absence. Growing the console surface makes
> the engine more controllable and more flexible for every future AI session. This is encouraged,
> not exceptional.
>
> **Rules when adding a command:**
> - Register it on the service that owns the capability (e.g. rendering commands on `Renderer`).
> - Register it **typed** (2026-09-27): parameters deduced from the lambda, one `Parameter` (name +
>   description with its unit) per argument, a `CommandResult` returned, hints set. The help line and
>   its usage are generated, and the arguments are validated before the lambda runs. Pattern and
>   rules: [`src/Console/AGENTS.md`](../src/Console/AGENTS.md) § 3 and § 7. Never add an untyped
>   (`Arguments, Outputs`) form: it was DELETED on 2026-09-27 once every command had migrated (owner decision).
> - Keep it self-describing (clear name + description — it shows up in the command listing).
> - Document it here in the command reference, in the same work session.
>
> Example added this way: `Core.RendererService.triggerRenderDocCapture()` (see §6).

## Connection

The engine listens on **TCP port 7777** (configurable via `Core/Console/RemoteListenerPort`) — **but only
when `Core/Console/EnableRemoteListener` is `true`; the port is CLOSED BY DEFAULT.** The bind address is
`Core/Console/RemoteListenerAddress` (default `127.0.0.1`: same host only). If the connection is refused,
the running instance was launched without the key: **set it in `settings.json`, relaunch, and do not retry
against that instance** — nothing will open the port while it runs, except a human pressing
**Shift+F10** in the window (dialog asking the port, session only). Startup log when off:
`Remote console disabled (Core/Console/EnableRemoteListener = false)`. Once connected,
`Core.remoteConsoleStatus()` shows the endpoint and `Core.restartRemoteConsole(port | address:port)`
moves the console (the current connection closes; reconnect on the new endpoint).

**The client — `tools/remote-console.py` (Python 3, every OS):**
```bash
# Send a single command: prints the text of the response; exit status 1 when it failed
python3 tools/remote-console.py "command"

# Several commands on ONE connection (the console keeps per-connection state: targetActiveScene, ...)
printf '%s\n' "Core.SceneManagerService.targetActiveScene()" "Core.SceneManagerService.listNodes()" | python3 tools/remote-console.py

# The raw JSON response lines instead of their text
python3 tools/remote-console.py --json "Core.WindowService.getState()"

# Interactive mode (REPL)
python3 tools/remote-console.py
```

> **Note:** Always use `python3`, not `python`. On many systems (Linux, Windows), `python` may not exist or may point to Python 2. The script requires Python 3.
> A Python tool imports `tools/emeraude_console.py` (`Console.call()` returns the parsed response,
> `Console.run()` its text) — never re-implement the protocol.

### Wire format (since 2026-09-27, owner decision)

A request is **one command line**. **Every request gets exactly one response**, even a failure and
even a command with no output, and that response is **one JSON object on one line**
(`src/Console/RemoteProtocol.hpp`):

```json
{"ok":false,"outputs":[{"kind":"text","message":"Argument 'width' expects an integer, got 'abc'.","severity":"Error"}]}
```

- `ok` — whether the command succeeded.
- `outputs` — in display order, possibly empty; each has `severity` (Debug/Success/Info/Warning/Error/
  Fatal), `kind` (`text`; `json` = the `message` IS a JSON document to parse; `binary` = adds
  `mimeType` and a Base64 `data`; `image` = an image FILE, adds `mimeType` and `path` — a screenshot)
  and `message`.
- The **welcome banner** sent on connection is a response too, with a top-level `"protocol": 1`.
- Responses come **in request order**: requests may be pipelined, the N-th line answers the N-th request.
- **Transport limits** answer a last error line, then close THAT connection: a line longer than 8192
  bytes, or more than 32 requests waiting for their answer (a client's share of the 256-command queue,
  so a flooding client can never starve another one). Nothing is ever sent unsolicited.
- `describeCommands()` returns every command as JSON — path, description, parameters (name, type,
  arity, description, default) and hints: the machine-readable twin of `help`.

Before this format the server wrote raw text with no end marker: clients guessed the end of an
answer from silence, and a command with no output sent nothing at all, so the client waited for its
whole timeout. ⚠️ An engine built before 2026-09-27 still speaks raw text; the current client refuses
it with a protocol error instead of misreading it.

**`nc` / netcat** still works for a quick look, but prints the raw JSON lines
(`echo "Core.WindowService.getState()" | nc -q 1 localhost 7777` on Linux, `-w 1` on macOS; no `nc` on
Windows). Use the Python client for anything else.

**The conformance bench — `tools/console-conformance.py`**: run it against a live instance after any
change to `src/Console/` or to a command. It checks the wire format (banner, one ordered line per
request, transport limits) and the typed contract of EVERY command (declared, arguments validated
before running) without executing anything but read-only, argument-less commands. 2090 checks pass
on Linux (2026-09-27); it has been shown to fail against a server that breaks the contract.

### Argument syntax and validation

- Arguments are positional: `Core.WindowService.resize(1920, 1080)`. `<path>.help()` prints, for a
  **typed** command, one line per parameter with its type, whether it may be omitted, its meaning
  and unit, then its hints (`[read-only]`, `[idempotent]`, `[destructive]`).
- A typed command checks its arguments **before** running and answers an error that names the
  parameter: `Missing argument 'z' (float: Target world Z, in metres.)`,
  `Argument 'bypassed' expects a boolean, got '2'.`, `Too many arguments: 3 given, at most 2 accepted.`
  An integer parameter takes `5` or `5.0`, never `5.5`; a boolean takes `true`/`false`/`1`/`0` only.
- **Quote** a string that contains a comma, a parenthesis or a dot you do not want split:
  `Core.openFiles("/tmp/a,b(1)/x.glb")` — inside quotes every character is literal (2026-09-27; the
  parser used to cut a quoted path on its commas).

### The MCP server — every typed command as a tool (2026-09-27)

The engine embeds a **Model Context Protocol server** (`src/Console/MCP/`): an MCP client (Claude
Code first) discovers every typed console command as a tool with its JSON schema, calls it with named
arguments, and receives captures **inline as images**. It is independent of TCP 7777 — either can be
open without the other — and **closed by default**, like the console.

**Settings** (`settings.json`, read at launch; written with their defaults on first run):

| Key | Default | Meaning |
|---|---|---|
| `Core/MCP/Enabled` | `false` | Start the server at all |
| `Core/MCP/Address` | `127.0.0.1` | Bind address. ⚠️ A non-loopback address REFUSES to start without a token |
| `Core/MCP/Port` | `17778` | Endpoint `http://<address>:<port>/mcp` (7778 until 2026-09-27, see below) |
| `Core/MCP/BearerToken` | `""` | Every request must carry `Authorization: Bearer <token>` when set |

The startup log says `MCP server listening on http://127.0.0.1:17778/mcp (protocol 2026-07-28 and
2025-11-25 era).`; `Core.remoteConsoleStatus()` reports the endpoint in its `mcp` field (null when off).
⚠️ **Port already taken**: ASUS Armoury Crate listens on `127.0.0.1:7778` on ASUS laptops (seen by the
Windows peer, 2026-09-27), and Windows reports a port held exclusively by another process as ACCESS
DENIED, not "in use". The log now says so (`the port is most likely taken by another process`); set
`Core/MCP/Port` to a free port. The engine keeps running, only MCP stays closed. That conflict is why
the default moved from 7778 to **17778** (owner decision, 2026-09-27). ⚠️ projet-alpha never resets its
settings: a `settings.json` written by an earlier build keeps `Core/MCP/Port = 7778` — change it by hand.

**Connecting Claude Code** (once the engine runs with `Core/MCP/Enabled = true`):
```bash
claude mcp add --transport http emeraude http://127.0.0.1:17778/mcp
# with a token: --header "Authorization: Bearer <token>"
```
Keep the server name short (`emeraude`): Claude Code calls a tool `mcp__<server>__<tool>`, which must
fit 64 characters, and the tool names are budgeted for that prefix (≤ 49 characters). A long capture
(`Renderer_temporalCapture`) may need a larger per-server `timeout` in `.mcp.json` (milliseconds).

**An application adds its own tools** by registering its own console objects: projet-alpha's top-level
`Stage` object gives `Stage_listDemos`, `Stage_loadDemo`, `Stage_unloadAct`, … (projet-alpha
`src/AGENTS.md` § 3a). A client should find a command for what it wants before resorting to
`InputManager_mouseClick` on a web UI.

**Tools**: one per typed command, aliases merged — 127 on `coordinates-debug`, 108 with no act loaded.
Names drop `Core.` and each `Service` suffix, dots become `_`:
`Core.SceneManagerService.PostProcess.select` → `SceneManager_PostProcess_select`,
`Core.RendererService.screenshot` → `Renderer_screenshot`. Each tool's `inputSchema` comes from the
declared signature (types, integer range, defaults, descriptions with units, `required`,
`additionalProperties: false`); its annotations from the hints (`readOnlyHint`, `destructiveHint`,
`idempotentHint`; `openWorldHint` is always false). An omitted optional argument before a supplied one
is fine (named arguments). A wrong or unknown argument answers a tool result with `isError: true`
naming it — the model can correct itself — and the command never runs. A `json` output is also given
as `structuredContent`.

**`Renderer_screenshot`** returns the next presented frame as an inline PNG **reduced to 1568 px** on
its long edge (area filter), plus the path of the full-resolution file for pixel measurement (owner
decision). TCP 7777 keeps answering only the path: the image is read and reduced only by the channel
that shows it (`OutputKind::Image`).

**Both protocol eras** (owner decision): 2026-07-28 (stateless; `server/discover`, per-request
`_meta` checked against the `MCP-Protocol-Version`, `Mcp-Method` and `Mcp-Name` headers,
`subscriptions/listen`) and the handshake era (2025-11-25 / 2025-06-18 / 2025-03-26: `initialize`,
a GET notification stream). ⚠️ Claude Code's v1 client runtime only speaks the handshake era; its v2
runtime asks for 2026-07-28 (`MCP_SDK_GENERATION`, `MCP_PROTOCOL_NEGOTIATION`); the official v2
TypeScript client only tries it with `versionNegotiation: { mode: "auto" }`. Measured 2026-09-27: SDK
v1 (1.30.1) negotiates 2025-11-25, SDK v2 (2.1.0, auto) negotiates 2026-07-28, both list the tools,
call them, get the image and receive `list_changed`.

**`notifications/tools/list_changed`** is sent on every open notification stream when the console tree
changes (the active scene's `PostProcess` node and projet-alpha's `Act` node come and go: F4 unloads the
act, 127 → 108 tools).

**Security**: `Origin` (DNS rebinding — CEF renders web pages inside this very process) and `Host`
(loopback binding) are validated on every request (403 otherwise); bearer token; request head
≤ 16 KiB, body ≤ 1 MiB (refused on the declared length), no chunked encoding, duplicated framing
headers refused (request smuggling); 16 connections, 64 requests waiting for the main thread, 120 s
idle, 30 s per request. Malformed JSON of any shape answers a JSON-RPC error and never stops the engine.

**The conformance bench — `tools/mcp-conformance.py`** (no SDK needed, plain HTTP): run it after any
change to `src/Console/` or to a command, with `--trigger-list-change` to press F4 and check the
notifications. 885 checks pass on `coordinates-debug` (2026-09-27); it has been shown to fail against
a server that breaks the contract.

---

## 1. Resource Discovery

Before creating a scene, the AI must know what resources are available. The engine provides two discovery commands.

### List all resource containers

```bash
python3 tools/remote-console.py "Core.ResourcesManagerService.listContainers()"
```

**Response** (JSON array):
```json
[
  {"id":"SkyBoxResource","name":"SkyBox Resources","loaded":3,"available":5},
  {"id":"MeshResource","name":"Mesh Resources","loaded":0,"available":12},
  {"id":"StandardResource","name":"Standard Material Resources","loaded":0,"available":8},
  {"id":"BasicResource","name":"Basic Material Resources","loaded":1,"available":1}
]
```

Each entry contains:
- `id` -- the container ClassId (use this to query resources)
- `name` -- human-readable name
- `loaded` -- number of resources currently loaded in memory
- `available` -- number of resources available (defined in store files)

### List resources in a container

```bash
python3 tools/remote-console.py "Core.ResourcesManagerService.listResources(SkyBoxResource)"
```

**Response** (JSON array of resource names):
```json
["Miramar","DNCity","CloudyDay","Sunset","NightSky"]
```

You can pass either the `id` (ClassId) or the `name` from `listContainers()`.

### Discovery workflow

```
1. listContainers()                     --> know what types of resources exist
2. listResources(SkyBoxResource)        --> available skyboxes
3. listResources(MeshResource)          --> available 3D meshes
4. listResources(StandardResource)      --> available PBR materials
5. Use discovered names in createScene(), addMesh(), setBackground(), etc.
```

---

## 2. Spatial Orientation for AI Agents

> **You are an AI operating inside a 3D world. You have no spatial intuition. This section teaches you how to orient yourself.**

### The coordinate system

```
        Y+ (up)
        |
        |
        |_______ X+ (right)
       /
      /
     Z+ (forward/toward you)
```

- **Y axis is vertical.** Y=0 is ground level. Y>0 is above ground (sky). Y<0 is below ground.
- **X axis is horizontal.** Left/right.
- **Z axis is depth.** Forward/backward.

### Where is the ground?

The ground is a flat plane at **Y=0**. It extends horizontally in X and Z.

- If your camera is at **Y=5**, you are 5 units above the ground.
- If your camera is at **Y=-2**, you are 2 units below the ground -- you will see the ground **above you** as a grey surface.
- If your camera is at **Y=0**, you are **on** the ground -- you cannot see it (you're inside the plane).

### How to verify you can see the ground

1. Place the camera **below** the ground: `setNodePosition(Camera, 0.0, -2.0, 0.0)`
2. Take a screenshot
3. Read the image: **if you see a grey/colored flat surface in the upper half of the image, the ground exists**
4. The lower half will show the skybox from below

This is your **ground verification test**. Always do this when creating a new scene.

### How to see the ground from above

Once you've confirmed the ground exists:
1. Move the camera above ground: `setNodePosition(Camera, 0.0, 10.0, 20.0)`
2. You need to **look downward** to see the ground
3. The default camera orientation looks roughly along the Z axis (horizontal)
4. Use `setNodeLookAt(Camera, x, y, z)` to orient the camera toward a point in world space
5. To look at the ground ahead: `setNodeLookAt(Camera, 20.0, 0.0, 0.0)` (target at ground level, ahead)

### Temporal capture: N consecutive frames (shimmer, temporal artefacts)

```bash
python3 tools/remote-console.py "Core.RendererService.temporalCapture(8)"
python3 tools/temporal-analysis.py <stem>          # or the .json path it printed
```

- `screenshot()` and `temporalCapture([N = 5])` both copy the PRESENTED image inside its frame (UI
  included) and answer once the files are written. ⚠️ Read the path they ANSWER: the stem is unique,
  so a burst of captures moves it past the clock (two screenshots in one second used to collide). A
  capture that times out is cancelled, and the next one arms normally. A temporal capture writes
  `<unix seconds>-<n>.png` for n = 0..N-1 and `<unix seconds>.json` (frame serial, timing, TAA
  jitter, camera, exposure per frame). Budget 1 GiB of staging (57 frames at 2880×1620).
- ⚠️ Capture **8** frames or more for a TAA question: the jitter cycle is 8 frames.
- Park the camera, PIN the exposure (`Camera.setExposure`) for an A/B, and let the scene converge: the
  analysis prints the flatness line first and warns when the camera moved or the exposure is auto.
- The analysis gives the per-pixel temporal peak-to-peak (mean, p99, p99.9, shares > 1/2/4/8/16),
  horizontal bands (distance on a ground), the worst tiles, the gradient/Laplacian signature and two
  maps (`-ptp.png` ×16, `-heat.png`). `--crop x,y,w,h`, `--compare other.json`.

### Understanding what you see in a screenshot

```
+-------------------------+
|                         |  <-- Sky (skybox, clouds)
|       SKY               |
|                         |
+-------------------------+  <-- Horizon line
|                         |
|       GROUND            |  <-- Ground surface (grey/textured)
|                         |
+-------------------------+
```

- **If you see only sky:** your camera is pointing upward, or the ground doesn't exist, or you're below ground looking down
- **If you see only grey/color:** you're very close to the ground looking straight at it, or below ground looking up
- **If you see sky on top and ground on bottom:** you have a proper view. The **horizon line** separates sky from ground
- **If you see ground on top and sky on bottom:** you're below ground (Y<0), looking at the underside

### How orientation works

`setNodeLookAt(nodeName, targetX, targetY, targetZ)` makes the camera look at a **point in world space**.

From camera position `(camX, camY, camZ)`:
- Looking at `(camX, camY-10, camZ)` = looking straight **down**
- Looking at `(camX, camY+10, camZ)` = looking straight **up**
- Looking at `(camX+100, camY, camZ)` = looking **right** along X
- Looking at `(camX, camY, camZ+100)` = looking **forward** along Z
- Looking at `(camX+50, camY-5, camZ+50)` = looking **ahead and slightly down** (typical ground view)

### AI workflow for spatial awareness

```
1. Create scene with camera at Y=2      --> verify ground exists (screenshot)
2. Move camera to Y=10, Z=20            --> above ground, behind origin
3. LookAt (0, 0, 0)                     --> aim at origin (ground level)
4. Screenshot                            --> analyze: do I see sky+ground?
5. If only sky --> orientation is wrong  --> try different lookAt target
6. If sky+ground --> success             --> adjust framing as needed
7. Repeat 3-6 until desired view
```

**The key insight:** you cannot "see" the 3D world. You must take screenshots and analyze the images to understand what the camera sees. This is your visual feedback loop. Use it after every camera change.

---

## 3. Scene Creation via Live Commands

### Minimal scene (skybox only)

```bash
python3 tools/remote-console.py "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, 2.0, 0.0, Miramar)"
```

Parameters: `createScene(name, boundary, cameraNodeName, camX, camY, camZ [, skyboxName [, groundMaterial]])`

### Scene with ground

The ground can be added inline or after scene creation:

```bash
# Option 1: Inline ground material (8th parameter)
python3 tools/remote-console.py "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar, default)"

# Option 2: Add ground AFTER scene creation (recommended for flexibility)
python3 tools/remote-console.py "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar)"
python3 tools/remote-console.py "Core.SceneManagerService.setGround(default)"
```

Ground materials:
- `default` -- flat grey surface (BasicResource)
- Any material name -- PBR textured surface (StandardResource)

### Scene with specific background

```bash
python3 tools/remote-console.py "Core.SceneManagerService.setBackground(Miramar)"
```

Available skyboxes depend on the application's resource store. Use `listResources(SkyBoxResource)` to discover them.

### Lighting

`createScene` automatically adds a neutral PHOTOMETRIC ambient (white, 5000 lx, overcast-like). No manual light setup needed for basic scenes; add directional/point/spot lights explicitly, or derive the lighting from the background (see `ApplyLighting` in the JSON scene format).

### Adding 3D objects

```bash
# addMesh(meshResource, entityName, x, y, z [, scale])
python3 tools/remote-console.py "Core.SceneManagerService.addMesh(Sponza, SponzaEntity, 0.0, 0.0, 0.0, 0.01)"
```

Use `listResources(MeshResource)` to discover available meshes.

### Critical rules for scene creation

1. **Camera is created BEFORE `enableScene()`** -- this prevents the engine from creating a default camera that overrides yours (on a node OR a static entity — since Aug 2026 the check sees both; before, a camera on a static entity was overridden)
2. **Camera Y=-2 is the verification position** -- below ground, you see the grey ground above you
3. **Camera Y=0 won't see the ground** -- same level as ground plane
4. **Lighting is automatic** -- `createScene` adds a neutral photometric ambient (5000 lx)
5. **Always verify with screenshot** -- take a screenshot and read the PNG to confirm visual output

### Opening files (the dropped-files pipeline)

`Core.openFiles(path[, path, ...])` runs the SAME pipeline as dropping files onto the window —
the only way to exercise it remotely (a real drag & drop cannot be injected). Runs on the main
thread; heavy imports stall the loop until done.

```bash
python3 tools/remote-console.py 'Core.openFiles("/abs/path/picture.png", "/abs/path/model.glb")'
```

Per-file behavior (after the application's `onCoreOpenFiles()` hook declined):

| File | Behavior |
|------|----------|
| JSON with `"Stores"` key | Completes the resource stores (`Resources::Manager::update()`) — consumed at the Core first-view stage, before the application hook |
| JSON scene definition (`Nodes`/`StaticEntities`/`Boundary` keys) | `loadScene()`; enabled only when NO scene is active |
| Image (`jpg/jpeg/png/tga/tif/tiff/hdr`) | `+ImageViewer` scene: unlit double-sided quad at the image ratio, orbit camera |
| Composite asset (any extension a scene loader supports: glTF/GLB/FBX/USD*/WAD) | `+ModelViewer` scene: neutral lighting (key + cool fill), manual sunny-16 HDR exposure, orbit camera framed on the model |
| Raw geometry (`obj/stl/mdl/md2/md3/md5mesh/ee3d`, per `VertexFactory::FileIO::isReadableExtension()`) | Same `+ModelViewer` scene, the mesh wears a neutral clay material (mid-grey, dull, dielectric) |
| Audio (`wav/flac/ogg/oga/opus/mp3/aiff/aif/au/caf/mid/midi`) | Ad-hoc track appended to the TrackMixer playlist and played immediately (`nowPlaying()` to verify) |
| Anything else | On-screen notification `Unable to open the file '...'` |

Viewer policy: a regular active scene is **never disturbed** (notification `A scene is running,
the file was ignored.`); an active `+ImageViewer`/`+ModelViewer` is **replaced** by the new drop.
In the viewers: left-drag orbits around the content, mouse wheel dollies in/out.

#### `+ModelViewer` animations — OFF on arrival, space bar walks them (Aug 2026)

An animated asset opens **at rest**, never playing: `ModelViewer::createScene()` clears
`enableAutoPlayFirstClip()` on every renderable carrying skeletal data, and collects the clip
names. The **space bar** then walks the cycle `OFF -> clip 1 -> ... -> clip N -> OFF`, so a full
turn always returns to the rest pose. `Core.cycleAnimation()` is the remote equivalent, calling
the very same `Core::cycleViewerAnimation()`.

⚠️⚠️ **NODE animations play only since 2026-09-26.** A glTF animation driving plain nodes (no skin —
the `ChronographWatch`'s second hand) needs a node hierarchy, and the viewer built every asset as
STATIC entities, which bake their world frame at build time: the space bar announced
`Animation 1/1: Anim_0` and nothing moved (log: `The asset carries node animations, which STATIC
mode cannot play`). The viewer now builds such an asset under a `ModelRoot` node (and frames on that
subtree); every other asset stays static. Measured: second hand still at rest, moving while playing,
still again after the OFF step. A skeletal clip always played in both modes — the cycle had only been
verified on skinned assets.

**`Core.resetAnimation()` forces the rest pose in ONE call** (added 2026-09-18), whatever the cycle
was on. The cycle alone cannot do it: reaching the rest pose from an unknown position takes as many
calls as the asset has clips, and a caller cannot know how many without tracking the state itself.
An asset carrying no animation is already at rest and counts as a success, so the command is usable
as a plain precondition; it fails only when the model viewer is not the active scene.

> [!IMPORTANT]
> **It draws NO on-screen notification, deliberately — that is the whole point of it.**
> `cycleAnimation()` announces itself with a toast, and a screenshot taken within the notifier's
> lifetime has that toast **burnt into the frame**. Use `resetAnimation()` before any automated
> capture. Verified 2026-09-18 on `CesiumMan`: the capture comes back to the rest pose within a
> single LSB (max delta **1/255**, against 219 for an actual pose change) and the notification band
> is bit-identical to a clean frame, where a `cycleAnimation()` toast lights 536 pixels.

> [!CAUTION]
> **A stray space bar on the window silently corrupts an automated capture.** Measured 2026-09-18:
> one keypress during a conformance-bench run took a model's A/B from a mean of 0.10/255 to 9.22 —
> an 80x jump that reads exactly like a real defect, and it cost a false regression diagnosis. The
> tells: `m_viewerAnimationIndex` resets on every model open, so only the capture between the
> keystroke and the next load is hit (an asymmetry no codec can produce), and the frame **shows**
> the `Animation 1/1: <clip>` overlay. Look at the image before trusting a number that moved.

⚠️ **The console command is the deterministic remote way in — and the claim that it was the ONLY
one is doubtful since 2026-09-13.** This paragraph used to state that a keyboard event injected
through `Input::Manager::injectKeyEvent()` **never reaches a Core-level binding** (observed Aug 2026
> [!IMPORTANT]
> **`mouseClick()` CANNOT drive anything that needs a held button.** It injects a press *and* a
> release, so every `mouseMove()` sent afterwards arrives with the button already up: an orbit
> camera, a gizmo drag, a slider — all see nothing, and the capture comes back **bit-identical**,
> which reads exactly like a broken control. Use the pair added 2026-09-18:
>
> ```
> Core.InputManagerService.mousePress(x, y, 0, 0)
> Core.InputManagerService.mouseMove(x2, y)          # as many as the gesture needs
> Core.InputManagerService.mouseRelease(x2, y, 0, 0)
> ```
>
> Measured on a model viewer: a held drag moves **99.82 %** of the pixels where the same gesture
> through `mouseClick` moves **0.0000 %**. ⚠️ The engine does NOT pair them — a forgotten
> `mouseRelease` leaves the control believing the button is still down.

on `keyPress(32, 0)`, which cycled nothing). On 2026-09-13 the consumer projet-alpha injected F9
(`keyPress(298, 0)`) and its `Application::onCoreKeyRelease()` toggled the compass on screen — the
same `Core::onKeyRelease()` entry the space bar's default behaviour sits behind, since `Core`
registers itself as a keyboard listener. The space bar itself was **not** re-measured; treat the
Aug 2026 observation as unreproduced rather than as a rule, and prefer `Core.cycleAnimation()`
because it does not depend on which scene has the focus.

⚠️ **`enableAutoPlayFirstClip(false)` mutates a CACHED resource**: the flag survives for every
later instance of the same asset in the session, viewer or not. An asset opened in the viewer and
then loaded by a demo appears in its bind pose.

⚠️ The space bar sits in Core's **default** key behaviors, after `onCoreKeyRelease()`: an
application or demo binding the space bar keeps it, and the cycle is a no-op unless
`+ModelViewer` is the active scene. Note that Core already owns **Shift**+KeyPad1/2/3 (gamma
presets 0.8/1.0/1.2) — those are unrelated and unmodified.

⚠️ Measuring "is it animating?" on raw pixels **does not discriminate**: the viewer camera runs
automatic exposure and a temporal chain, so two consecutive frames always differ (measured: 10.65 %
of pixels change with the animation OFF). Compare **heavily downsampled** frames instead — the
sensor noise is high-frequency and global, a moving limb is low-frequency and local. On a 96x54
reduction the noise floor was 144/5184 cells against 736/5184 for a playing clip.

#### `+ModelViewer` environment — three settings, and two of them are INDEPENDENT axes

| setting | default | what it decides |
|---|---|---|
| `Core/Viewers/Background` | `GreenLandscape` | what is **behind** the subject. **Empty = no backdrop at all**, which renders a bit-exact black |
| `Core/Viewers/EnvironmentCubemap` | *(empty)* | what the subject **reflects**. Empty = whatever the background installed |
| `Core/Viewers/AmbientIlluminance` | `80.72` | the flat ambient illuminance floor, in lux, DELIVERED (renamed 2026-09-25 from `AmbientIntensity` = 200 × a (0.4, 0.4, 0.45) colour that dimmed it; the light colour is a unit-luminance chromaticity since) |

> [!IMPORTANT]
> **The backdrop and the reflected environment are NOT the same axis**, and the engine already
> separates them (`Scene::setBackground()` vs `Scene::setEnvironmentCubemap()`, the latter documented
> as replacing whatever a background installed). Keeping them apart is what makes the configuration
> the Khronos conformance references actually use expressible: **a black backdrop with a bright
> studio reflection**. A single "environment" preset could not express it.
> ⚠️ The override is applied AFTER the background, necessarily — a background installs its own
> cubemap as the scene's environment, so an explicit choice has to come second.

**Why the ambient is a setting.** ~80 lux of flat ambient (200 × the old dimming colour until 2026-09-25) is enough to wash out a sheen rim or an
iridescence fringe, which is exactly what the tests Khronos shoots on black are measuring. Measured
on `SheenCloth`: with the defaults the backdrop reads sRGB (23.4, 28.2, 19.7) with a maximum of 44,
and the rim-to-backdrop contrast is **8.1×**; with `Background = ""` and `AmbientIlluminance = 0` the
backdrop is **exactly (0, 0, 0)**, maximum 0, and the contrast is **390×** — a **48× gain on the
discriminating figure**, and a backdrop that is a measurement surface rather than a picture.

⚠️ Both new keys default to the previous behaviour, so an existing session is unchanged. They
self-register into `settings.json` on first launch.

⚠️⚠️ **A black backdrop is NOT the universal answer.** `SpecularTest`'s spheres declare
`baseColorFactor [0,0,0,1]`, `metallicFactor 0` and `roughnessFactor 0`: their only light is a mirror
reflection at F0 ≤ 0.04, so they are too DARK, not washed out, and they need a bright *reflected*
environment — the other axis. Read what a test declares before choosing its environment.

### Calling a web API (`Core.NetAPIClientService.*`)

The engine talks to HTTPS APIs at C++ level, **without CEF**, through `Net::APIClient`. Every call
is asynchronous and addressed by a **ticket**; the response comes back on the main thread.

```bash
python3 tools/remote-console.py 'Core.NetAPIClientService.isEnabled()'
python3 tools/remote-console.py 'Core.NetAPIClientService.setHeader(Authorization, Bearer xxxxx)'
python3 tools/remote-console.py 'Core.NetAPIClientService.get(https://api.github.com/repos/EmeraudeEngine/emeraude-base)'
python3 tools/remote-console.py 'Core.NetAPIClientService.status(1)'      # status, httpStatus, body
python3 tools/remote-console.py 'Core.NetAPIClientService.release(1)'     # ⚠️ frees the response
```

| Command | What it does |
|---|---|
| `isEnabled()` | Whether calls are performed, and the ticket accounting |
| `request(METHOD, url[, body[, contentType]])` | Any method — `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, … |
| `get(url)` / `post(url, body[, contentType])` | Shorthands; `contentType` defaults to `application/json` |
| `status(ticket)` | JSON: status, HTTP status, content type, whether the body parsed as JSON, and **the body itself** |
| `header(ticket, Name)` | One response header (pagination cursor, rate-limit budget) |
| `list()` | Every held ticket |
| `release(ticket)` | **Drops the response.** Do it once read |
| `cancel(ticket)` | Abandons a ticket — see the warning below |
| `setHeader(Name, value)` / `removeHeader(Name)` / `headers()` | The default headers sent with every call (where an `Authorization` belongs) |

⚠️ **A ticket status of `Done` does NOT mean the API accepted the call.** `Done` means a response
arrived; a 404 or a 422 is `Done`. Read `httpStatus`. Only a call that never completed is `Error`,
and then `reason` names the cause (`TLSFailure`, `Unreachable`, `Timeout`, `BadRequest`, …).

⚠️ **`cancel()` does not interrupt anything on the wire.** The HTTPS client is synchronous and has
no cancellation point: a call already in flight runs to completion on its worker and its response
is thrown away. `cancel()` frees the caller, not the socket.

⚠️ **Responses are held in RAM and are NOT kept forever.** `Core/Net/API/MaxRetainedTickets`
(default 64) drops the **oldest terminal** tickets when nobody releases them — possibly one whose
response was never read. Call `release(ticket)`.

⚠️ **`headers()` prints values in clear, bearer token included** — owner decision of 2026-08-28,
taken knowingly over the redacting alternative. What contains it is that this console is closed by
default and binds `127.0.0.1`.

This is NOT the download manager: `Core.NetManagerService.*` fetches files into a disk cache,
deduplicates by URL and retries. `Core.NetAPIClientService.*` does none of those, on purpose — see
[`../src/Net/AGENTS.md`](../src/Net/AGENTS.md) § Web API client.

### Driving an entity's components (`Core.SceneManagerService.<Type>.*`, 2026-09-27)

Owner decision: one set of TYPED commands per component type, and **explicit addressing** — every
command names its `entity` and its `component` (the name on that entity). No targeting state: two
clients cannot steal each other's target, as the MCP specification recommends. Discover first, then act:

```bash
python3 tools/remote-console.py "Core.SceneManagerService.listEntities()"            # JSON: every node + static entity, its ADDRESS, its components
python3 tools/remote-console.py "Core.SceneManagerService.listEntityComponents(Bulb1)"  # JSON: [{"name":"Bulb","type":"PointLight"}]
python3 tools/remote-console.py "Core.SceneManagerService.PointLight.setLuminousPower(Bulb1, Bulb, 800)"
python3 tools/remote-console.py "Core.SceneManagerService.Camera.getActive()"         # the rendering camera: "address" + "name"
python3 tools/remote-console.py "Core.SceneManagerService.highlightEntity(Bulb1)"      # the selection OUTLINE (full visible, dimmed hidden)
python3 tools/remote-console.py "Core.SceneManagerService.setHighlightStyle(1, 0.6, 0.1, 2, 0.35)"  # sRGB colour, px, hidden opacity
python3 tools/remote-console.py "Core.SceneManagerService.clearHighlight()"
```

The outline is the editor's selection feedback, usable to POINT at an entity in a screenshot: graphics
`docs/subsystems/graphics/34-the-selection-outline-custom-depth.md`.

**Addressing** (owner decision, 2026-09-27): `entity` is the entity's ADDRESS — its name when unique in
the scene, else the shortest suffix of its node path that is (`ACTOR_…06/Head`). Every projet-alpha actor
carries a `Head` node (camera `Eyes`, microphone `Ears`, `SoundEmitter`): on `animation-debug` six of them,
and a bare `Head` is **refused** with the six candidate addresses — it used to act silently on the first
node found. `listEntities()` gives each entity's `address`; `Camera.getActive()` answers the rendering
camera's `address` and component `name`, ready to pass. A static entity's address is its name.

| Type | Commands (after `entity, component`) |
|---|---|
| `PointLight` | `getState` (JSON: enabled, colour, candela, radius, shadow, colour projection), `setEnabled(bool)`, `setColor(r, g, b)` (0-1, a chromaticity), `setLuminousPower(lumens)`, `setRadius(metres, 0 = unbounded)`, `setPCFRadius(r)`, `setShadowBias(b)`, `setProjectionBoost(k)` |
| `SpotLight` | the same + `setConeAngles(innerDeg, outerDeg)` (0 ≤ inner ≤ outer ≤ 90; change the cone BEFORE the power: lumens are converted with the current outer angle) |
| `DirectionalLight` | `getState` (+ CSM cascade count/lambda or coverage), `setEnabled`, `setColor`, `setIlluminance(lux)`, `setPCFRadius`, `setShadowBias`, `setProjectionBoost` |
| `Camera` | `getActive()` (no argument), `getState`, `setLens(focalMm, sensorWidthMm?)`, `setViewDistance(m)`, `setExposure(f, shutterS, iso)` (PINS the triad, auto-exposure off), `setAutoExposure(bool)`, `setExposureCompensation(ev)`, `setSensitivityRange(minIso, maxIso)`, `setFocus(m?)` (omitted = auto focus), `setBloom(thresholdNits, fraction)` |
| `SunCourse` | `getState` (phase, elevation, lux, kelvins, course), `start`, `stop`, `setPhase(0-1)` (0 sunrise, 0.25 noon), `setCourse(dayDuration?, noonElevation?, zenithIlluminance?, extinction?, zenithTemperature?, horizonTemperature?)` — the phase is kept |
| `SkyFollowsSun` | `getState` only (factor, day luminance, twilight curve) |
| `CloudVolume` | `getState`, `setLook(opticalThickness?, erosion?, detailFrequency?, boilingSpeed?, skyTint?, albedoRed?, albedoGreen?, albedoBlue?)` (the albedo: three together) |
| `NodeAnimation` | `getState` (clips, active, wrap, speed), `play(clip, wrap = "Loop")` (`Once`/`Loop`/`PingPong`), `stop` (rest frame restored), `setSpeed(≥ 0)` |
| `ParticlesEmitter` | `getState`, `start`, `stop`, `setSpawnRate(perCycle ≤ limit)`, `setLifetime(minCycles, maxCycles?)`, `setSize(min, max?)`, `setSizeDelta(perCycle)`, `setSpreadingRadius(m)`, `setChaos(k)` — cycles are logic cycles, 60 per second |
| `DirectionalPushModifier` | `getState`, `setEnabled`, `setMagnitude`, `setDirection(x?, y?, z?)` (three = a custom world direction; none = follow the entity again) |
| `SphericalPushModifier` | `getState`, `setEnabled`, `setMagnitude` |
| `Weight` | `getState` (mass, drag…, bounds — `null` when unset), `setRadius(m)`, `setBoxSize(x, y?, z?)` |
| `SoundEmitter` | `getState` (playing, gain, attached sound, loop, Doppler), `setGain`, `setVelocityDistortion(bool)`, `replay`, `stop`, `pause`, `resume`, `rewind` |
| `Visual`, `MultipleVisuals` | `getState` (renderable, LOD count, instance count, build-time switches, distances), `setDrawDistance(near, far)`, `setShadowDistance(m)`, `setShadowLODBias(levels)` |
| `Beam` | `getState` (segmentCount, start/end in entity space, followed entity NAME + offset or `null`, enabled, colour, luminance, width, arc), `setStart(x, y, z)`, `setEnd(x, y, z)` (forgets the target), `setEndTarget(targetAddress, ox, oy, oz)` (offset in the target's space), `setEnabled(bool)`, `setColor(r, g, b)` (linear hue), `setLuminance(nits)`, `setWidth(halfWidth, coreExponent)`, `setArc(amplitude, frequency, octaves)` (amplitude 0 = laser; an amplitude on a beam of `segmentCount` 1 is REFUSED: it cannot wander), `setArcMotion(seed, restrikeRate, drift)` — graphics doc 33 |

Every setter answers its confirmation **and the component's new state as JSON** (MCP
`structuredContent`), so the applied values are confirmed without a `getState()` call (suggested by
Gemini's review of the server, 2026-09-27). MCP names: `SceneManager_<Type>_<command>`,
`SceneManager_listEntities`, … An error names what is wrong: an unknown or ambiguous entity, a component
of another type (`is a SpotLight, not a PointLight`), an out-of-range value — nothing is applied then.
The commands run under EXCLUSIVE access to the active scene, so they wait at most one frame.

**What is deliberately NOT drivable** — a setter that the engine clamps or ignores is REFUSED with the
reason, never applied halfway:
- **Shadow on/off**: `enableShadowCasting()` is a creation-time switch. Flipped at runtime it only froze
  the CSM fitting while the map was still drawn and sampled — the whole `forest` floor went INTO shadow
  (engine item `light-shadow-runtime-toggle`). `setPCFRadius`/`setShadowBias` refuse a light built
  without a shadow map; `setProjectionBoost` refuses a light without a projection texture.
- **Visual lighting / shadow casting / ray tracing**: decided when the instance is built (shader
  generation, ray-tracing lists) — reported by `getState`, not settable.
- **Camera DoF / motion blur switches**: `Core/Graphics/PostProcessing/DepthOfField|MotionBlur/Enabled`
  override the camera; `getState` reports them.
- `setExposure` refuses an ISO outside the camera's range (the setter clamps: widen it with
  `setSensitivityRange` first); `setSpawnRate` refuses more than the particle limit; `setShadowLODBias`
  refuses more levels than the renderable holds.
- **Loading a sound** by resource name (owner decision 2026-09-27: the emitter's ATTACHED sound only);
  `ParticlesEmitter.start` takes no timeout (engine item `particles-emitter-timeout-unit`); `Microphone`
  has nothing to drive or read and has no adapter.

⚠️ **An optional parameter in the MIDDLE is reachable by NAME only (MCP arguments).** The console's
positional syntax has no hole: `setLook(Cloud044, Cloud0, , , , , , 1, 0.5)` set the optical thickness
to 1 and the erosion to 0.5. At the console, give every parameter up to the last one you change.

⚠️ **Let the change reach a frame before capturing.** The light is published by the next logic tick and
drawn from the following render state: a screenshot taken milliseconds after the command showed the
PREVIOUS image and read as "the command does nothing" (2026-09-27). Wait ~1 s, and measure the HUE of
the lit area, not the luminance, if the auto-exposure is on (it absorbs a brightness change). Measured
on `light-and-shadow-debug`: the pink spot's floor patch reads b−g = +3…+9 on, −24 off, both ways, as
the demo's own KeyPad2 toggle does.

### Driving the post-process chain (`Core.SceneManagerService.PostProcess.*`)

The active scene's chain is addressable at runtime. Both lighting lanes are resident in every
scene, so switching is free of any rebuild and compares two techniques **on the very same
framing**.

```bash
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.listEffects()'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.getStatus()'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.setLightingMode(ScreenSpace)'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.setLightingMode(RayTracing)'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.setLightingMode(None)'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.select(Reflections, SSREffect)'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.disable(AmbientOcclusion)'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.bypassSceneEffects(1)'
python3 tools/remote-console.py 'Core.SceneManagerService.PostProcess.bypassSceneEffects(0)'
```

The `Metering:` line of `getStatus()` reads `ISO … at 1/… s` since 2026-09-26: the auto-exposure is
APERTURE PRIORITY (ISO first, then the shutter once the ISO sits at 100, down to 1/8000 s), and the motion
blur follows the metered speed. `ISO 100 at 1/8000 s` or `ISO 12800` at the authored speed is a SATURATION.

`bypassSceneEffects(1)` (2026-09-26, projet-alpha's **KeyPad4**) is THE "no effect" A/B: every SCENE
slot goes dark — lighting family, clouds, light shafts, fog, custom effects, TAA — while the camera
chain keeps exposing and tone mapping the frame. Nothing selected is written, so `(0)` brings back
exactly what ran; `getStatus()` opens with `Scene effects: BYPASSED` and marks each scene slot
`off (bypassed; selected …)`. ⚠️ Never use `PostProcessor::enable(false)` for that comparison: it
forces the DIRECT path (no scene target, no exposure, no tone mapping) and shows a raw luminance
clipped to white — a renderer diagnostic, not a picture.

`listEffects()` prints every slot, its occupants, and two marks: `>` is what you selected, `*` is
what the **last frame actually ran**. They differ exactly when a fallback is active — which is the
only way to see one, since the fallback is silent by design (a trace would be one line per slot
per frame).

- Slot names are `EffectSlot`'s own (`IndirectDiffuse`, `Reflections`, `AmbientOcclusion`,
  `ContactShadows`, `Fog`, `VolumetricLight`, `Clouds`, `TemporalAA`, `LensFlare`, `Custom`); effect
  names are the labels `listEffects()` prints (`RTGIEffect`, `SSGIEffect`, …).
- `Clouds` (Sep 2026) is SCENE-DRIVEN: its `VolumetricCloudsEffect` appears the first frame the scene
  holds a `Component::CloudVolume` — no application adds it. `disable(Clouds)` / `select(Clouds,
  VolumetricCloudsEffect)` work like any slot. ⚠️ Listed is not drawn: the pass traces a census
  (`Clouds drawn: N of M (…)`) in the LOG when it changes; judge its look at a pinned exposure
  (`Camera.setExposure(...)`).
- `select()` mixes lanes per slot — `RTGI` + `SSR` + `RTAO` is a legal and useful A/B.
- ⚠️ **A selection is applied on the NEXT frame, never immediately.** The console runs on the main
  thread and the chain is walked on the render thread; `syncSlotSelection()` applies it at the
  frame boundary. Take the screenshot after the switch has landed, not in the same breath.
- Every lighting slot has an occupant in **both** lanes since Sep 2026 (`SSContactShadows`
  completed the set), so `setLightingMode()` no longer leaves a slot dark. It still warns, per
  slot, if one ever does.
- **A lane switch moves the LANE, never the set of concepts that are on** (fixed 2026-09-13). A
  concept switched off by `Core/Graphics/PostProcessing/<Concept>/Enabled = false`, or by
  `disable(<slot>)`, stays off across every `setLightingMode()` and every KeyPad9 press; an explicit
  `select(<slot>, <effect>)` switches it on for the session and it follows the lanes from then on.
  `setLightingMode(None)` switches the whole family off and keeps the concepts' switches, so the next
  lane brings back exactly what was on — the one-command "no indirect lighting" control a three-mode
  capture needs. `getStatus()` opens with `Lane selected: RayTracing | ScreenSpace | none` and
  annotates a gated-off concept (`Reflections: off  (concept switched off — …)`) so that "off" is
  never mistaken for a fallback.
- ⚠️ The four camera-owned slots (`DepthOfField`, `MotionBlur`, `Glare`, `ToneMapping`) are
  reported but **not selectable**: the camera owns them, drive them through the camera.
- `getStatus()` ends with the last frame's `Metering:` line and `Overflow census:` block (§ 6,
  "Counting the fp16 overflows") — a copy the render thread published, never a live read.
- ⚠️⚠️ **A lane switch is invisible to a mean-luminance comparison.** Measured on Sponza: the two
  frames differ on 99.7 % of pixels (mean |Δ| 20/255) while the mean luminance moves by 0.01 — the
  auto-exposure absorbs the change. Read the image, or compare per-pixel.

---

## 4. Scene Creation via JSON

Complete scenes can be built from a single JSON description sent via TCP. Lines starting with `{` are automatically routed to the JSON scene handler.

### Sending a JSON scene

```bash
python3 tools/remote-console.py '{"Name":"AIScene","Boundary":1024.0,"Background":{"Type":"SkyBox","Resource":"Miramar","ApplyLighting":true},"Ground":{"Type":"Basic","Material":{"Type":"Basic"}},"Nodes":[{"Name":"Observer","Position":[0.0,5.0,20.0],"LookAt":[0.0,0.0,0.0],"Components":[{"Type":"Camera","Name":"MainCam","Primary":true},{"Type":"Microphone","Name":"MainMic","Primary":true}]}],"StaticEntities":[{"Name":"SponzaBuilding","Position":[0.0,0.0,0.0],"Components":[{"Type":"Visual","Mesh":"Sponza","Scale":0.01}]}]}'
```

### JSON Scene Format Specification

```json
{
  "Name": "SceneName",
  "Boundary": 1024.0,

  "Background": {
    "Type": "SkyBox",
    "Resource": "Miramar",
    "ApplyLighting": true
  },

  "Ground": {
    "Type": "Basic | PerlinNoise | DiamondSquare",
    "GridDivision": 64,
    "UVMultiplier": 1024.0,
    "ShiftHeight": 0.0,
    "Material": {
      "Type": "Basic | Standard | PBR",
      "Resource": "MaterialName"
    },
    "Noise": {
      "Size": 1.0,
      "Factor": 0.5,
      "Roughness": 0.5,
      "Seed": 0
    }
  },

  "Lighting": {
    "Ambient": {
      "Color": [1.0, 1.0, 1.0, 1.0],
      "Intensity": 5000.0
    }
  },

  "Nodes": [
    {
      "Name": "Observer",
      "Position": [0.0, 5.0, 20.0],
      "LookAt": [0.0, 0.0, 0.0],
      "Components": [
        {"Type": "Camera", "Name": "MainCam", "Primary": true},
        {"Type": "Microphone", "Name": "MainMic", "Primary": true}
      ],
      "Nodes": [
        { "...recursive children..." }
      ]
    }
  ],

  "StaticEntities": [
    {
      "Name": "MyMesh",
      "Position": [10.0, 0.0, 5.0],
      "Components": [
        {"Type": "Visual", "Mesh": "MeshResourceName", "Scale": 1.0}
      ]
    }
  ],

  "ExtraData": {
    "custom": "application-specific data"
  }
}
```

### Field Reference

| Section | Field | Type | Description |
|---------|-------|------|-------------|
| **Root** | `Name` | string | Scene name (must be unique) |
| | `Boundary` | float | Half-size of the cubic scene volume |
| **Background** | `Type` | string | `SkyBox` (only supported type currently) |
| | `Resource` | string | SkyBox resource name |
| | `ApplyLighting` | bool | OPT-IN: derive ambient + directional lights from the background photometric manifest (default: false) |
| **Ground** | `Type` | string | `Basic` (flat), `PerlinNoise`, `DiamondSquare` |
| | `GridDivision` | int | Mesh tessellation (default: 64) |
| | `UVMultiplier` | float | Texture repeat factor (default: boundary) |
| | `ShiftHeight` | float | Vertical offset (default: 0.0) |
| | `Material.Type` | string | `Basic` (grey), `Standard`/`PBR` (textured) |
| | `Material.Resource` | string | Material resource name |
| | `Noise.Size` | float | Perlin noise scale (default: 1.0) |
| | `Noise.Factor` | float | Perlin noise amplitude (default: 0.5) |
| | `Noise.Roughness` | float | Diamond-square roughness (default: 0.5) |
| | `Noise.Seed` | int | Diamond-square seed (default: 0) |
| **Lighting** | `Ambient.Color` | [r,g,b,a] | Ambient light colour: a CHROMATICITY since 2026-09-25 (raw components scaled to unit luminance — it never dims the ambient; default: white) |
| | `Ambient.Intensity` | float | Ambient ILLUMINANCE in lux, delivered whatever the hue (default: 100; open shade 20000, overcast 5000, moonlit night ~1) |
| **Nodes** | `Name` | string | Node name (required) |
| | `Position` | [x,y,z] | World-space position |
| | `LookAt` | [x,y,z] | World-space target to look at |
| | `Components` | array | Component list (see below) |
| | `Nodes` | array | Recursive child nodes |
| **Components** | `Type` | string | `Camera`, `Microphone`, or `Visual` |
| | `Name` | string | Component name (auto-generated if omitted) |
| | `Primary` | bool | Mark as primary camera/microphone |
| | `Mesh` | string | Mesh resource name (Visual only) |
| | `Scale` | float | Uniform scale (Visual only, default: 1.0) |
| **StaticEntities** | `Name` | string | Entity name (required) |
| | `Position` | [x,y,z] | World-space position |
| | `Components` | array | Same component format as Nodes |

### Ground Types

| Type | Description | Noise Parameters |
|------|-------------|-----------------|
| `Basic` | Flat plane at Y=0 | None |
| `PerlinNoise` | Perlin noise terrain | `Size`, `Factor` |
| `DiamondSquare` | Diamond-square terrain | `Factor`, `Roughness`, `Seed` |

---

## 5. Camera Control

### Moving the camera

```bash
python3 tools/remote-console.py "Core.SceneManagerService.setNodePosition(Camera, 0.0, 10.0, 20.0)"
```

### Orienting the camera

```bash
python3 tools/remote-console.py "Core.SceneManagerService.setNodeLookAt(Camera, 50.0, 0.0, 50.0)"
```

---

## 6. Visual Verification (AI Feedback Loop)

### Isolating the physical simulation

```bash
python3 tools/remote-console.py "Core.togglePhysicalSimulation()"
# Response: Physical simulation ENABLED. / DISABLED (entities move, nothing collides).
```

Turns `Scene::resolveCollisions()` on and off at runtime — collisions, boundary clipping and
ground response. **It is not a pause**: entities keep moving under their own logic and gravity,
only the resolution stops. Use it to settle "is this a rendering defect or a physics one?" in one
command instead of a rebuild. Enabled by default.

⚠️ A body released with the simulation OFF keeps accelerating downwards for ever — re-enabling it
mid-fall makes it resolve a very deep penetration in one step. Toggle before spawning, not during.

### Taking a screenshot

```bash
python3 tools/remote-console.py "Core.RendererService.screenshot()"
# Response: Screenshot saved: "/path/to/captures/<timestamp>.png"
```

Screenshots are saved to `fileSystem().userDataDirectory("captures")`, i.e. **`<user data dir>/captures/`** — on Linux `~/.local/share/<org>/<app-name>/captures/` (projet-alpha: `LNIsle/projet-alpha`, LycheeSlicer: `Lychee/LycheeSlicer`); macOS and Windows follow the same `<org>/<app-name>` resolution under their user-data root (`src/FileSystem.cpp`). The Unix-timestamp file names make before/after diffs free.

### Dumping a render-to-texture target (probe diagnostics)

```bash
python3 tools/remote-console.py "Core.SceneManagerService.dumpRenderTarget(ProbeCubemap)"
# Response: Render target 'MyProbe_...' dumped: "/path/to/captures/rt-dump-<timestamp>.png"
```

Matches the target by NAME FRAGMENT and writes its color image (layer 0 — the +X face for
a cubemap) as a PNG in the captures directory. This is the ground-truth tool for any
"the reflection looks wrong" investigation: it answers "what did the probe actually bake?"
without inferring it through a curved reflection. A target that never rendered (an unfired
"once" probe, a suspended target) is refused cleanly instead of being read in an undefined
layout.

### Reading per-pass GPU timings (built-in profiler)

The engine has a built-in GPU profiler (Vulkan timestamp queries, `Vulkan::GPUProfiler`).
When enabled, every pass of the main frame command buffer is timed on the GPU and the
results are harvested stall-free (one query pool per frame in flight):

```bash
python3 tools/remote-console.py "Core.RendererService.getGPUTimings()"
python3 tools/remote-console.py "Core.RendererService.getGPUTimings(reset)"   # clear avg/max
```

- **Gated by the settings key `Core/Graphics/GPUProfiler/Enabled` (default `false`)** —
  set it to `true` in `settings.json` (app NOT running) and restart. Zero cost when disabled.
- Output: one line per scope in command-stream order, indentation = nesting, with
  `last` / `avg` (~60-frame rolling) / `max` / `samples` columns, all in milliseconds.
- Scope granularity: the frame root, the TLAS build, the scene pass, the grab pass, the
  post-process chain and inside it **one scope per real pass** — `<Effect>/trace`,
  `SharedDenoise`, `<Effect>/temporal`, `Combine`, plus standalone effects (TAA...) —
  and the final composite. Shared-denoise effects have NO contiguous per-effect total by
  design: their passes are interleaved (this mirrors the actual command stream).
- **Shadow maps and render-to-textures** (2026-09-23): each gets a TOP-LEVEL line
  `ShadowMap/<target id>` / `RenderToTexture/<target id>`, listed before `Frame` — they are
  separate submissions, so they are NOT inside `Frame` or `ScenePass`. A cascaded map nests one
  `Cascade/<n>` line per cascade under its `ShadowMap/<id>` (one render pass each, since 2026-09-24). Needs the device feature
  `hostQueryReset` (every desktop driver and MoltenVK advertise it); without it the profiler's
  startup line says "main command buffer only" and those lines are absent.
- A continuous reflection probe is SUSPENDED while an enabled SSR/RTR is in the stack, so its
  `RenderToTexture/…` line is absent until `PostProcess.disable(Reflections)`: that is the
  reflection cost ladder, not a profiler gap.
- This is the FIRST tool for any "the frame is slow" question — reach for RenderDoc only
  when a single pass needs draw-call-level dissection.

### Counting the fp16 overflows (the overflow census, Sep 2026)

How many texels of the scene radiance an RGBA16F target has turned into **NaN**, **Inf**, or the
largest finite half (**ceiling**, ≥ 65 504), per frame and per image — the instrument of the
scene-colour pre-exposure work (engine `docs/todo/scene-colour-pre-exposure.md`, step B1a;
mechanism: `src/Graphics/AGENTS.md` § "The overflow census").

```bash
python3 tools/remote-console.py "Core.RendererService.testOverflowCensus()"      # positive control, FIRST
python3 tools/remote-console.py "Core.RendererService.setOverflowCensus(1)"      # arm (0 disarms)
python3 tools/remote-console.py "Core.RendererService.resetOverflowCensus()"     # new statistics window
python3 tools/remote-console.py "Core.RendererService.getFrameDiagnostics()"     # JSON, for scripts
python3 tools/remote-console.py "Core.SceneManagerService.PostProcess.getStatus()"  # same data, for a human
```

- **Disarmed by default** (it costs one branch then). Arm it live, or at launch with
  `Core/Graphics/PostProcessing/OverflowCensus/Enabled = true` (read once). Every switch lands on the
  NEXT rendered frame. It counts the HDR chain only (an 8-bit chain cannot overflow).
- ⚠️⚠️ **`testOverflowCensus()` must answer `PASS` on a machine before any count read there means
  anything.** It counts a 16×16 image of raw half bit patterns in the next frame (blocks at most 3 s)
  and prints the expected tuple — tested 256, NaN 21, Inf 12, ceiling 8, peak 65472 — against the
  measured one. `FAIL` = the census is blind on that platform: take no baseline there. It needs a scene
  rendering through the HDR post-process chain (it answers `FAIL: no frame counted…` otherwise).
- Channels: `SceneColour` (the grabbed scene colour the chain starts from), `ToneMapInput` (what the
  tone mapper receives; the chain output with `toneMapped: false` when none ran), `RTGI_Trace` /
  `RTR_Trace` (the raw traces, only on the frames their effect runs), `ProbeIrradiance` (the probe
  atlas interior). ⚠️ **`ToneMapInput` = 0 does not mean the scene colour is clean**: the TAA and SSR
  guards scrub non-finite values first — the gap between the two channels IS those guards.
- `getStatus()` appends a `Metering:` line (auto/manual, scene average in nits, ISO, rejected
  measurements — counted only with a camera; a GROWING count is the corruption fingerprint of
  `ToneMapping::meteredRejectedCount()`) and an `Overflow census:` block (last counted frame, the
  window, one line per channel with `tested/expected`, the classes, the peak, and the window maxima).
- `getFrameDiagnostics()` schema (one line of JSON; a non-finite float is `null`):

```json
{"renderedFrame":18342,
 "metering":{"toneMapper":true,"auto":true,"meteredLuminance":812.4,"meteredSensitivity":3200,"rejected":0},
 "census":{"available":true,"armed":true,"valid":true,"frame":18339,"toneMapped":true,
   "channels":[{"name":"SceneColour","tested":4665600,"expected":4665600,"nan":0,"inf":3912,"ceiling":118,
                "overflow":4030,"peakFinite":65472,"invalid":false}],
   "window":{"startsAfterFrame":17739,"firstFrame":17740,"lastFrame":18339,"frames":600,"framesWithOverflow":12,
             "staleSlots":0,
             "channels":[{"name":"SceneColour","framesCounted":600,"framesOverflowing":12,"maxOverflow":4870,
                          "maxOverflowFrame":18101,"maxPeakFinite":65472}]},
   "selfTest":{"ran":true,"passed":true,"frame":17702,"tested":256,"nan":21,"inf":12,"ceiling":8,"peakFinite":65472}}}
```

- The counts arrive `framesInFlight` frames late (harvested after the slot's fence): `frame` is the
  frame they belong to, `renderedFrame` the last one rendered. `tested != expected` flags a channel
  `invalid` (a census defect, never a property of the scene); `staleSlots` counts batches recorded but
  never executed.
- Cost: read it in `getGPUTimings()`, the `OverflowCensus` line inside `PostFXChain`.

### Triggering a RenderDoc GPU frame capture

For deep GPU analysis (draw calls, bound descriptors, sampled images, pipeline state) the engine
can trigger a [RenderDoc](https://renderdoc.org/) frame capture from the console:

```bash
python3 tools/remote-console.py "Core.RendererService.triggerRenderDocCapture()"   # next frame
python3 tools/remote-console.py "Core.RendererService.triggerRenderDocCapture(5)"   # next 5 frames
```

- The optional argument is the number of consecutive frames to capture (default 1). Capturing a
  frame **past the first** is how per-frame / state-tracking bugs are caught.
- **Requires the app to be launched under RenderDoc** so the in-application API is injected, and a
  build with `EMERAUDE_ENABLE_RENDERDOC=ON` (otherwise the call reports "RenderDoc is not
  available" and is a no-op). The option is self-sufficient — it fetches the RenderDoc sources
  itself (`cmake/SetupRenderDoc.cmake`); there is no submodule to initialise since Sep 2026:
  ```bash
  /opt/renderdoc_<ver>/bin/renderdoccmd capture --wait-for-exit ./<app> --load-demo <demo> --disable-cef
  ```
- The command sets the capture output path to `<user data dir>/RenderDoc/` (same `<org>/<app-name>` resolution as the screenshots above) before
  triggering (RenderDoc is otherwise never told where to write).
- Captures are then analysed with the RenderDoc Python module (see the project's RenderDoc notes).

### The fundamental loop

```
Command --> Screenshot --> Analyze PNG --> Adjust --> Repeat
```

This is the AI's primary mechanism for understanding the 3D world. The AI:
1. Sends a command (create scene, move camera, add object)
2. Takes a screenshot
3. Reads the PNG file (multimodal analysis)
4. Evaluates the result (is the scene correct? is the camera oriented properly?)
5. Sends corrective commands if needed
6. Repeats until the desired composition is achieved

### Verification workflow for new scenes

1. **Create scene** with camera at Y=-2 (below ground)
2. **Add ground** with `setGround(default)`
3. **Screenshot** -- read the PNG -- confirm grey ground is visible above camera
4. **Reposition camera** above ground
5. **Orient camera** to look at scene content
6. **Screenshot** -- verify the view
7. **Iterate** until the desired composition is achieved

---

## 7. Node System

Everything in a scene is attached to **nodes**. A node is a positioned container. Components (camera, microphone, visuals) are attached to nodes.

### Creating nodes

```bash
python3 tools/remote-console.py "Core.SceneManagerService.createNode(MyObject, 5.0, 1.0, 5.0)"
```

### Attaching components

```bash
# Camera + microphone (creates a viewpoint)
python3 tools/remote-console.py "Core.SceneManagerService.attachCamera(MyNode, MyCamera)"
python3 tools/remote-console.py "Core.SceneManagerService.attachMicrophone(MyNode, MyMic)"
```

### Inspecting nodes

```bash
python3 tools/remote-console.py "Core.SceneManagerService.getNode(MyNode)"
# Returns JSON: {"name":"MyNode","address":"0x...","position":[5,1,5],"childCount":0}
```

### Manipulating nodes

```bash
python3 tools/remote-console.py "Core.SceneManagerService.setNodePosition(MyNode, 10.0, 0.0, 10.0)"
python3 tools/remote-console.py "Core.SceneManagerService.setNodeLookAt(MyNode, 0.0, 0.0, 0.0)"
python3 tools/remote-console.py "Core.SceneManagerService.destroyNode(MyNode)"
```

---

## 8. Audio Control

### Music playback

```bash
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.play()"       # Play/resume
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.pause()"      # Pause
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.stop()"       # Stop
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.next()"       # Next track
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.previous()"   # Previous track
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.volume(50)"   # Volume 0-100
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.seek(30.0)"   # Seek to 30s
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.shuffle(on)"  # Shuffle on/off
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.loop(on)"     # Loop on/off
```

### Playlist management

```bash
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlist()"           # List tracks
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlistPlay(3)"      # Play track #3
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlistClear()"      # Clear playlist
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.status()"             # Full status
```

---

## 9. Engine Configuration

### Reading settings (JSON)

```bash
python3 tools/remote-console.py "Core.SettingsService.getJson()"
python3 tools/remote-console.py "Core.ArgumentsService.getJson()"
python3 tools/remote-console.py "Core.FileSystemService.getJson()"
```

### Modifying settings

```bash
python3 tools/remote-console.py "Core.SettingsService.set(Core/Video/Window/Width, 1920)"
python3 tools/remote-console.py "Core.SettingsService.set(Core/Video/Window/Height, 1080)"
python3 tools/remote-console.py "Core.SettingsService.save()"
```

### Window control

```bash
python3 tools/remote-console.py "Core.WindowService.resize(1920, 1080)"
python3 tools/remote-console.py "Core.WindowService.getState()"
```

---

## 10. Engine Lifecycle

### Graceful shutdown

```bash
python3 tools/remote-console.py "quit"
# Settings are saved automatically if SavePropertiesAtExit is true
```

### Renderer information

```bash
python3 tools/remote-console.py "Core.RendererService.getStatus()"
```

---

## 11. Complete Example: AI Creates a Scene from Scratch

```bash
#!/bin/bash
# AI creates a complete 3D scene from scratch

PORT=7777
# Every echo opens its own connection here: fine for stateless commands. Use pipe mode (printf ... |)
# for a sequence that relies on targetActiveScene()/targetNode().
CMD="python3 tools/remote-console.py --port $PORT"

# Step 1: Discover available resources
echo "Core.ResourcesManagerService.listContainers()" | $CMD
echo "Core.ResourcesManagerService.listResources(SkyBoxResource)" | $CMD
echo "Core.ResourcesManagerService.listResources(MeshResource)" | $CMD

# Step 2: Create scene with skybox, camera below ground for verification
echo "Core.SceneManagerService.createScene(AIScene, 1024.0, Observer, 0.0, -2.0, 0.0, Miramar)" | $CMD

# Step 3: Add ground
echo "Core.SceneManagerService.setGround(default)" | $CMD

# Step 4: Verify ground exists (screenshot from below)
echo "Core.RendererService.screenshot()" | $CMD
# --> Read PNG: should see grey ground above, skybox below

# Step 5: Move camera above ground for normal view
echo "Core.SceneManagerService.setNodePosition(Observer, 0.0, 10.0, 30.0)" | $CMD
echo "Core.SceneManagerService.setNodeLookAt(Observer, 0.0, 0.0, 0.0)" | $CMD

# Step 6: Verify camera angle
echo "Core.RendererService.screenshot()" | $CMD
# --> Read PNG: should see sky on top, ground on bottom

# Step 7: Add a 3D mesh
echo "Core.SceneManagerService.addMesh(Sponza, SponzaBuilding, 0.0, 0.0, 0.0, 0.01)" | $CMD

# Step 8: Final screenshot
echo "Core.RendererService.screenshot()" | $CMD

# Step 9: Play some music
echo "Core.AudioManagerService.TrackMixerService.play()" | $CMD

# When done
echo "quit" | $CMD
```

### Alternative: JSON scene (single command)

```bash
python3 tools/remote-console.py '{"Name":"AIScene","Boundary":512.0,"Background":{"Type":"SkyBox","Resource":"Miramar","ApplyLighting":true},"Ground":{"Type":"Basic"},"Nodes":[{"Name":"Observer","Position":[0.0,10.0,30.0],"LookAt":[0.0,0.0,0.0],"Components":[{"Type":"Camera","Primary":true},{"Type":"Microphone","Primary":true}]}]}'
```

---

## 12. Service Command Reference

### Discovering commands at runtime

```bash
python3 tools/remote-console.py "listObjects"              # List top-level objects
python3 tools/remote-console.py "Core.lsobj()"              # List Core's children
python3 tools/remote-console.py "Core.RendererService.lsfunc()"  # List service commands
```

### Complete command table

| Service | Command | Description |
|---------|---------|-------------|
| **ResourcesManagerService** | `listContainers()` | JSON array of all resource containers |
| | `listResources(containerNameOrId)` | JSON array of available resource names |
| **SceneManagerService** | `createScene(name, boundary, camNode, x, y, z [, bg [, ground]])` | Create full scene |
| | `setGround([material])` | Add/replace ground (default: "default") |
| | `setBackground(skyboxName [, applyLighting])` | Set skybox on active scene; `true` as second argument derives the scene lighting (ambient + stars) from the sky's photometric manifest |
| | `addMesh(meshResource, entityName, x, y, z [, scale])` | Place a 3D mesh |
| | `createNode(name [, x, y, z])` | Create node in active scene |
| | `destroyNode(name)` | Remove node |
| | `setNodePosition(name, x, y, z)` | Move node |
| | `setNodeLookAt(name, x, y, z)` | Orient node to look at point |
| | `getNode(name)` | Inspect node (JSON) |
| | `attachCamera(node, camName)` | Attach primary camera |
| | `attachMicrophone(node, micName)` | Attach primary microphone |
| | `getSceneInfo()` | Active scene summary |
| | `writeImposterAtlases()` | Writes the albedo atlas (premultiplied, mip 0) of every octahedral imposter the active scene baked to the captures directory, and says how many bakes are still queued |
| | `getStateSyncStatistics(reset)` | How often a rendered frame read a logic state the logic thread was rewriting — frames measured, **overwritten frames (must be 0)**, logic publications inside a frame, latch-to-end duration. `true` resets the window after reading: call it once to open a window, wait, call again. Measured before the 2026-09-24 triple-buffer fix: 41-66 % on `terrain`. A non-zero value means objects slide on each other while the camera turns |
| | `getRenderStatistics()` | What the last frame's render lists submit, per geometry LOD: batches, instances, triangles — view lists, then shadow lists (summed over the shadow targets). The answer to "is the LOD used, and where do the triangles go" without a GPU capture |
| | `listScenes()` | List all scenes |
| | `listNodes()` | List nodes (target scene first) |
| | `listStaticEntities()` | List static entities (target scene first) |
| | `targetActiveScene()` | Target the active scene for inspection |
| | `targetScene(name)` | Target a named scene |
| | `targetNode(name)` | Target a node for inspection |
| | `targetStaticEntity(name)` | Target a static entity |
| | `enableScene(name)` | Enable a scene |
| | `deleteScene(name)` | Delete a scene |
| | `getActiveSceneName()` | Get active scene name |
| | `moveNodeTo(x, y, z)` | Move targeted node |
| | *(JSON input)* | Send `{...}` to create scene from JSON |
| **RendererService** | `screenshot()` | Capture framebuffer to PNG |
| | `getStatus()` | FPS, frame time, resolution |
| | `setOverflowCensus(1\|0)` | Arm / disarm the overflow census (NaN / Inf / fp16-ceiling counts) |
| | `resetOverflowCensus()` | Open a new census statistics window |
| | `testOverflowCensus()` | Census positive control: `PASS` / `FAIL` with both tuples |
| | `getFrameDiagnostics()` | Last frame's metering + overflow census (JSON) |
| **WindowService** | `resize(w, h)` | Resize window |
| | `getState()` | Window state (JSON) |
| **SettingsService** | `getJson()` | All settings (JSON) |
| | `set(key, value)` | Modify setting |
| | `save()` | Save to disk |
| | `print()` | Text dump |
| **FileSystemService** | `getJson()` | All paths (JSON) |
| | `get(name)` | Specific path |
| | `print()` | Text dump |
| **ArgumentsService** | `getJson()` | All arguments (JSON) |
| | `get(index)` | Argument by index |
| | `print()` | Text dump |
| **TrackMixerService** | `play([song])` | Play/resume |
| | `pause()` | Pause |
| | `stop()` | Stop |
| | `volume(0-100)` | Set volume |
| | `next()` / `previous()` | Navigate playlist |
| | `playlist()` / `playlistClear()` / `playlistAdd(name)` / `playlistPlay(N)` | List / manage the playlist (split 2026-09-27: one tool per action) |
| | `seek(seconds)` | Seek position |
| | `shuffle(on/off)` | Toggle shuffle |
| | `loop(on/off)` | Toggle loop |
| | `crossfade(on/off)` | Toggle crossfade |
| | `status()` | Full mixer state |
| **Built-in** | `exit` / `quit` / `shutdown` | Graceful shutdown |
| | `hardExit` | Immediate shutdown |
| | `help` / `lsfunc()` | List commands |
| | `listObjects` / `lsobj()` | List services |
| | `describeCommands()` | Every command as JSON (parameters, types, arity, defaults, hints) |

### Service hierarchy

```
Core
+-- ArgumentsService        (getJson, get, print)
+-- AudioManagerService
|   +-- TrackMixerService   (play, pause, stop, volume, playlist, etc.)
+-- FileSystemService       (getJson, get, print)
+-- InputManagerService     (keyPress, mouseClick, mousePress, mouseRelease, mouseMove, pointerState)
+-- RendererService         (screenshot, getStatus)
+-- ResourcesManagerService (listContainers, listResources)
+-- SceneManagerService     (createScene, setGround, setBackground, addMesh, etc.)
+-- SettingsService         (getJson, set, save, print)
+-- WindowService           (resize, getState)
```

---

## 13. Input Injection (AI Interaction)

> **This is critical for AI autonomy.** The AI can simulate keyboard and mouse events, enabling full interaction with the running application without a physical user.

### Keyboard events

```bash
# Inject a key press + release. Args: key_code, modifiers (optional)
python3 tools/remote-console.py "Core.InputManagerService.keyPress(292, 1)"
# 292 = F3, 1 = Shift → Shift+F3
```

Key codes follow GLFW constants (see `Input/Types.hpp`). Common keys:
- F1-F12: 290-301
- Escape: 256, Enter: 257, Space: 32
- Letters: ASCII values (A=65, G=71, R=82, S=83)

Modifier flags: Shift=1, Ctrl=2, Alt=4, Super=8

⚠️ Validated since 2026-09-27: a key code outside 32-348 (`GLFW_KEY_SPACE`..`GLFW_KEY_LAST`), a mouse
button outside 0-7 or a modifier mask outside 0-63 is refused with an error — the listeners index
per-key / per-button state with these values, and the command used to hand them over unchecked.

### Mouse events

```bash
# Click at screen coordinates. Args: x, y, button (0=left), modifiers
python3 tools/remote-console.py "Core.InputManagerService.mouseClick(1920, 1000)"

# Move pointer to coordinates
python3 tools/remote-console.py "Core.InputManagerService.mouseMove(500, 300)"
```

**Coordinate space:** injected coordinates are dispatched to listeners **without pointer scaling** — give them in **physical framebuffer pixels** (`framebufferWidth`/`framebufferHeight` from `Core.WindowService.getState()`). All pointer consumers (overlay hit-testing, scene editor picking) work in that space; real cursor events are scaled to it by `Core::updatePointerScaling()` on Wayland/macOS.

### The AI interaction loop

```
Screenshot → Analyze image → Decide action → Inject input → Screenshot → Verify
```

The AI can:
1. **See** the application state via `screenshot()`
2. **Act** on it via `keyPress()` / `mouseClick()` / `mouseMove()`
3. **Verify** the result via another `screenshot()`
4. **Navigate** the camera via scene commands (`Act.setPosition`, `Act.lookAt`)

This enables fully autonomous testing, debugging, and scene editing.

---

## 14. Clean Shutdown

```bash
# Graceful quit (Shift+Escape)
python3 tools/remote-console.py "Core.InputManagerService.keyPress(256, 1)"
```

Prefer this over `kill` or `timeout` — it lets the engine clean up resources properly (GPU, audio, files).

---

<!-- NOTE: CEF integration is handled at the application level (projet-alpha), not in the engine. -->

## Downloads (`Core.NetManagerService`) and on-demand resource loading

The engine downloads `https://` files through `Net::Manager` (emeraude-base `HTTPSClient`, TLS
verified against the system trust store) into a URL-keyed cache. Settings: `Core/Net/DownloadEnabled`
(default `true`), `Core/Net/CABundleFile` (default empty). Completion is reported on the next
main-loop cycle — poll `status(ticket)`.

```bash
python3 tools/remote-console.py "Core.NetManagerService.isEnabled()"
python3 tools/remote-console.py "Core.NetManagerService.download(https://raw.githubusercontent.com/EmeraudeEngine/emeraude-base/main/README.md)"
#   -> Ticket #1 (Transferring). Poll with status(1).
python3 tools/remote-console.py "Core.NetManagerService.status(1)"
#   -> {"ticket":1,"status":"Done","filepath":"~/.cache/<app>/downloads/<hash>.md","bytesReceived":13566,"bytesTotal":13566,"remaining":0}
#      (while Transferring: bytesReceived grows, bytesTotal is the Content-Length or 0 when unknown)
python3 tools/remote-console.py "Core.NetManagerService.listCache()"
python3 tools/remote-console.py "Core.NetManagerService.clearCache()"
```

A resource declared with `"Source": "ExternalData"` in a store is downloaded the same way, then
loaded. To exercise the chain from the console, drop a store and request the resource by name:

```bash
cat > /tmp/store.json <<'JSON'
{"Stores":{"Images":[{"Name":"RemotePicture","Source":"ExternalData","Data":"https://raw.githubusercontent.com/pnggroup/libpng/libpng16/contrib/pngsuite/basn6a08.png"}]}}
JSON
python3 tools/remote-console.py 'Core.openFiles("/tmp/store.json")'
python3 tools/remote-console.py "Core.ResourcesManagerService.loadResource(ImageResource, RemotePicture)"
python3 tools/remote-console.py "Core.ResourcesManagerService.resourceStatus(ImageResource, RemotePicture)"
#   -> Loading, then Loaded (or Failed: the default resource is served — the fail-safe contract)
```

`http://` URLs are refused (HTTPS only, by decision). A refused download fails the resource
immediately; a certificate error fails it after the handshake. Both leave nothing in the cache.

