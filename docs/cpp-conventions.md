# C++ Conventions - Emeraude Engine

This document describes the C++ coding conventions used in Emeraude Engine.

## General Rules

- **Standard:** C++20
- **Formatting:** Use `.clang-format` and `.clang-tidy` configurations
- **Language:** All code, comments, and documentation in English

---

## Naming Conventions

### Classes and Structs

- **PascalCase** for class and struct names
- Suffix with purpose when relevant (e.g., `Resource`, `Manager`, `Builder`)

```cpp
class PhysicalDevice;
class TextureResource;
struct HybridGPUConfig;
```

### Methods and Functions

- **camelCase** for method and function names
- Prefix accessors with `get`, mutators with `set`, predicates with `is`/`has`/`can`

```cpp
std::string deviceName() const noexcept;      // Simple accessor (no get prefix)
Vendor vendor() const noexcept;               // Simple accessor
void setEnabled(bool state) noexcept;         // Mutator
bool isFullscreenMode() const noexcept;       // Predicate
bool hasExtension(const char* name) const;    // Predicate
```

### Variables

- **camelCase** for local variables and parameters
- **m_** prefix for member variables
- **s_** prefix for static member variables
- **g_** prefix for global variables (avoid when possible)

```cpp
class Device
{
    VkDevice m_device{VK_NULL_HANDLE};        // Member variable
    static size_t s_instanceCount;             // Static member

    void create(bool useVMA)                   // Parameter: camelCase
    {
        const auto deviceInfo = getInfo();     // Local: camelCase
    }
};
```

### Constants and Enums

- **PascalCase** for enum values
- **PascalCase** or **UPPER_SNAKE_CASE** for constants (project uses PascalCase)
- Always use `enum class` for type safety

```cpp
enum class DeviceAutoSelectMode : uint8_t
{
    DontCare = 0,
    Performance = 1,
    PowerSaving = 2,
    Failsafe = 3
};

enum class Vendor : uint32_t
{
    Unknown = 0,
    AMD = 0x1002,
    Nvidia = 0x10DE,
    Intel = 0x8086
};
```

### Namespaces

- **PascalCase** for namespaces
- Main namespace: `EmEn` (Emeraude Engine)

```cpp
namespace EmEn::Vulkan
{
    class Instance;
}
```

---

## Member Layout and Padding

### Boolean Members Must Be Grouped at the End

To avoid memory padding waste, **all boolean members must be placed at the end** of struct/class member declarations. Booleans are 1 byte, but the compiler may add padding bytes to align the next member.

```cpp
// WRONG - padding waste between booleans and larger types
struct BadLayout
{
    bool m_enabled{false};           // 1 byte + 7 bytes padding
    uint64_t m_size{0};              // 8 bytes
    bool m_visible{true};            // 1 byte + 7 bytes padding
    void* m_pointer{nullptr};        // 8 bytes
};  // Total: 32 bytes (with 14 bytes wasted)

// CORRECT - booleans grouped at the end
struct GoodLayout
{
    uint64_t m_size{0};              // 8 bytes
    void* m_pointer{nullptr};        // 8 bytes
    bool m_enabled{false};           // 1 byte
    bool m_visible{true};            // 1 byte + 6 bytes padding (unavoidable)
};  // Total: 24 bytes (only 6 bytes padding)
```

### General Member Ordering

Order members by size (largest first) to minimize padding:

1. Pointers, `uint64_t`, `double` (8 bytes)
2. `uint32_t`, `float`, `int` (4 bytes)
3. `uint16_t`, `short` (2 bytes)
4. `uint8_t`, `char`, `bool` (1 byte)

```cpp
class Instance
{
    // Large types first
    std::shared_ptr<Device> m_graphicsDevice;
    std::shared_ptr<Device> m_computeDevice;
    std::vector<const char*> m_requiredValidationLayers;
    std::vector<const char*> m_requiredInstanceExtensions;
    HybridGPUConfig m_hybridConfig{};

    // Booleans at the end
    bool m_showInformation{false};
    bool m_debugMode{false};
    bool m_useDebugMessenger{false};
};
```

---

## Function Attributes

### Use `[[nodiscard]]` for Return Values

