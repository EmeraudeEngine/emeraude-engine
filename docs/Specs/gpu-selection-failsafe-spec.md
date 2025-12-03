# Spécification : Sélection GPU Intelligente avec Détection Optimus

**Version:** 2.0
**Date:** 2025-12-04
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

Sur les configurations **Nvidia Optimus** (laptop avec iGPU Intel/AMD + dGPU Nvidia mobile), un **deadlock** se produit dans le driver Nvidia lors de la recréation de surface WSI Windows. Le problème survient de manière aléatoire lors du resize de fenêtre ou de la recréation de swapchain.

### 1.2 Symptômes

- Freeze complet de l'application
- Deadlock dans `win32u.dll` (callstack driver Nvidia)
- `vkAcquireNextImageKHR` ne retourne jamais
- Problème **non reproductible** avec l'iGPU seul

### 1.3 Cause Racine

Bug confirmé côté driver Nvidia sur configurations hybrides **laptop** où le dGPU n'a pas de sorties display physiques et passe par l'iGPU. Documenté dans plusieurs projets :
- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820)
- [Dolphin Emulator #13522](https://bugs.dolphin-emu.org/issues/13522)
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091)

### 1.4 Condition de Manifestation

**Important:** Le bug ne se manifeste que lors de la **recréation de swapchain en mode fenêtré** (resize). En mode **fullscreen exclusif**, le dGPU Nvidia est safe car il n'y a pas de resize dynamique de la surface.

### 1.5 Desktop vs Laptop

Un desktop avec un CPU ayant un iGPU intégré (ex: Intel UHD) + une RTX discrete n'est **PAS** un système Optimus. Sur desktop, la RTX a ses propres sorties display (HDMI, DisplayPort) et ne passe pas par l'iGPU → pas de bug.

### 1.6 Solution

Système de **FailSafe** indépendant du mode de sélection, activé par défaut, qui détecte automatiquement les configurations Optimus **laptop** (GPU mobile) et exclut le dGPU Nvidia problématique **uniquement en mode fenêtré**.

---

## 2. Solution Implémentée

### 2.1 Architecture v2.0

Le système sépare deux préoccupations distinctes :

| Concept | Setting | Rôle |
|---------|---------|------|
| **Mode de scoring** | `AutoSelectMode` | Détermine comment scorer les GPUs (Performance, PowerSaving, DontCare) |
| **Protection FailSafe** | `EnableFailSafe` | Active/désactive les exclusions de sécurité (Optimus, etc.) |

Cette séparation permet d'avoir le FailSafe actif quel que soit le mode de scoring choisi.

### 2.2 Modes de Sélection

| Mode | Comportement |
|------|--------------|
| **DontCare** | Premier device disponible |
| **Performance** | Meilleur score (favorise dGPU) - **défaut** |
| **PowerSaving** | Score inversé (favorise iGPU) |

### 2.3 Protection FailSafe

Le FailSafe est une **couche de protection indépendante** qui s'applique avant le scoring :

- **Activé par défaut** (`EnableFailSafe = true`)
- Détecte les configurations problématiques connues
- Exclut les devices concernés de la sélection

### 2.4 Définition Optimus

Une configuration est considérée **Optimus** si :
1. Présence d'un `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` (Intel ou AMD)
2. **ET** présence d'un `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` avec `vendorID == 0x10DE` (Nvidia)
3. **ET** le GPU Nvidia est un **modèle mobile/laptop** (détecté par son nom)

### 2.5 Condition d'Exclusion

Le dGPU Nvidia est exclu **si et seulement si** :
- `EnableFailSafe = true`
- Configuration Optimus détectée
- **ET** mode fullscreen exclusif **non activé**

En fullscreen exclusif, le dGPU Nvidia est utilisable même sur Optimus.

### 2.6 Détection Mobile GPU

La méthode `isLikelyMobileGPU()` vérifie si le nom du GPU contient un indicateur mobile :

| Pattern | Exemple |
|---------|---------|
| `"Mobile"` | "GeForce RTX 3080 Mobile" |
| `"Laptop"` | "GeForce RTX 4070 Laptop GPU" |
| `"Max-Q"` | "Quadro RTX 3000 Max-Q" |
| `" MX"` | "GeForce MX450" (série MX = toujours mobile) |

