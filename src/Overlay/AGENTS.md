# Overlay System

Context spécifique pour le développement du système d'overlay 2D d'Emeraude Engine.

## Vue d'ensemble du module

Système d'abstraction 2D pour afficher des éléments par-dessus le rendu 3D. Architecture hiérarchique Manager → Screen → Surface avec support bitmap générique, ImGui pour debug, et intégration CEF possible.

## Règles spécifiques à Overlay/

### Architecture hiérarchique

**Manager** : Gestionnaire principal du système overlay
**Screen** : Groupe logique de Surfaces (pas de dimensions graphiques, juste organisation)
**Surface** : Élément graphique avec Pixmap (bitmap), position, dimensions, coordonnée Z

### Concept de Surface
- **Pixmap** : Bitmap/image représentant le contenu de la Surface
- **Position** : Coordonnées X, Y à l'écran
- **Dimensions** : Largeur, hauteur en pixels
- **Z-ordering** : Coordonnée Z pour superposition des Surfaces
- Plusieurs Surfaces peuvent coexister dans un Screen

### Double Buffering System (Transition Buffer)

For asynchronous content providers (e.g., CEF browsers), Surface supports a transition buffer system to handle resize smoothly without visual glitches.

**Key concepts:**
- **Active Buffer**: Currently displayed framebuffer
- **Transition Buffer**: New-size buffer waiting for content during resize
- **Framebuffer struct**: Contains `image`, `imageView`, `sampler`, `pixmap`, `descriptorSet`, plus `width()` and `height()` accessors

**Lifecycle:**
1. `enableTransitionBuffer()` - Enable async content provider mode
2. On resize: transition buffer created at new size, active buffer continues displaying
3. Content provider fills transition buffer via `transitionPixmap()` or `writeTransitionBufferWithMapping()`
4. `commitTransitionBuffer()` - Swap buffers when new content ready
5. Callbacks: `onActiveBufferReady()`, `onTransitionBufferReady()` for notifications

**Code references:**
- `Surface.hpp:Framebuffer` struct definition
- `Surface.cpp:commitTransitionBuffer()` - Buffer swap logic
- `Surface.hpp:isTransitionBufferReady()` - Check if transition buffer awaits content

### Direct GPU Memory Mapping

For performance optimization, Surface supports direct GPU memory writes bypassing the staging buffer path.

**When to use:**
- High-frequency content updates (video, browser rendering)
- When content provider already has pixel data in memory
- Reduces CPU→GPU copy overhead

**Requirements:**
- Call `enableMapping()` **before** `createOnHardware()` (typically in constructor)
- Image created with `VK_IMAGE_TILING_LINEAR` and `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`
- Image layout transitioned to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` after creation

**API:**
```cpp
// In constructor (BEFORE createOnHardware is called):
this->enableMapping();

// Write to active buffer:
bool success = this->writeActiveBufferWithMapping([&](void* ptr, VkDeviceSize rowPitch) {
    // Copy pixel data respecting rowPitch
    return true;
});

// Write to transition buffer:
bool success = this->writeTransitionBufferWithMapping([&](void* ptr, VkDeviceSize rowPitch) {
    // Copy pixel data respecting rowPitch
    return true;
});
```

**C++20 type safety:** `writeWithMapping()` uses `requires` constraint:
```cpp
requires std::invocable<write_func_t, void*, VkDeviceSize> &&
         std::convertible_to<std::invoke_result_t<write_func_t, void*, VkDeviceSize>, bool>
