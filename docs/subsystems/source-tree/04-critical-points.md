## Critical Points

- **Core is central**: Everything goes through Core, don't bypass
- **Three threads**: Main, Logic, Render - watch thread safety
- **Arguments > Settings**: Strict priority hierarchy
- **Free debug tracer**: Debug messages eliminated in Release (zero cost)
- **Settings auto-save**: Automatic save, no need to call save()
- **SHIFT+F5**: Live Settings editing, watch for invalid values
- **Cross-platform FileSystem**: Never hardcode OS-specific paths
- **ServiceInterface**: All services must respect init/shutdown protocol
- **Window owns Surface**: Don't manually create Vulkan Surface
- **PlatformManager init GLFW**: Must be initialized before Window
