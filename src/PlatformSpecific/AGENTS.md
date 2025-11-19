# PlatformSpecific System - Development Context

Context spécifique pour le développement du code platform-specific d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Isolation du code spécifique aux systèmes d'exploitation (Windows, Linux, macOS) avec abstractions maximales pour fournir des API uniformes multiplateforme.

## 📋 Règles spécifiques à PlatformSpecific/

### Philosophie d'isolation STRICTE
- **Code OS séparé** : Chaque OS dans son espace propre
- **JAMAIS de contamination** : Code Windows ne doit JAMAIS toucher Linux/macOS et vice-versa
- **Abstraction maximale** : Interface commune, implémentations OS-specific
- **Isolation stricte** : Code spécifique à un OS ne se retrouve PAS dans espace commun

### Fonctionnalités abstraites

**Appels système** :
- Exécution de commandes système spécifiques OS
- Gestion des processus et environnement

**Dialog boxes** :
- Boîtes de dialogue messages (info, warning, error)
- File open dialog (sélection fichier à ouvrir)
- File save dialog (sélection emplacement sauvegarde)

**Application système** :
- Taskbar notifications (clignoter taskbar)
- Window focus et attention
- Autres interactions spécifiques fenêtre/application

### Organisation du code

**Approche mixte (par ordre de préférence)** :

1. **constexpr if (C++17+)** - Préféré quand possible
```cpp
if constexpr (std::is_same_v<Platform, Windows>) {
    // Code Windows
} else if constexpr (std::is_same_v<Platform, Linux>) {
    // Code Linux
} else if constexpr (std::is_same_v<Platform, macOS>) {
    // Code macOS
}
```

2. **#ifdef** - Si constexpr non applicable
```cpp
#ifdef _WIN32
    // Code Windows
#elif __linux__
    // Code Linux
#elif __APPLE__
    // Code macOS
#endif
```

3. **Fichiers .cpp séparés** - Pour code volumineux
```
PlatformSpecific/
├── DialogBox.hpp          // Interface commune
├── DialogBox_Windows.cpp  // Implémentation Windows
├── DialogBox_Linux.cpp    // Implémentation Linux
└── DialogBox_macOS.cpp    // Implémentation macOS
```
CMake sélectionne le bon fichier selon plateforme cible.

### Gestion des fallbacks
- **Pas de règle fixe** : Décision au cas par cas
- **Warning général** : Si aucune implémentation possible → warning console
- **Graceful degradation** : Fonctionnalité désactivée plutôt que crash
- **Documentation** : Indiquer limitations par OS si applicable

## 🛠️ Commandes de développement

```bash
# Tests platform-specific
ctest -R PlatformSpecific
./test --filter="*Platform*"
```

## 🔗 Fichiers importants

- `DialogBox.*` - Abstractions dialog boxes OS
- `SystemCall.*` - Exécution commandes système
- `WindowManager.*` - Gestion fenêtre et notifications
- CMakeLists.txt - Sélection fichiers par plateforme

## ⚡ Patterns de développement

### Ajout d'une nouvelle fonctionnalité platform-specific

**1. Définir l'interface abstraite commune**
```cpp
// MyFeature.hpp
class MyFeature {
public:
    static bool doSomething(const std::string& param);
};
```

**2. Implémenter pour chaque OS**

**Option A: constexpr if dans .cpp unique**
```cpp
// MyFeature.cpp
bool MyFeature::doSomething(const std::string& param) {
    if constexpr (Platform::isWindows()) {
        // Windows implementation
        return windowsDoSomething(param);
    } else if constexpr (Platform::isLinux()) {
        // Linux implementation
        return linuxDoSomething(param);
    } else if constexpr (Platform::isMacOS()) {
        // macOS implementation
        return macosDoSomething(param);
    }
}
```

**Option B: Fichiers séparés + CMake**
```cpp
// MyFeature_Windows.cpp
bool MyFeature::doSomething(const std::string& param) {
    // Windows only
    return /* ... */;
}

// MyFeature_Linux.cpp
bool MyFeature::doSomething(const std::string& param) {
    // Linux only
    return /* ... */;
}

// MyFeature_macOS.cpp
bool MyFeature::doSomething(const std::string& param) {
    // macOS only
    return /* ... */;
}
```

**CMakeLists.txt**
```cmake
if(WIN32)
    set(PLATFORM_SOURCES MyFeature_Windows.cpp)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES MyFeature_Linux.cpp)
elseif(APPLE)
    set(PLATFORM_SOURCES MyFeature_macOS.cpp)
endif()

add_library(PlatformSpecific ${PLATFORM_SOURCES} ...)
```

### Utilisation depuis code commun
```cpp
// Dans code moteur (commun)
#include "PlatformSpecific/DialogBox.hpp"

void showError(const std::string& message) {
    // API abstraite, implémentation OS-specific transparente
    DialogBox::showError("Error", message);
}
```

### Gestion des fonctionnalités non supportées
```cpp
bool MyFeature::doSomething(const std::string& param) {
    if constexpr (Platform::isWindows()) {
        // Implémentation Windows
        return true;
    } else if constexpr (Platform::isLinux()) {
        // Implémentation Linux
        return true;
    } else {
        // macOS: pas encore implémenté
        Log::warning("MyFeature::doSomething not implemented on macOS");
        return false;  // Échec gracieux
    }
}
```

## 🚨 Points d'attention CRITIQUES

- **Isolation STRICTE** : Code Windows ne touche JAMAIS Linux/macOS et vice-versa
- **Pas de #ifdef dans code commun** : Tout le platform-specific DOIT rester dans PlatformSpecific/
- **Test cross-platform** : Tester sur les 3 OS avant commit
- **API abstraite** : Interface .hpp commune, implémentations séparées
- **CMake correctement configuré** : Vérifier sélection fichiers par OS
- **Warnings explicites** : Si fonctionnalité non supportée, log warning clair
- **Documentation** : Indiquer limitations par OS dans commentaires

## 📚 Documentation détaillée

Systèmes liés:
→ **CMakeLists.txt** - Configuration build multiplateforme
→ **README.md** - Plateformes supportées et requirements
