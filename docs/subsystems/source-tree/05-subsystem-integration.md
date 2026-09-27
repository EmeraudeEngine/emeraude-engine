## Subsystem Integration

### Core Orchestrates Everything
```
Core
 ├─ Window (window + Vulkan Surface)
 ├─ PrimaryServices
 │   ├─ Arguments
 │   ├─ FileSystem
 │   ├─ Settings
 │   ├─ Net::Manager
 │   └─ ThreadPool
 ├─ Renderer (Graphics)
 ├─ Physics
 ├─ Audio
 ├─ Scenes
 ├─ Resources
 └─ Input
```

### Initialization Flow
```
1. main(argc, argv)
2. PlatformManager init GLFW
3. Arguments parse (argc, argv)
4. Settings load (FileSystem config dir)
5. Arguments override Settings
6. Window create (+ Vulkan Surface)
7. Core init subsystems (ServiceInterface)
8. Core start threads (Logic, Render)
9. Main loop
10. Core shutdown subsystems
11. Settings save (auto)
```