Mark functions with `[[nodiscard]]` when ignoring the return value is likely a bug:

```cpp
[[nodiscard]]
bool create() noexcept;

[[nodiscard]]
std::optional<uint32_t> findQueueFamily() const noexcept;
```

### Use `noexcept` When Appropriate

Mark functions `noexcept` when they don't throw exceptions:

```cpp
void destroy() noexcept;
const char* deviceName() const noexcept;
```

### Use `const` Correctly

- Mark methods `const` when they don't modify the object
- Prefer `const&` for input parameters that won't be modified

```cpp
[[nodiscard]]
Vendor vendor() const noexcept;

void processDevice(const std::shared_ptr<PhysicalDevice>& device);
```

---

## Error Handling

### Use `value_or()` with `std::optional`

When parsing user input or settings, always provide a fallback:

```cpp
// WRONG - crashes if setting contains invalid value
const auto mode = magic_enum::enum_cast<DeviceAutoSelectMode>(modeString).value();

// CORRECT - safe fallback
const auto mode = magic_enum::enum_cast<DeviceAutoSelectMode>(modeString)
    .value_or(DeviceAutoSelectMode::Failsafe);
```

### Never Return nullptr for Resources

The engine uses fail-safe resources. Always return a valid fallback:

```cpp
// WRONG
Texture* getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : nullptr;  // Dangerous!
}

// CORRECT
std::shared_ptr<Texture> getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : m_neutralTexture;  // Safe
}
```

---

## Initialization

### Use Brace Initialization with Default Values

```cpp
class Device
{
    VkDevice m_device{VK_NULL_HANDLE};
    uint32_t m_queueCount{0};
    bool m_initialized{false};
};
```

### Use Designated Initializers for Structs (C++20)

```cpp
VkApplicationInfo appInfo{
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName = "MyApp",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Emeraude",
    .engineVersion = VK_MAKE_VERSION(0, 8, 0),
    .apiVersion = VK_API_VERSION_1_3
};
```

---

## Template Patterns

### CRTP (Curiously Recurring Template Pattern)

Use CRTP for zero-overhead polymorphism when the derived types are known at compile time. This avoids virtual function overhead while enabling code reuse.

**Naming convention:** Prefix template class names with `T_` to indicate a template base class.

```cpp
// CRTP base class - uses T_ prefix
template< typename derived_t, Severity severity_t >
class T_TraceHelperBase : public Libs::BlobTrait
{
    public:
        explicit T_TraceHelperBase(const char* tag,
            const std::source_location& location = std::source_location::current()) noexcept
            : m_tag{tag}, m_location{location} {}

        ~T_TraceHelperBase()
        {
            Tracer::getInstance().trace(severity_t, m_tag, this->get(), m_location);
        }

    protected:
        const char* m_tag;
        std::source_location m_location;
};

// Derived classes inherit constructors via using declaration
class TraceInfo final : public T_TraceHelperBase<TraceInfo, Severity::Info>
{
    public:
        using T_TraceHelperBase::T_TraceHelperBase;  // Inherit all constructors
};

class TraceError final : public T_TraceHelperBase<TraceError, Severity::Error>
{
    public:
        using T_TraceHelperBase::T_TraceHelperBase;
};
```

**When to use CRTP:**
- Multiple classes share identical logic with only a compile-time constant difference
- Virtual dispatch overhead is unacceptable (hot paths)
- The set of derived types is fixed and known

**Reference implementation:** `src/Tracer.hpp:T_TraceHelperBase`

---

## Comments

### Use Doxygen for Public APIs

```cpp
/**
 * @brief Returns the vendor enum.
 * @note Shortcut to PhysicalDevice::properties().
 * @return Vendor
 * @version 0.8.38
 */
[[nodiscard]]
Vendor vendor() const noexcept;
```

### Use `/* NOTE: */` for Implementation Comments

```cpp
/* NOTE: Detect hybrid GPU configuration (Optimus, etc.) */
m_hybridConfig = this->detectHybridGPUConfiguration();
```

---

## See Also

- [AGENTS.md](../AGENTS.md) - Main project documentation
- [Tracer System Usage](../AGENTS.md#tracer-system-usage) - Logging conventions
- [Coordinate System](coordinate-system.md) - Y-down convention