```

**Code references:**
- `Surface.hpp:enableMapping()` - Enable memory mapping mode
- `Surface.hpp:isMemoryMappingEnabled()` - Check if enabled
- `Surface.hpp:Framebuffer::writeWithMapping()` - RAII mapping with lambda
- `Surface.cpp:createFramebufferResources()` - Image creation with LINEAR tiling
- `Vulkan/Image.cpp:mapMemory()` / `unmapMemory()` - VMA memory mapping

**CRITICAL:** `enableMapping()` must be called before `createOnHardware()`. If called after, the image is already created without host-visible memory and mapping will fail.

### Types de contenu supportés

**Surface générique** : Pixmap bitmap modifiable (cas d'usage principal)
**ImGui** : Intégration pour développement/debug rapide
**CEF offscreen** : Pages web via CEF rendues dans Surface générique (intégration externe)
**Future** : Système UI basique intégré (boutons, widgets, etc.)

### Integration avec le rendu
- **Pipeline 2D via Saphir** : OverlayManager utilise OverlayGenerator
- **Ordre de rendu** : Renderer fait 3D puis 2D overlay
- **Pas de lighting** : Rendu 2D screen-space pur
- **Alpha blending** : Support transparence et multi-layer

### Integration avec Input
- **OverlayManager client d'InputManager** : Reçoit événements souris/clavier
- **Dispatch hiérarchique** : Manager → Screen → Surface
- **Gestion interactions** : Clics, hover, keyboard focus
- **Z-ordering** : Surfaces avec Z plus élevé reçoivent events en premier

### Integration CEF (externe)
- CEF non intégré au framework (dépendance externe)
- Applications peuvent utiliser CEF en mode offscreen
- Rendu CEF → Pixmap de Surface générique
- OverlayManager affiche la Surface normalement

**Two rendering paths for CEF OnPaint():**

1. **Staging Buffer (Classic)**: CEF buffer → local Pixmap → staging buffer → GPU
   - Uses `activePixmap()` / `transitionPixmap()` + `setVideoMemoryOutdated()`
   - Compatible with all hardware

2. **Direct Memory Mapping**: CEF buffer → GPU mapped memory (bypasses staging)
   - Uses `writeActiveBufferWithMapping()` / `writeTransitionBufferWithMapping()`
   - Better performance, requires `enableMapping()` in constructor
   - Handle `rowPitch` differences between CEF (width*4) and GPU tiling

**Dirty rects support:** Both paths support partial updates via CEF's `dirtyRects` parameter for optimal performance.

## Commandes de développement

```bash
# Tests overlay
ctest -R Overlay
./test --filter="*Overlay*"
```

## Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire principal, coordination Screens, client InputManager
- `Screen.cpp/.hpp` - Groupe logique de Surfaces
- `Surface.cpp/.hpp` - Élément graphique avec Framebuffer, position, Z-order, transition buffer system
- `Surface.hpp:Framebuffer` - Struct with image, imageView, sampler, pixmap, descriptorSet, width()/height()
- `Surface.hpp:writeWithMapping()` - C++20 template with requires constraint for type-safe GPU writes
- `FramebufferProperties.cpp/.hpp` - Screen resolution and scaling properties
- `ImGui/` - Intégration ImGui pour debug/dev

### Documentation complémentaire
- `@docs/saphir-shader-system.md` - OverlayGenerator pour pipeline 2D

## Patterns de développement

### Création d'un Screen avec Surfaces
```cpp
// Créer un Screen
auto hudScreen = overlayManager.createScreen("hud");

// Créer des Surfaces dans le Screen
auto healthBar = hudScreen->createSurface("health_bar");
healthBar->setPosition(10, 10);
healthBar->setDimensions(200, 20);
healthBar->setZ(10);  // Z-ordering

auto minimap = hudScreen->createSurface("minimap");
minimap->setPosition(screenWidth - 210, 10);
minimap->setDimensions(200, 200);
minimap->setZ(5);  // Derrière la health bar si overlap
```

### Modification du contenu d'une Surface
```cpp
// Accéder au Pixmap de la Surface
auto& pixmap = surface->pixmap();

// Dessiner dans le bitmap
pixmap.fill(Color::Black);
pixmap.drawRectangle(10, 10, 50, 30, Color::Red);
pixmap.drawText(20, 20, "HP: 100", font, Color::White);

// Marquer comme modifié pour re-upload GPU
surface->markDirty();
```

### Utilisation d'ImGui pour debug
```cpp
// ImGui intégré pour développement rapide
overlayManager.beginImGuiFrame();

ImGui::Begin("Debug Info");
ImGui::Text("FPS: %.1f", fps);
ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
ImGui::End();

