## 2. Service Hierarchy

All services are registered under `Core` as a single entry point:

```
Core
├── ArgumentsService          — Launch arguments (getJson, get, print)
├── AudioManagerService       — Audio system
│   └── TrackMixerService     — Music playback (play, pause, stop, volume, playlist, etc.)
├── FileSystemService         — File paths (getJson, get, print)
├── RendererService           — Graphics (screenshot, getStatus)
├── ResourcesManagerService   — Resource discovery (listContainers, listResources), memoryCensus
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
