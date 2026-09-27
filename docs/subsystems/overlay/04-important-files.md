## Important Files

- `Manager.cpp/.hpp` - Main manager, Screen coordination, InputManager client, 4 shader programs
- `UIScreen.cpp/.hpp` - Logical Surface group with rendering options (premultiplied alpha, BGRA format)
- `Surface.cpp/.hpp` - Graphical element with Framebuffer, position, Z-order, transition buffer system
- `Surface.hpp:Framebuffer` - Struct with image, imageView, sampler, pixmap, descriptorSet, width()/height()
- `Surface.hpp:writeWithMapping()` - C++20 template with requires constraint for type-safe GPU writes
- `FramebufferProperties.cpp/.hpp` - Screen resolution and scaling properties
- `ImGui/` - ImGui integration for debug/dev

### Additional Documentation
- `@docs/saphir-shader-system.md` - OverlayGenerator for 2D pipeline
