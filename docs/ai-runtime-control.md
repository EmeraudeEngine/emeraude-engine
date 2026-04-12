# AI Runtime Control Guide

## Purpose

This document is **THE reference** for any AI agent controlling the Emeraude-Engine at runtime via the Remote Console (TCP). It covers connection, resource discovery, scene creation (live commands and JSON), spatial orientation, visual verification, audio control, and the complete command reference.

The AI connects to a running engine instance via TCP and sends commands to discover resources, create scenes, place cameras, add objects, take screenshots, play music, modify settings, and verify results visually.

## Connection

The engine listens on **TCP port 7777** (configurable via `Core/Console/RemoteListenerPort`).

**Cross-platform (Python, recommended — required on Windows where `nc` is not available):**
```bash
# Send a single command
python3 tools/remote-console.py "command"

# Interactive mode (REPL)
python3 tools/remote-console.py
```

> **Note:** Always use `python3`, not `python`. On many systems (Linux, Windows), `python` may not exist or may point to Python 2. The script requires Python 3.

**Linux/macOS only (nc / netcat):**

`nc` behavior differs between Linux and macOS:
- **Linux**: Use `-q <seconds>` — closes the connection after EOF + timeout
- **macOS**: Use `-w <seconds>` — macOS `nc` (BSD netcat) does **not** support `-q`
- **Windows**: `nc` is not available — use the Python client above

```bash
# Linux — send a single command
echo "command" | nc -q 1 localhost 7777

# macOS — send a single command
echo "command" | nc -w 1 localhost 7777

# Send multiple commands in sequence (both platforms, no flag needed)
(
echo "command1"
sleep 1
echo "command2"
sleep 2
) | nc localhost 7777
```

**For examples in this document**, all `nc` commands use the Linux `-q` flag. On macOS, replace `-q` with `-w`.

**Responses are clean** -- no log prefixes, no ANSI codes. Just the command output, ready for parsing. Each response is sent directly to the requesting client (not broadcast).

---

## 1. Resource Discovery

Before creating a scene, the AI must know what resources are available. The engine provides two discovery commands.

### List all resource containers

```bash
echo "Core.ResourcesManagerService.listContainers()" | nc -q 2 localhost 7777
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
echo "Core.ResourcesManagerService.listResources(SkyBoxResource)" | nc -q 2 localhost 7777
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
1. Create scene with camera at Y=-2     --> verify ground exists (screenshot)
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
echo "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, 2.0, 0.0, Miramar)" | nc -q 3 localhost 7777
```

Parameters: `createScene(name, boundary, cameraNodeName, camX, camY, camZ [, skyboxName [, groundMaterial]])`

### Scene with ground

The ground can be added inline or after scene creation:

```bash
# Option 1: Inline ground material (8th parameter)
echo "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar, default)" | nc -q 3 localhost 7777

# Option 2: Add ground AFTER scene creation (recommended for flexibility)
echo "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar)" | nc -q 3 localhost 7777
echo "Core.SceneManagerService.setGround(default)" | nc -q 2 localhost 7777
```

Ground materials:
- `default` -- flat grey surface (BasicResource)
- Any material name -- PBR textured surface (StandardResource)

### Scene with specific background

```bash
echo "Core.SceneManagerService.setBackground(Miramar)" | nc -q 1 localhost 7777
```

Available skyboxes depend on the application's resource store. Use `listResources(SkyBoxResource)` to discover them.

### Lighting

`createScene` automatically adds static directional lighting (ambient blue + warm directional light). No manual light setup needed for basic scenes.

### Adding 3D objects

```bash
# addMesh(meshResource, entityName, x, y, z [, scale])
echo "Core.SceneManagerService.addMesh(Sponza, SponzaEntity, 0.0, 0.0, 0.0, 0.01)" | nc -q 5 localhost 7777
```

Use `listResources(MeshResource)` to discover available meshes.

### Critical rules for scene creation