Les GPU desktop (ex: "GeForce RTX 4080") n'ont jamais ces mots-clés.

### 2.7 Flux d'Exécution

```
readSettings()
├── m_autoSelectMode = Performance | PowerSaving | DontCare
├── m_enableFailSafe = true (défaut)
├── m_fullscreenExclusiveEnabled
└── m_useVulkanMemoryAllocator

getScoredGraphicsDevices(window)
├── detectHybridGPUConfiguration() → HybridGPUConfig
├── logGPUConfiguration(hybridConfig)
├── Pour chaque device :
│   ├── Si m_enableFailSafe && shouldExcludeForFailsafe(hybridConfig, device) → skip
│   ├── checkDeviceCompatibility() → score de base
│   └── modulateDeviceScoring() selon m_autoSelectMode
└── Retourne map triée par score

getGraphicsDevice(window)
├── ForceGPU configuré ? → Utiliser ce GPU
└── Sinon → Device avec meilleur score
```

---

## 3. Architecture Technique

### 3.1 Fichiers Modifiés

| Fichier | Modifications |
|---------|---------------|
| `src/Vulkan/Types.hpp` | `DeviceAutoSelectMode` enum, `Vendor` enum, `HybridGPUConfig` struct |
| `src/Vulkan/Instance.hpp` | Membres et méthodes |
| `src/Vulkan/Instance.cpp` | Implémentation détection et exclusion |
| `src/SettingKeys.hpp` | Clés settings |

### 3.2 Enum DeviceAutoSelectMode

```cpp
enum class DeviceAutoSelectMode : uint8_t
{
    DontCare = 0,      // Premier device disponible
    Performance = 1,   // Favorise dGPU (meilleur score) - DÉFAUT
    PowerSaving = 2    // Favorise iGPU (score inversé)
};
```

### 3.3 Enum Vendor

```cpp
enum class Vendor : uint32_t
{
    Unknown = 0,
    AMD = 0x1002,
    ImgTec = 0x1010,
    Nvidia = 0x10DE,
    ARM = 0x13B5,
    Qualcomm = 0x5143,
    Intel = 0x8086
};
```

### 3.4 Structure HybridGPUConfig

```cpp
struct HybridGPUConfig
{
    bool isOptimusDetected{false};      // Laptop: iGPU + mobile Nvidia dGPU
    bool isHybridNonOptimus{false};     // Desktop: iGPU CPU + discrete RTX (safe)
    std::string integratedGPUName{};
    std::string discreteGPUName{};
    uint32_t integratedVendorID{0};
    uint32_t discreteVendorID{0};
};
```

### 3.5 Méthodes Clés

| Méthode | Rôle |
|---------|------|
| `readSettings()` | Lit les settings au démarrage |
| `isLikelyMobileGPU(string)` | Détecte si le nom GPU indique un modèle mobile |
| `detectHybridGPUConfiguration()` | Détecte iGPU + dGPU Nvidia + vérifie si mobile |
| `shouldExcludeForFailsafe(config, device)` | Retourne true si GPU doit être exclu |
| `logGPUConfiguration(config)` | Log config GPU |
| `modulateDeviceScoring(props, score)` | Ajuste le score selon m_autoSelectMode |

---

## 4. Settings

### 4.1 Structure JSON

