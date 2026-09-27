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
