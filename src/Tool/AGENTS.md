# Tool (Utilities)

Context for developing Emeraude Engine utility tools.

## Module Overview

**Status: RARELY USED** - Auxiliary utilities using engine logic for specific tasks, launched via command line arguments. Concept present but not actively used.

## Tool-Specific Rules

### Concept: Auxiliary Utilities
- **Not the main application**: Tools for specific tasks, not for launching game/app
- **Uses engine logic**: Reuses engine systems (Geometry, Vulkan, etc.)
- **Runtime via CLI**: Launched by command line arguments

### Available Tools (examples)

**GeometryDataPrinter**:
- Displays geometry data structure in text format
- Inspection/debug of geometry files

**VulkanCapabilities**:
- Displays system Vulkan capabilities
- GPU info, supported extensions, limits

### Invocation (approximate syntax)
```bash
# Conceptual example (exact syntax to be defined)
./Emeraude --tool geometry-printer file.obj
./Emeraude --tool vulkan-capabilities
```

### Architecture
- Tools registered with unique name
- Dispatch based on `--tool` argument
- Access to engine systems (Resources, Graphics, etc.)

## Development Commands

```bash
# List available tools (if implemented)
./Emeraude --list-tools

# Launch a tool
./Emeraude --tool <tool-name> [args...]
```

## Important Files

- GeometryDataPrinter - Geometry inspection
- VulkanCapabilities - GPU capabilities info
- To be documented upon system activation

## Development Patterns

### Adding a New Tool (concept)
```cpp
// Create tool class
class MyTool {
public:
    static int run(const std::vector<std::string>& args) {
        // Use engine systems
        auto geometry = loadGeometry(args[0]);
        processGeometry(geometry);
        return 0;  // Success
    }
};

// Register
ToolRegistry::register("my-tool", &MyTool::run);
```

### Usage from main()
```cpp
int main(int argc, char* argv[]) {
    // Detect tool mode
    if (hasArg("--tool")) {
        std::string toolName = getArg("--tool");
        return ToolRegistry::run(toolName, remainingArgs);
    }

    // Otherwise launch normal application
    return runApplication();
}
```

## Critical Points

- **Rarely used currently**: Concept present but not actively developed
- **Syntax to define**: Exact CLI interface to be standardized
- **Engine system access**: Tools can use Resources, Graphics, etc.
- **Not for production**: Development/debug tools only
- **Clean exit**: Return appropriate exit code (0 = success)

## Detailed Documentation

To be created if Tool system becomes actively used.

Systems usable by Tools:
- @src/Libs/AGENTS.md - Foundational libraries
- @src/Graphics/AGENTS.md - For graphics tools
- @src/Resources/AGENTS.md - Resource loading
