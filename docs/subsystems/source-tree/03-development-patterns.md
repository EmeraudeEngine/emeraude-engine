## Development Patterns

### Creating an Application
```cpp
#include <EmEn/Core.hpp>

/* Settings entries kept across a settings reset, on top of the engine base
 * (SettingsKeyRestoration::EngineKeys). Constant data above the constructor:
 * the list grows without bloating the initializer, and it is independent of
 * the per-release reset switch. */
constexpr std::array< std::string_view, 2 > RestoredSettingsKeys{
    EmEn::VkInstanceEnableDebugKey,
    EmEn::VkInstanceRequestedValidationLayersKey
};

class MyGame : public EmEn::Core {
public:
    MyGame(int argc, char** argv) noexcept
        : Core{argc, argv, "MyGame", {1, 0, 0}, "MyOrg", "example.com",
               /* resetSettingsOnNewVersion */ true, EmEn::SettingsKeyRestoration{RestoredSettingsKeys}} {}

private:
    // Required: Called when engine is fully initialized
    bool onCoreStarted(const EmEn::Arguments & arguments, EmEn::Settings & settings) noexcept override {
        TRACE_INFO("Game initialized");
        // Load scenes, resources, etc.
        return true;  // Return true to start main loop
    }

    // Required: Called every logic frame (separate thread)
    void onCoreProcessLogics(size_t engineCycle) noexcept override {
        updateGameState();
    }

    // Optional: Cleanup before shutdown
    void onBeforeCoreStop() noexcept override {
        TRACE_INFO("Game shutdown");
    }
};

int main(int argc, char** argv) {
    MyGame game(argc, argv);
    return game.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

### Using Settings
```cpp
// Reading
int width = settings.get<int>("window.width");
bool vsync = settings.get<bool>("graphics.vsync");

// Writing (saved on close)
settings.set("window.width", 1920);
settings.set("audio.master_volume", 0.8f);

// Manual editing: SHIFT+F5 during execution
```

### Using Tracer

> **FUNDAMENTAL RULE:** One trace = one complete log entry. **NEVER** use multiple traces to compose a single logical message. Use `"\n"` for multi-line content, or a named trace variable across scopes.

**Three forms** (choose based on use case):

```cpp
// 1. Static methods — no variables, most performant
Tracer::info(ClassId, "Loading level");
Tracer::warning(ClassId, "Low FPS detected");

// 2. Inline RAII — messages with variables
TraceInfo{ClassId} << "Loaded " << count << " textures";
TraceError{ClassId} << "Failed to load: " << path;
TraceDebug{ClassId} << "Position: " << x << ", " << y;  // Eliminated in Release

// 3. Named variable — multi-scope/conditional message (single trace entry)
TraceWarning trace{ClassId};
trace << "Report:" "\n";
if ( hasData )
{
    trace << "  Data: " << data << "\n";
}
trace << "Done.";
// Emitted as ONE log entry when 'trace' goes out of scope.
```

**Complete rules**: See [`docs/tracer-system.md`](../../tracer-system.md) (braces `{}`, method vs class choice, `"\n"` for multi-line).

### Using FileSystem
```cpp
// Cross-platform directories
auto configPath = fileSystem.getConfigDirectory();  // ~/.config/MyApp/
auto cachePath = fileSystem.getCacheDirectory();    // ~/.cache/MyApp/
auto dataPath = fileSystem.getDataDirectory();      // ~/.local/share/MyApp/
auto tempPath = fileSystem.getTempDirectory();      // /tmp/MyApp/

// Settings automatically uses configPath
// Resources can use dataPath for user assets
```

### Override with Arguments
```bash
# Arguments override Settings
./MyGame --window.width=2560 --window.height=1440 --graphics.vsync=false

# Custom arguments
./MyGame --custom-flag --my-value=42
```

```cpp
// In code
if (arguments.has("custom-flag")) {
    int value = arguments.get<int>("my-value");
}
```