1. **Camera is created BEFORE `enableScene()`** -- this prevents the engine from creating a default camera that overrides yours
2. **Camera Y=-2 is the verification position** -- below ground, you see the grey ground above you
3. **Camera Y=0 won't see the ground** -- same level as ground plane
4. **Lighting is automatic** -- `createScene` adds static directional lighting
5. **Always verify with screenshot** -- take a screenshot and read the PNG to confirm visual output

---

## 4. Scene Creation via JSON

Complete scenes can be built from a single JSON description sent via TCP. Lines starting with `{` are automatically routed to the JSON scene handler.

### Sending a JSON scene

```bash
echo '{"Name":"AIScene","Boundary":1024.0,"Background":{"Type":"SkyBox","Resource":"Miramar"},"Ground":{"Type":"Basic","Material":{"Type":"Basic"}},"Lighting":{"Type":"Static","Ambient":{"Color":[0.3,0.4,0.6,1.0],"Intensity":0.25},"Light":{"Color":[1.0,0.95,0.8,1.0],"Intensity":1.2},"Direction":[1.0,1.0,1.0]},"Nodes":[{"Name":"Observer","Position":[0.0,5.0,20.0],"LookAt":[0.0,0.0,0.0],"Components":[{"Type":"Camera","Name":"MainCam","Primary":true},{"Type":"Microphone","Name":"MainMic","Primary":true}]}],"StaticEntities":[{"Name":"SponzaBuilding","Position":[0.0,0.0,0.0],"Components":[{"Type":"Visual","Mesh":"Sponza","Scale":0.01}]}]}' | nc -q 5 localhost 7777
```

### JSON Scene Format Specification

```json
{
  "Name": "SceneName",
  "Boundary": 1024.0,

  "Background": {
    "Type": "SkyBox",
    "Resource": "Miramar"
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
    "Type": "Static",
    "Ambient": {
      "Color": [0.3, 0.4, 0.6, 1.0],
      "Intensity": 0.25
    },
    "Light": {
      "Color": [1.0, 0.95, 0.8, 1.0],
      "Intensity": 1.2
    },
    "Direction": [1.0, 1.0, 1.0]
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
| **Lighting** | `Type` | string | `Static` (only supported type currently) |
| | `Ambient.Color` | [r,g,b,a] | Ambient light color (default: blue-ish) |
| | `Ambient.Intensity` | float | Ambient intensity (default: 0.25) |
| | `Light.Color` | [r,g,b,a] | Directional light color (default: warm white) |
| | `Light.Intensity` | float | Directional intensity (default: 1.2) |
| | `Direction` | [x,y,z] | Light direction vector |
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
echo "Core.SceneManagerService.setNodePosition(Camera, 0.0, 10.0, 20.0)" | nc -q 1 localhost 7777
```

### Orienting the camera

```bash
echo "Core.SceneManagerService.setNodeLookAt(Camera, 50.0, 0.0, 50.0)" | nc -q 1 localhost 7777
```

---

## 6. Visual Verification (AI Feedback Loop)

### Taking a screenshot

```bash
echo "Core.RendererService.screenshot()" | nc -q 2 localhost 7777
# Response: Screenshot saved: "/path/to/captures/<timestamp>.png"
```

