# Spécification : Sélection GPU Intelligente avec Détection Optimus

**Version:** 1.1
**Date:** 2025-12-03
**Auteur:** Claude (assistant) / Sheldon
**Statut:** Implémenté

---

## Sommaire

1. [Contexte et Problématique](#1-contexte-et-problématique)
2. [Solution Implémentée](#2-solution-implémentée)
3. [Architecture Technique](#3-architecture-technique)
4. [Settings](#4-settings)
5. [Comportement Runtime](#5-comportement-runtime)
6. [Références](#6-références)

---

## 1. Contexte et Problématique

### 1.1 Bug Identifié

Sur les configurations **Nvidia Optimus** (laptop avec iGPU Intel/AMD + dGPU Nvidia), un **deadlock** se produit dans le driver Nvidia lors de la recréation de surface WSI Windows. Le problème survient de manière aléatoire lors du resize de fenêtre ou de la recréation de swapchain.

### 1.2 Symptômes

- Freeze complet de l'application
- Deadlock dans `win32u.dll` (callstack driver Nvidia)
- `vkAcquireNextImageKHR` ne retourne jamais
- Problème **non reproductible** avec l'iGPU seul

### 1.3 Cause Racine

Bug confirmé côté driver Nvidia sur configurations hybrides. Documenté dans plusieurs projets :
- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820)
- [Dolphin Emulator #13522](https://bugs.dolphin-emu.org/issues/13522)
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091)

### 1.4 Solution

Mode **Failsafe** par défaut qui détecte automatiquement les configurations Optimus et exclut le dGPU Nvidia problématique.

---

## 2. Solution Implémentée

### 2.1 Mode Failsafe

Le mode `Failsafe` (défaut) :
1. Détecte automatiquement les configurations Optimus au démarrage
2. Se comporte comme **Performance** si pas d'Optimus
3. **Exclut** le dGPU Nvidia si Optimus détecté → utilise l'iGPU

### 2.2 Définition Optimus

Une configuration est considérée **Optimus** si :
- Présence d'un `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` (Intel ou AMD)
- **ET** présence d'un `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` avec `vendorID == 0x10DE` (Nvidia)

### 2.3 Flux d'Exécution

```
preparePhysicalDevices()
├── Énumère les GPUs via vkEnumeratePhysicalDevices()
├── detectHybridGPUConfiguration() → isOptimusDetected
├── Lit setting OptimusWorkaround
└── logGPUConfiguration()

getGraphicsDevice()
├── Lit AutoSelectMode (défaut: "Failsafe")
├── Sauvegarde AvailableGPUs (tous les GPUs)
├── getScoredGraphicsDevices(Failsafe)
│   └── Si Failsafe + Optimus → shouldExcludeForFailsafe() → skip dGPU Nvidia
├── Sélectionne le GPU avec le meilleur score
└── Crée le Device logique
```

---

## 3. Architecture Technique

### 3.1 Fichiers Modifiés

| Fichier | Modifications |
|---------|---------------|
| `src/Vulkan/Types.hpp` | `DeviceRunMode::Failsafe`, `VendorID` namespace, `HybridGPUConfig` struct |
| `src/Vulkan/Instance.hpp` | Nouvelles méthodes et membres |
| `src/Vulkan/Instance.cpp` | Implémentation détection et exclusion |
| `src/SettingKeys.hpp` | Nouvelles clés settings |

### 3.2 Enum DeviceRunMode

```cpp
enum class DeviceRunMode : uint8_t
{
    DontCare = 0,      // Sélection basique
    Performance = 1,   // Favorise dGPU
    PowerSaving = 2,   // Favorise iGPU
    Failsafe = 3       // Performance SAUF si Optimus → iGPU
};
```

### 3.3 Structure HybridGPUConfig

```cpp
struct HybridGPUConfig
{
    bool isOptimusDetected{false};
    std::string integratedGPUName{};
    std::string discreteGPUName{};
    uint32_t integratedVendorID{0};
    uint32_t discreteVendorID{0};
};
```

### 3.4 Vendor IDs

```cpp
namespace VendorID
{
    constexpr uint32_t AMD = 0x1002;
    constexpr uint32_t Intel = 0x8086;
    constexpr uint32_t Nvidia = 0x10DE;
    constexpr uint32_t ARM = 0x13B5;
    constexpr uint32_t Qualcomm = 0x5143;
}
```

### 3.5 Méthodes Clés

| Méthode | Rôle |
|---------|------|
| `detectHybridGPUConfiguration()` | Détecte iGPU + dGPU Nvidia |
| `shouldExcludeForFailsafe()` | Retourne true si GPU doit être exclu |
| `logGPUConfiguration()` | Log config GPU au démarrage |

---

## 4. Settings

### 4.1 Structure JSON

```json
{
  "Core": {
    "Video": {
      "VulkanDevice": {
        "AutoSelectMode": "Failsafe",
        "ForceGPU": "",
        "OptimusWorkaround": true,
        "UseVMA": true,
        "AvailableGPUs": [
          "AMD Radeon(TM) Graphics",
          "NVIDIA GeForce RTX 3060 Laptop GPU"
        ]
      }
    }
  }
}
```

### 4.2 Description des Settings

| Setting | Type | Défaut | Description |
|---------|------|--------|-------------|
| `AutoSelectMode` | string | `"Failsafe"` | Mode de sélection : Failsafe, Performance, PowerSaving, DontCare |
| `ForceGPU` | string | `""` | Forcer un GPU par son nom (override tout) |
| `OptimusWorkaround` | bool | `true` | Activer l'exclusion du dGPU Nvidia sur Optimus |
| `UseVMA` | bool | `true` | Utiliser Vulkan Memory Allocator |
| `AvailableGPUs` | array | (runtime) | Liste de tous les GPUs détectés |

### 4.3 Notes

- `AvailableGPUs` est **recalculé à chaque démarrage** (pas persisté)
- Tous les GPUs sont listés, même ceux exclus par Failsafe
- Permet à l'utilisateur de voir les options disponibles pour `ForceGPU`

---

## 5. Comportement Runtime

### 5.1 Matrice des Comportements

| Mode | Optimus | dGPU Nvidia | iGPU | Résultat |
|------|---------|-------------|------|----------|
| **Failsafe** | Non | Score ×5 | Score ×3 | dGPU sélectionné |
| **Failsafe** | Oui | **Exclu** | Score ×5 | **iGPU sélectionné** |
| **Performance** | Non/Oui | Score ×5 | Score ×3 | dGPU sélectionné |
| **PowerSaving** | Non/Oui | Score ×1 | Score ×5 | iGPU sélectionné |

### 5.2 Log de Démarrage

```
[Info]  ========== GPU Configuration ==========
[Info]  Physical devices found: 2
[Info]    [iGPU] AMD Radeon(TM) Graphics (Vendor: AMD (4098))
[Info]    [dGPU] NVIDIA GeForce RTX 3060 Laptop GPU (Vendor: Nvidia (4318))
[Info]  ---------------------------------------
[Warn]  ! Nvidia Optimus configuration DETECTED
[Warn]    Integrated : AMD Radeon(TM) Graphics
[Warn]    Discrete   : NVIDIA GeForce RTX 3060 Laptop GPU
[Warn]    Known driver issues may occur with discrete GPU.
[Warn]    Optimus workaround: Enabled
[Info]  =======================================
[Warn]  Failsafe: Excluding 'NVIDIA GeForce RTX 3060 Laptop GPU' (Nvidia Optimus configuration detected - known driver issues)
[Success] >>> GPU Selected: AMD Radeon(TM) Graphics (Failsafe mode)
```

### 5.3 Priorité de Sélection

```
1. ForceGPU configuré → Utiliser ce GPU (override total)
2. Mode Failsafe + Optimus → Exclure dGPU Nvidia
3. Scoring selon le mode → Prendre le meilleur score
```

---

## 6. Références

### 6.1 Bug Reports

- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820) - Swapchain destruction corrupts driver state
- [Dolphin #13522](https://bugs.dolphin-emu.org/issues/13522) - Resize deadlock with Nvidia Vulkan
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091) - WSI layer deadlock

### 6.2 Documentation

- [Vulkan Tutorial - Swapchain Recreation](https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation)
- [ArchWiki - NVIDIA Optimus](https://wiki.archlinux.org/title/NVIDIA_Optimus)
