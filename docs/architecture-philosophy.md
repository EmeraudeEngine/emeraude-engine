# Design Philosophy

Emeraude Engine follows three fundamental design principles that guide all architectural and implementation decisions:

## 1. Principle of Least Astonishment (POLA)
**"The code should behave as a reasonable user would expect."**

The system should be intuitive and predictable. Functions, classes, and APIs should do exactly what their names suggest, without hidden side effects or surprising behaviors.

Examples:
```cpp
// CORRECT: Clear, predictable behavior
auto camera = scene->createCamera("main");  // Creates a camera, nothing else
camera->lookAt(target);                     // Points camera at target

// CORRECT: Consistent naming conventions
geometry->getVertexCount();   // get* prefix for accessors
geometry->setVertexData();    // set* prefix for mutators

// WRONG: Surprising side effects
camera->render();  // Should only render, not also create shaders or modify scene
```

## 2. Pit of Success
**"Make the right way easier than the wrong way."**

The architecture should guide developers toward correct, safe implementations by default, making it harder to write buggy or unsafe code.

Examples:
```cpp
// CORRECT: Fail-safe by design (impossible to crash)
auto texture = resources.get<TextureResource>("missing.png");
// Returns neutral texture if missing, application continues safely

// CORRECT: RAII ensures cleanup (impossible to leak)
{
    auto buffer = device->createBuffer(size);
    // Automatic destruction, no manual cleanup needed
}

// CORRECT: Type safety prevents errors at compile time
Vector3 gravity(0, +9.81, 0);  // Y-down enforced by convention

// ANTI-PATTERN: Requires manual error checking (easy to forget)
auto texture = loadTexture("file.png");
if (texture == nullptr) {  // Developer MUST remember to check
    // Handle error...
}
```

## 3. Avoid the Gulf of Execution
**"Never create a gap between user intention and the actions needed to accomplish it."**

High-level (user-facing) APIs must be simple and direct, without complex parameters or convoluted logic. End-user APIs should be trivial to use.

Examples:
```cpp
// CORRECT: Simple API, clear intention, zero complexity
auto texture = resources.get<TextureResource>("albedo.png");
camera->lookAt(target);

// CORRECT: Minimal parameters, intelligent defaults
auto renderTarget = std::make_shared<RenderTarget::Texture>(
    "MyTarget", precisions, extent, viewDistance
);

// WRONG: Too many required parameters (Gulf of Execution)
auto texture = resources.get<TextureResource>(
    "albedo.png",
    VK_FORMAT_R8G8B8A8_SRGB,           // Why should user know this?
    VK_IMAGE_TILING_OPTIMAL,            // Unnecessary complexity
    VK_IMAGE_USAGE_SAMPLED_BIT,         // Should be automatic
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    mipmapLevels, arrayLayers
);

// WRONG: Complex logic required (Gulf of Execution)
if (scene->hasCamera()) {
    auto camera = scene->getCamera();
    if (camera->isInitialized()) {
        if (camera->getType() == CameraType::Perspective) {
            // ... 10 lines of setup
        }
    }
}
```

Where this applies:
- User-facing APIs (Resources, Scene, Camera, Material): Maximum simplicity
- Mid-level APIs (Graphics, Physics): Complexity/control trade-off acceptable
- Low-level APIs (Vulkan abstractions): Complexity acceptable, not exposed to users

How these principles apply to Emeraude Engine:
- Fail-safe Resources = Pit of Success (impossible to crash from nullptr)
- Y-down Coordinate System = Least Astonishment (total consistency, no surprises)
- Vulkan Abstraction = Avoid Gulf of Execution (complexity hidden from users)
- RAII Memory Management = Pit of Success (automatic cleanup, can't forget)