```json
{
  "Core": {
    "Video": {
      "VulkanDevice": {
        "AutoSelectMode": "Performance",
        "EnableFailSafe": true,
        "ForceGPU": "",
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
| `AutoSelectMode` | string | `"Performance"` | Mode de scoring : Performance, PowerSaving, DontCare |
| `EnableFailSafe` | bool | `true` | Active les exclusions de sécurité (Optimus, etc.) |
| `ForceGPU` | string | `""` | Forcer un GPU par son nom (override tout) |
| `UseVMA` | bool | `true` | Utiliser Vulkan Memory Allocator |
| `AvailableGPUs` | array | (runtime) | Liste de tous les GPUs détectés |

### 4.3 Notes

- `AvailableGPUs` est **recalculé à chaque démarrage** (pas persisté)
- Tous les GPUs sont listés, même ceux exclus par FailSafe
- Permet à l'utilisateur de copier-coller un nom pour `ForceGPU`

---

## 5. Comportement Runtime

### 5.1 Matrice des Comportements

| AutoSelectMode | EnableFailSafe | Config | Fullscreen | Résultat |
|----------------|----------------|--------|------------|----------|
| Performance | true | Laptop Optimus | Non | iGPU sélectionné |
| Performance | true | Laptop Optimus | Oui | dGPU sélectionné |
| Performance | true | Desktop hybrid | Any | dGPU sélectionné |
| Performance | false | Laptop Optimus | Any | dGPU sélectionné (risqué!) |
| PowerSaving | Any | Any | Any | iGPU sélectionné |
| DontCare | Any | Any | Any | Premier device |

### 5.2 Logs de Démarrage

#### Cas 1: Laptop Optimus en mode fenêtré (FailSafe actif)

```
[Info]    Physical device [iGPU] 'AMD Radeon(TM) Graphics' (Vendor: AMD (4098))
[Info]    Physical device [dGPU] 'NVIDIA GeForce RTX 3060 Laptop GPU' (Vendor: Nvidia (4318))
[Warning] Nvidia Optimus (LAPTOP) detected: iGPU 'AMD Radeon(TM) Graphics' + dGPU 'NVIDIA GeForce RTX 3060 Laptop GPU' (mobile). Failsafe mode will avoid dGPU in windowed mode.
[Warning] Failsafe: Excluding 'NVIDIA GeForce RTX 3060 Laptop GPU' (Nvidia Optimus + windowed mode - known driver issues with swap-chain resize)
[Success] Graphics capable physical device selected: AMD Radeon(TM) Graphics (Performance mode)
```

#### Cas 2: Laptop Optimus en fullscreen (FailSafe actif)

```
[Info]    Physical device [iGPU] 'AMD Radeon(TM) Graphics' (Vendor: AMD (4098))
[Info]    Physical device [dGPU] 'NVIDIA GeForce RTX 3060 Laptop GPU' (Vendor: Nvidia (4318))
[Warning] Nvidia Optimus (LAPTOP) detected: iGPU 'AMD Radeon(TM) Graphics' + dGPU 'NVIDIA GeForce RTX 3060 Laptop GPU' (mobile). Failsafe mode will avoid dGPU in windowed mode.
[Success] Graphics capable physical device selected: NVIDIA GeForce RTX 3060 Laptop GPU (Performance mode)
```

#### Cas 3: Desktop hybrid (FailSafe actif)

```
[Info]    Physical device [iGPU] 'Intel(R) UHD Graphics 770' (Vendor: Intel (32902))
[Info]    Physical device [dGPU] 'NVIDIA GeForce RTX 3070 Ti' (Vendor: Nvidia (4318))
[Info]    Hybrid GPU (Desktop, non-Optimus): iGPU 'Intel(R) UHD Graphics 770' + dGPU 'NVIDIA GeForce RTX 3070 Ti'. All modes safe.
[Success] Graphics capable physical device selected: NVIDIA GeForce RTX 3070 Ti (Performance mode)
```

#### Cas 4: GPU forcé

```
[Debug]   Trying to force the physical device (Graphics) named 'Intel(R) UHD Graphics 770' ...
[Success] Graphics capable physical device selected: Intel(R) UHD Graphics 770 (FORCED)
```

### 5.3 Priorité de Sélection

```
1. ForceGPU configuré → Utiliser ce GPU (override total)
2. EnableFailSafe + condition d'exclusion → Exclure le device
3. Scoring selon AutoSelectMode → Prendre le meilleur score
```

### 5.4 Extensibilité

La méthode `shouldExcludeForFailsafe()` est conçue pour accueillir d'autres cas d'exclusion :

```cpp
bool Instance::shouldExcludeForFailsafe(const HybridGPUConfig& config,
                                         const std::shared_ptr<PhysicalDevice>& device) const noexcept
{
    // Cas 1: Nvidia Optimus en mode fenêtré
    if (!m_fullscreenExclusiveEnabled && config.isOptimusDetected) { ... }

    // NOTE: Other exclusion cases will follow...

    return false;
}
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