Screenshots are saved to: `~/.local/share/LNIsle/<app-name>/captures/`

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
echo "Core.SceneManagerService.createNode(MyObject, 5.0, 1.0, 5.0)" | nc -q 1 localhost 7777
```

### Attaching components

```bash
# Camera + microphone (creates a viewpoint)
echo "Core.SceneManagerService.attachCamera(MyNode, MyCamera)" | nc -q 1 localhost 7777
echo "Core.SceneManagerService.attachMicrophone(MyNode, MyMic)" | nc -q 1 localhost 7777
```

### Inspecting nodes

```bash
echo "Core.SceneManagerService.getNode(MyNode)" | nc -q 1 localhost 7777
# Returns JSON: {"name":"MyNode","address":"0x...","position":[5,1,5],"childCount":0}
```

### Manipulating nodes

```bash
echo "Core.SceneManagerService.setNodePosition(MyNode, 10.0, 0.0, 10.0)" | nc -q 1 localhost 7777
echo "Core.SceneManagerService.setNodeLookAt(MyNode, 0.0, 0.0, 0.0)" | nc -q 1 localhost 7777
echo "Core.SceneManagerService.destroyNode(MyNode)" | nc -q 1 localhost 7777
```

---

## 8. Audio Control

### Music playback

```bash
echo "Core.AudioManagerService.TrackMixerService.play()" | nc -q 1 localhost 7777       # Play/resume
echo "Core.AudioManagerService.TrackMixerService.pause()" | nc -q 1 localhost 7777      # Pause
echo "Core.AudioManagerService.TrackMixerService.stop()" | nc -q 1 localhost 7777       # Stop
echo "Core.AudioManagerService.TrackMixerService.next()" | nc -q 1 localhost 7777       # Next track
echo "Core.AudioManagerService.TrackMixerService.previous()" | nc -q 1 localhost 7777   # Previous track
echo "Core.AudioManagerService.TrackMixerService.volume(50)" | nc -q 1 localhost 7777   # Volume 0-100
echo "Core.AudioManagerService.TrackMixerService.seek(30.0)" | nc -q 1 localhost 7777   # Seek to 30s
echo "Core.AudioManagerService.TrackMixerService.shuffle(on)" | nc -q 1 localhost 7777  # Shuffle on/off
echo "Core.AudioManagerService.TrackMixerService.loop(on)" | nc -q 1 localhost 7777     # Loop on/off
```

### Playlist management

```bash
echo "Core.AudioManagerService.TrackMixerService.playlist()" | nc -q 1 localhost 7777           # List tracks
echo "Core.AudioManagerService.TrackMixerService.playlist(play, 3)" | nc -q 1 localhost 7777    # Play track #3
echo "Core.AudioManagerService.TrackMixerService.playlist(clear)" | nc -q 1 localhost 7777      # Clear playlist
echo "Core.AudioManagerService.TrackMixerService.status()" | nc -q 1 localhost 7777             # Full status
```

---

## 9. Engine Configuration

### Reading settings (JSON)

```bash
echo "Core.SettingsService.getJson()" | nc -q 2 localhost 7777
echo "Core.ArgumentsService.getJson()" | nc -q 2 localhost 7777
echo "Core.FileSystemService.getJson()" | nc -q 2 localhost 7777
```

### Modifying settings

```bash
echo "Core.SettingsService.set(Core/Video/Window/Width, 1920)" | nc -q 1 localhost 7777
echo "Core.SettingsService.set(Core/Video/Window/Height, 1080)" | nc -q 1 localhost 7777
echo "Core.SettingsService.save()" | nc -q 1 localhost 7777
```

### Window control

```bash
echo "Core.WindowService.resize(1920, 1080)" | nc -q 1 localhost 7777
echo "Core.WindowService.getState()" | nc -q 1 localhost 7777
```

---

## 10. Engine Lifecycle

### Graceful shutdown

```bash
echo "quit" | nc -q 1 localhost 7777
# Settings are saved automatically if SavePropertiesAtExit is true
```

### Renderer information

```bash
echo "Core.RendererService.getStatus()" | nc -q 1 localhost 7777
```

---

## 11. Complete Example: AI Creates a Scene from Scratch

```bash
#!/bin/bash
# AI creates a complete 3D scene from scratch