overlayManager.endImGuiFrame();
```

### Integration CEF (externe)
```cpp
// WebView constructor - enable features BEFORE createOnHardware()
WebView::WebView(...) : Surface{...} {
    this->enableTransitionBuffer();  // For smooth resize

    if (settings.get<bool>("CEF/UseMemoryMapping")) {
        this->enableMapping();  // MUST be before createOnHardware()
    }
}

// CEF OnPaint callback - dual path implementation
void WebView::OnPaint(const void* buffer, int width, int height, const RectList& dirtyRects) {
    const auto widthU = static_cast<uint32_t>(width);
    const auto heightU = static_cast<uint32_t>(height);

    // PATH 1: Direct GPU Memory Mapping
    if (this->isMemoryMappingEnabled()) {
        const auto* srcBuffer = static_cast<const uint8_t*>(buffer);
        const size_t srcPitch = widthU * 4;  // CEF provides BGRA

        auto writeFunction = [&](void* mappedPtr, VkDeviceSize rowPitch) -> bool {
            auto* dstBuffer = static_cast<uint8_t*>(mappedPtr);
            for (uint32_t y = 0; y < heightU; ++y) {
                std::memcpy(dstBuffer + y * rowPitch, srcBuffer + y * srcPitch, srcPitch);
            }
            return true;
        };

        // Choose buffer based on size match
        if (this->isTransitionBufferReady() && transitionBuffer().width() == widthU) {
            this->writeTransitionBufferWithMapping(writeFunction);
            this->commitTransitionBuffer();
        } else {
            this->writeActiveBufferWithMapping(writeFunction);
        }
        return;
    }

    // PATH 2: Staging Buffer (Classic)
    auto& pixmap = (isTransitionBufferReady() && matchesTransitionSize)
                   ? this->transitionPixmap()
                   : this->activePixmap();

    // Copy to pixmap using PixelFactory::Processor
    Processor<uint8_t> processor{pixmap};
    processor.blit(rawData, clip);

    if (isTransitionBuffer) {
        this->commitTransitionBuffer();
    } else {
        this->setVideoMemoryOutdated();
    }
}
```

### Gestion des événements Input
```cpp
// OverlayManager dispatch automatiquement
// Implémenter dans Surface si nécessaire
class CustomSurface : public Surface {
    void onMouseClick(int x, int y, MouseButton button) override {
        // Gérer clic sur cette Surface
    }

    void onMouseHover(int x, int y) override {
        // Gérer hover
    }
};
```

## Points d'attention

- **Z-ordering** : Coordonnée Z détermine ordre de rendu et priorité input
- **Pixmap dirty flag** : Marquer Surface dirty après modification pour re-upload GPU
- **Screen organization** : Grouper logiquement les Surfaces par fonctionnalité
- **Performance** : Éviter modifications Pixmap trop fréquentes (coût upload GPU)
- **Alpha blending** : Utiliser transparence pour Surfaces superposées
- **ImGui temporaire** : Pour debug/dev, pas pour UI finale production
- **CEF externe** : Pas de dépendance framework, intégration par application

### Memory Mapping Critical Rules
- **TIMING:** `enableMapping()` MUST be called before `createOnHardware()` (in constructor)
- **Row pitch:** GPU memory may have different row pitch than source data; always use the `rowPitch` parameter
- **Image tiling:** Mapped images use `VK_IMAGE_TILING_LINEAR` (vs OPTIMAL for staging path)
- **Layout transition:** Engine handles `UNDEFINED → SHADER_READ_ONLY_OPTIMAL` transition automatically

### Transition Buffer Critical Rules
- **Size matching:** Always compare frame size with `activeBuffer().width()/height()`, not pixmap dimensions
- **Commit timing:** Call `commitTransitionBuffer()` only after content is fully written
- **Callback order:** `onTransitionBufferReady()` fires when new buffer is ready for content

## Documentation détaillée

Systèmes liés:
- @docs/saphir-shader-system.md** - OverlayGenerator (pipeline 2D)
- @src/Input/AGENTS.md** - Système d'input (polling + events)
- @src/Graphics/AGENTS.md** - Renderer et pipelines
