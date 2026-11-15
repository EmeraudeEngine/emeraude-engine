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
# Minimise, then restore (2026-11-15). getState() says "iconified" as the window system reports it: never on Wayland.
python3 tools/remote-console.py "Core.WindowService.iconify()"
python3 tools/remote-console.py "Core.WindowService.getBackOnDesktop()"
```
⚠️ The Core-level keys need the Shift modifier: the fullscreen toggle is `Core.InputManagerService.keyPress(300, 1)`
(Shift+F11), `keyPress(300, 0)` does nothing.

### Music playback
```bash
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.play()"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.pause()"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.volume(50)"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlist()"
python3 tools/remote-console.py "Core.AudioManagerService.TrackMixerService.playlistPlay(3)"
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
