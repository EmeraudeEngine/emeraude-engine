# AI Runtime Control Guide

## Purpose

This document describes how an **AI agent controls the Emeraude-Engine at runtime** via the Remote Console (TCP). This is not about developing the engine — it's about **using it** as an autonomous tool for 3D scene creation, inspection, and manipulation.

The AI connects to a running engine instance via TCP and sends commands to create scenes, place cameras, add objects, take screenshots, play music, modify settings, and verify results visually.

## Connection

The engine listens on **TCP port 7777** (configurable via `Core/Console/RemoteListenerPort`).

```bash
# Send a single command
echo "command" | nc -q 1 localhost 7777

# Send multiple commands in sequence
(
echo "command1"
sleep 1
echo "command2"
sleep 2
) | nc localhost 7777
```

**Responses are clean** — no log prefixes, no ANSI codes. Just the command output, ready for parsing.

---

## 1. Spatial Orientation for AI Agents

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
- If your camera is at **Y=-2**, you are 2 units below the ground — you will see the ground **above you** as a grey surface.
- If your camera is at **Y=0**, you are **on** the ground — you cannot see it (you're inside the plane).

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
┌─────────────────────────┐
│                         │  ← Sky (skybox, clouds)
│       SKY               │
│                         │
├─────────────────────────┤  ← Horizon line
│                         │
│       GROUND            │  ← Ground surface (grey/textured)
│                         │
└─────────────────────────┘
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

> **Known issue:** The `lookAt` function's direction mapping may be inverted in some cases. Always verify with a screenshot after changing orientation.

### AI workflow for spatial awareness

```
1. Create scene with camera at Y=-2     → verify ground exists (screenshot)
2. Move camera to Y=10, Z=20            → above ground, behind origin
3. LookAt (0, 0, 0)                     → aim at origin (ground level)
4. Screenshot                            → analyze: do I see sky+ground?
5. If only sky → orientation is wrong   → try different lookAt target
6. If sky+ground → success              → adjust framing as needed
7. Repeat 3-6 until desired view
```

**The key insight:** you cannot "see" the 3D world. You must take screenshots and analyze the images to understand what the camera sees. This is your visual feedback loop. Use it after every camera change.

---



### Minimal scene (skybox only)

```bash
echo "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, 2.0, 0.0, Miramar)" | nc -q 3 localhost 7777
```

Parameters: `createScene(name, boundary, cameraNodeName, camX, camY, camZ [, skyboxName [, groundMaterial]])`

### Scene with ground

The ground **must** be added after scene creation:

```bash
echo "Core.SceneManagerService.createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar)" | nc -q 3 localhost 7777
echo "Core.SceneManagerService.setGround(default)" | nc -q 2 localhost 7777
```

Ground materials:
- `default` — flat grey surface (BasicResource)
- Any material name — PBR textured surface (StandardResource)

### Scene with specific background

```bash
echo "Core.SceneManagerService.setBackground(Miramar)" | nc -q 1 localhost 7777
```

Available skyboxes depend on the application's resource store.

### Lighting

`createScene` automatically adds static directional lighting (ambient blue + warm directional light). No manual light setup needed for basic scenes.

---

## 2. Camera Control

### Understanding the coordinate system

- **Y axis is UP** — ground is at Y=0
- Camera at **Y > 0** is above ground
- Camera at **Y < 0** is below ground (sees ground above)
- Camera at **Y = 0** is at ground level (ground not visible)

### Initial camera placement

For **verifying the ground exists**, place the camera below ground:
```bash
# Camera at Y=-2, looking up → ground visible as grey surface above
createScene(MyScene, 1024.0, Camera, 0.0, -2.0, 0.0, Miramar)
```

For **normal viewing**, place camera above ground:
```bash
# Camera at Y=10, above ground
createScene(MyScene, 1024.0, Camera, 0.0, 10.0, 20.0, Miramar)
```

### Moving the camera

```bash
echo "Core.SceneManagerService.setNodePosition(Camera, 0.0, 10.0, 20.0)" | nc -q 1 localhost 7777
```

### Orienting the camera

```bash
echo "Core.SceneManagerService.setNodeLookAt(Camera, 50.0, 0.0, 50.0)" | nc -q 1 localhost 7777
```

> **Note:** The `lookAt` function has a known orientation convention issue. Position changes work reliably. Orientation changes work but the direction mapping may need calibration. Always verify with a screenshot.

---

## 3. Visual Verification

### Taking a screenshot

```bash
echo "Core.RendererService.screenshot()" | nc -q 2 localhost 7777
# Response: Screenshot saved: "/path/to/captures/<timestamp>.png"
```

Screenshots are saved to: `~/.local/share/LNIsle/<app-name>/captures/`

### Verification workflow

1. **Create scene** with camera at Y=-2 (below ground)
2. **Add ground** with `setGround(default)`
3. **Screenshot** → read the PNG → confirm grey ground is visible above camera
4. **Reposition camera** above ground
5. **Screenshot** → verify the view
6. **Iterate** until the desired composition is achieved

This is the fundamental loop: **command → screenshot → analyze → adjust**.

---

## 4. Node System

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

## 5. Audio Control

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

## 6. Engine Configuration

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

## 7. Engine Lifecycle

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

## 8. Complete Example: AI Creates a Scene

```bash
#!/bin/bash
# AI creates a complete 3D scene from scratch

PORT=7777
CMD="nc -q 2 localhost $PORT"

# Create scene with skybox, camera below ground for verification
echo "Core.SceneManagerService.createScene(AIScene, 1024.0, Observer, 0.0, -2.0, 0.0, Miramar)" | $CMD

# Add ground
echo "Core.SceneManagerService.setGround(default)" | $CMD

# Verify: screenshot from below ground
echo "Core.RendererService.screenshot()" | $CMD
# → Read PNG: should see grey ground above, skybox in upper half

# Move camera above ground for normal view
echo "Core.SceneManagerService.setNodePosition(Observer, 0.0, 10.0, 30.0)" | $CMD

# Screenshot to see the scene from above
echo "Core.RendererService.screenshot()" | $CMD

# Play some music
echo "Core.AudioManagerService.TrackMixerService.play()" | $CMD

# When done
echo "quit" | nc -q 1 localhost $PORT
```

---

## Service Command Reference

| Service | Command | Description |
|---------|---------|-------------|
| **SceneManagerService** | `createScene(name, boundary, camNode, x, y, z [, bg [, ground]])` | Create full scene |
| | `setGround([material])` | Add ground to active scene |
| | `setBackground(skyboxName)` | Set skybox |
| | `createNode(name [, x, y, z])` | Create node |
| | `destroyNode(name)` | Remove node |
| | `setNodePosition(name, x, y, z)` | Move node |
| | `setNodeLookAt(name, x, y, z)` | Orient node |
| | `getNode(name)` | Inspect node (JSON) |
| | `attachCamera(node, camName)` | Attach camera |
| | `attachMicrophone(node, micName)` | Attach microphone |
| | `getSceneInfo()` | Scene summary |
| | `listScenes()` | List all scenes |
| | `listNodes()` | List nodes (target scene first) |
| | `deleteScene(name)` | Delete scene |
| | `enableScene(name)` | Enable scene |
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
| **ArgumentsService** | `getJson()` | All arguments (JSON) |
| | `get(index)` | Argument by index |
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
| **Core** | `exit` / `quit` | Graceful shutdown |
| | `help` / `lsfunc()` | List commands |
| | `listObjects` / `lsobj()` | List services |