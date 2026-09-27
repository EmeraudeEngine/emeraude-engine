## Development Patterns

### Adding a New Platform-Specific Feature

**1. Define the common interface** (`.hpp`):
```cpp
// MyFeature.hpp
class MyFeature {
public:
    static bool doSomething(const std::string& param);
};
```

**2. Create platform implementations**:
```
MyFeature.linux.cpp
MyFeature.mac.mm
MyFeature.windows.cpp
```

**3. Update CMakeLists.txt**:
```cmake
if(WIN32)
    list(APPEND PLATFORM_SOURCES MyFeature.windows.cpp)
elseif(UNIX AND NOT APPLE)
    list(APPEND PLATFORM_SOURCES MyFeature.linux.cpp)
elseif(APPLE)
    list(APPEND PLATFORM_SOURCES MyFeature.mac.mm)
endif()
```

### Handling Unsupported Features

```cpp
bool MyFeature::doSomething(const std::string& param) {
    // Implementation not available on this platform
    Tracer::warning(ClassId, "MyFeature::doSomething not implemented on this platform");
    return false;  // Graceful failure
}
```

---