PORT=7777
CMD="nc -q 2 localhost $PORT"

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
echo "quit" | nc -q 1 localhost $PORT
```

### Alternative: JSON scene (single command)

```bash
echo '{"Name":"AIScene","Boundary":512.0,"Background":{"Type":"SkyBox","Resource":"Miramar"},"Ground":{"Type":"Basic"},"Lighting":{"Type":"Static"},"Nodes":[{"Name":"Observer","Position":[0.0,10.0,30.0],"LookAt":[0.0,0.0,0.0],"Components":[{"Type":"Camera","Primary":true},{"Type":"Microphone","Primary":true}]}]}' | nc -q 5 localhost 7777
```

---

## 12. Service Command Reference

### Discovering commands at runtime

```bash
echo "listObjects" | nc -q 1 localhost 7777              # List top-level objects
echo "Core.lsobj()" | nc -q 1 localhost 7777              # List Core's children
echo "Core.RendererService.lsfunc()" | nc -q 1 localhost 7777  # List service commands
```

### Complete command table

| Service | Command | Description |
|---------|---------|-------------|
| **ResourcesManagerService** | `listContainers()` | JSON array of all resource containers |
| | `listResources(containerNameOrId)` | JSON array of available resource names |
| **SceneManagerService** | `createScene(name, boundary, camNode, x, y, z [, bg [, ground]])` | Create full scene |
| | `setGround([material])` | Add/replace ground (default: "default") |
| | `setBackground(skyboxName)` | Set skybox on active scene |
| | `addMesh(meshResource, entityName, x, y, z [, scale])` | Place a 3D mesh |
| | `createNode(name [, x, y, z])` | Create node in active scene |
| | `destroyNode(name)` | Remove node |
| | `setNodePosition(name, x, y, z)` | Move node |
| | `setNodeLookAt(name, x, y, z)` | Orient node to look at point |
| | `getNode(name)` | Inspect node (JSON) |
| | `attachCamera(node, camName)` | Attach primary camera |
| | `attachMicrophone(node, micName)` | Attach primary microphone |
| | `getSceneInfo()` | Active scene summary |
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
| | `playlist([clear\|play,N\|add,name])` | Manage playlist |
| | `seek(seconds)` | Seek position |
| | `shuffle(on/off)` | Toggle shuffle |
| | `loop(on/off)` | Toggle loop |
| | `crossfade(on/off)` | Toggle crossfade |
| | `status()` | Full mixer state |
| **Built-in** | `exit` / `quit` / `shutdown` | Graceful shutdown |
| | `hardExit` | Immediate shutdown |
| | `help` / `lsfunc()` | List commands |
| | `listObjects` / `lsobj()` | List services |

### Service hierarchy

```
Core
+-- ArgumentsService        (getJson, get, print)
+-- AudioManagerService
|   +-- TrackMixerService   (play, pause, stop, volume, playlist, etc.)
+-- FileSystemService       (getJson, get, print)
+-- InputManagerService     (keyPress, mouseClick, mouseMove)
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
echo "Core.InputManagerService.keyPress(292, 1)" | nc -q 1 localhost 7777
# 292 = F3, 1 = Shift → Shift+F3
```

Key codes follow GLFW constants (see `Input/Types.hpp`). Common keys:
- F1-F12: 290-301
- Escape: 256, Enter: 257, Space: 32
- Letters: ASCII values (A=65, G=71, R=82, S=83)

Modifier flags: Shift=1, Ctrl=2, Alt=4, Super=8

### Mouse events

```bash
# Click at screen coordinates. Args: x, y, button (0=left), modifiers
echo "Core.InputManagerService.mouseClick(1920, 1000)" | nc -q 1 localhost 7777

# Move pointer to coordinates
echo "Core.InputManagerService.mouseMove(500, 300)" | nc -q 1 localhost 7777
```

**Coordinate space:** query `Core.WindowService.getState()` to get `windowWidth`, `framebufferWidth`, and `contentXScale`. GLFW mouse coordinates may differ from framebuffer pixels on HiDPI displays.

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
echo "Core.InputManagerService.keyPress(256, 1)" | nc -q 1 localhost 7777
```

Prefer this over `kill` or `timeout` — it lets the engine clean up resources properly (GPU, audio, files).

---

<!-- NOTE: CEF integration is handled at the application level (projet-alpha), not in the engine. -->
