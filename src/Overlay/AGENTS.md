# Overlay System - Development Context

Context spécifique pour le développement du système d'overlay 2D d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Système d'abstraction 2D pour afficher des éléments par-dessus le rendu 3D. Architecture hiérarchique Manager → Screen → Surface avec support bitmap générique, ImGui pour debug, et intégration CEF possible.

## 📋 Règles spécifiques à Overlay/

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

## 🛠️ Commandes de développement

```bash
# Tests overlay
ctest -R Overlay
./test --filter="*Overlay*"

# Debug overlay
./Emeraude --debug-overlay
./Emeraude --show-overlay-bounds
./Emeraude --overlay-stats
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire principal, coordination Screens, client InputManager
- `Screen.cpp/.hpp` - Groupe logique de Surfaces
- `Surface.cpp/.hpp` - Élément graphique avec Pixmap, position, Z-order
- `Pixmap.cpp/.hpp` - Bitmap/image pour contenu Surface
- `ImGui/` - Intégration ImGui pour debug/dev

### Documentation complémentaire
- `@docs/saphir-shader-system.md` - OverlayGenerator pour pipeline 2D

## ⚡ Patterns de développement

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
// Dans application utilisant le framework
CefBrowser* browser = CreateOffscreenBrowser();

// Callback CEF paint
void OnPaint(const void* buffer, int width, int height) {
    auto surface = overlayManager.getScreen("web")->getSurface("browser");
    surface->pixmap().copyFrom(buffer, width, height);
    surface->markDirty();
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

## 🚨 Points d'attention

- **Z-ordering** : Coordonnée Z détermine ordre de rendu et priorité input
- **Pixmap dirty flag** : Marquer Surface dirty après modification pour re-upload GPU
- **Screen organization** : Grouper logiquement les Surfaces par fonctionnalité
- **Performance** : Éviter modifications Pixmap trop fréquentes (coût upload GPU)
- **Alpha blending** : Utiliser transparence pour Surfaces superposées
- **ImGui temporaire** : Pour debug/dev, pas pour UI finale production
- **CEF externe** : Pas de dépendance framework, intégration par application

## 📚 Documentation détaillée

Systèmes liés:
→ **@docs/saphir-shader-system.md** - OverlayGenerator (pipeline 2D)
→ **@src/Input/AGENTS.md** - Système d'input (polling + events)
→ **@src/Graphics/AGENTS.md** - Renderer et pipelines
