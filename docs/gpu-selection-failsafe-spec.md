# Spécification : Sélection GPU Intelligente avec Détection Optimus

**Version:** 1.0
**Date:** 2025-12-02
**Auteur:** Claude (assistant) / Sheldon
**Statut:** Proposition

---

## Sommaire

1. [Contexte et Problématique](#1-contexte-et-problématique)
2. [État Actuel](#2-état-actuel)
3. [Solution Proposée](#3-solution-proposée)
4. [Modifications Techniques](#4-modifications-techniques)
5. [Logging et Diagnostic](#5-logging-et-diagnostic)
6. [Matrice des Comportements](#6-matrice-des-comportements)
7. [UX côté Lychee](#7-ux-côté-lychee)
8. [Plan d'Implémentation](#8-plan-dimplémentation)
9. [Références](#9-références)

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
- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820) - Corruption état driver lors destruction swapchain
- [Dolphin Emulator #13522](https://bugs.dolphin-emu.org/issues/13522) - Deadlock resize + Nvidia Vulkan
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091) - Deadlock WSI avec oldSwapchain

### 1.4 Solution Temporaire Actuelle

Forcer manuellement l'iGPU dans le fichier de configuration :
```json
"ForceGPU": "AMD Radeon(TM) Graphics"
```

### 1.5 Objectif

Automatiser la détection des configurations Optimus et proposer un mode **Failsafe** qui évite automatiquement le dGPU Nvidia problématique.

---

## 2. État Actuel

### 2.1 Architecture Existante

| Composant | Fichier | Description |
|-----------|---------|-------------|
| `DeviceRunMode` | `src/Vulkan/Types.hpp` | Enum des modes de sélection |
| `PhysicalDevice` | `src/Vulkan/PhysicalDevice.hpp/.cpp` | Wrapper GPU Vulkan |
| `Instance` | `src/Vulkan/Instance.hpp/.cpp` | Sélection et scoring GPU |
| Settings | `src/SettingKeys.hpp` | Clés de configuration |

### 2.2 Enum Actuel

```cpp
enum class DeviceRunMode : uint8_t
{
    DontCare = 0,
    Performance = 1,
    PowerSaving = 2,
};
```

### 2.3 Système de Scoring Actuel

Le scoring GPU fonctionne par multiplicateurs selon le type de device :

| Mode | Discrete GPU | Integrated GPU | Virtual GPU | CPU |
|------|--------------|----------------|-------------|-----|
| Performance | ×5 | ×3 | ×2 | ×1 |
| PowerSaving | ×1 | ×5 | ×2 | ×3 |
| DontCare | ×3 | ×2 | ×1 | ×1 |

### 2.4 Settings JSON Actuels

```json
{
  "Core": {
    "Video": {
      "VulkanDevice": {
        "AutoSelectMode": "Performance",
        "ForceGPU": "",
        "UseVMA": true,
        "AvailableGPUs": ["GPU1", "GPU2"]
      }
    }
  }
}
```

### 2.5 Limitations Actuelles

- Pas de détection automatique des configurations Optimus
- Pas de mode qui combine "Performance si possible, iGPU si Optimus"
- L'utilisateur doit connaître le problème et forcer manuellement l'iGPU

---

## 3. Solution Proposée

### 3.1 Nouveau Mode : Failsafe

Ajouter un quatrième mode de sélection GPU qui :
1. Détecte automatiquement les configurations Optimus
2. Se comporte comme **Performance** si pas d'Optimus
3. Exclut le dGPU Nvidia et utilise l'iGPU si Optimus détecté

### 3.2 Définition Optimus

Une configuration est considérée **Optimus** si :
- Présence d'un `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` (Intel ou AMD)
- **ET** présence d'un `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` avec `vendorID == 0x10DE` (Nvidia)

### 3.3 Comportement Failsafe

```
SI mode == Failsafe:
    SI Optimus détecté ET OptimusWorkaround activé:
        → Exclure dGPU Nvidia (score = 0)
        → Favoriser iGPU (×5)
        → Logger un warning explicatif
    SINON:
        → Comportement identique à Performance
```

---

## 4. Modifications Techniques

### 4.1 Enum `DeviceRunMode` (Types.hpp)

```cpp
/**
 * @brief Modes de sélection automatique du GPU Vulkan.
 */
enum class DeviceRunMode : uint8_t
{
    /** @brief Sélection basique sans préférence. */
    DontCare = 0,

    /** @brief Favorise le GPU le plus performant (discrete). */
    Performance = 1,

    /** @brief Favorise le GPU le plus économe (integrated). */
    PowerSaving = 2,

    /**
     * @brief Mode sécurisé : Performance SAUF si Optimus détecté.
     * @details Sur configurations Nvidia Optimus, exclut automatiquement
     *          le dGPU Nvidia pour éviter les bugs driver connus.
     */
    Failsafe = 3
};
```

### 4.2 Structure de Détection Hybride (Instance.hpp)

```cpp
/**
 * @brief Informations sur la configuration GPU hybride détectée.
 */
struct HybridGPUConfig
{
    /** @brief True si configuration Nvidia Optimus détectée (iGPU + dGPU Nvidia). */
    bool isOptimusDetected{false};

    /** @brief True si configuration AMD Hybrid détectée (futur). */
    bool isAMDHybridDetected{false};

    /** @brief Nom du GPU intégré détecté. */
    std::string integratedGPUName{};

    /** @brief Nom du GPU discret détecté. */
    std::string discreteGPUName{};

    /** @brief Vendor ID du GPU intégré (Intel: 0x8086, AMD: 0x1002). */
    uint32_t integratedVendorID{0};

    /** @brief Vendor ID du GPU discret (Nvidia: 0x10DE, AMD: 0x1002). */
    uint32_t discreteVendorID{0};
};
```

### 4.3 Nouvelles Méthodes (Instance.hpp)

```cpp
class Instance final : public ServiceInterface
{
public:
    // ... existing methods ...

    /**
     * @brief Détecte les configurations GPU hybrides.
     * @return Structure contenant les informations de détection.
     */
    [[nodiscard]] HybridGPUConfig detectHybridGPUConfiguration() const noexcept;

    /**
     * @brief Retourne la configuration hybride détectée.
     * @return Référence constante vers HybridGPUConfig.
     */
    [[nodiscard]] const HybridGPUConfig& getHybridConfig() const noexcept;

    /**
     * @brief Vérifie si un GPU doit être exclu en mode Failsafe.
     * @param physicalDevice Le GPU à évaluer.
     * @return True si le GPU doit être exclu.
     */
    [[nodiscard]] bool shouldExcludeForFailsafe(
        const PhysicalDevice& physicalDevice) const noexcept;

private:
    HybridGPUConfig m_hybridConfig{};
};
```

### 4.4 Implémentation Détection (Instance.cpp)

```cpp
HybridGPUConfig
Instance::detectHybridGPUConfiguration() const noexcept
{
    HybridGPUConfig config{};

    const PhysicalDevice* integratedGPU = nullptr;
    const PhysicalDevice* nvidiaDiscreteGPU = nullptr;

    for (const auto& device : m_physicalDevices)
    {
        const auto& props = device->properties();
        const auto deviceType = props.deviceType;
        const auto vendorID = props.vendorID;

        // Détection iGPU (Intel: 0x8086, AMD: 0x1002)
        if (deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            integratedGPU = device.get();
            config.integratedGPUName = props.deviceName;
            config.integratedVendorID = vendorID;
        }

        // Détection dGPU Nvidia (0x10DE)
        if (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            vendorID == VendorID::Nvidia)
        {
            nvidiaDiscreteGPU = device.get();
            config.discreteGPUName = props.deviceName;
            config.discreteVendorID = vendorID;
        }
    }

    // Optimus = iGPU présent + dGPU Nvidia présent
    config.isOptimusDetected = (integratedGPU != nullptr) &&
                                (nvidiaDiscreteGPU != nullptr);

    return config;
}

bool
Instance::shouldExcludeForFailsafe(const PhysicalDevice& physicalDevice) const noexcept
{
    if (!m_hybridConfig.isOptimusDetected)
    {
        return false;  // Pas d'Optimus, pas d'exclusion
    }

    const auto& props = physicalDevice.properties();

    // Exclure uniquement le dGPU Nvidia sur config Optimus
    return (props.vendorID == VendorID::Nvidia) &&
           (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
}
```

### 4.5 Modification du Scoring (Instance.cpp)

Dans la méthode `modulateDeviceScoring()` :

```cpp
size_t
Instance::modulateDeviceScoring(
    size_t baseScore,
    const PhysicalDevice& device,
    DeviceRunMode mode) const noexcept
{
    const auto& props = device.properties();
    size_t modulation = 1;

    switch (mode)
    {
        case DeviceRunMode::Performance:
            modulation = getPerformanceModulation(props.deviceType);
            break;

        case DeviceRunMode::PowerSaving:
            modulation = getPowerSavingModulation(props.deviceType);
            break;

        case DeviceRunMode::DontCare:
            modulation = getDontCareModulation(props.deviceType);
            break;

        case DeviceRunMode::Failsafe:
        {
            if (shouldExcludeForFailsafe(device))
            {
                // Log explicatif
                TraceWarning{ClassId} <<
                    "Failsafe: Excluding '" << props.deviceName <<
                    "' (Nvidia Optimus configuration detected - known driver issues)";
                return 0;  // Score nul = exclusion totale
            }

            if (m_hybridConfig.isOptimusDetected)
            {
                // Optimus détecté : favoriser iGPU
                modulation = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    ? 5 : 1;
            }
            else
            {
                // Pas d'Optimus : comportement Performance
                modulation = getPerformanceModulation(props.deviceType);
            }
            break;
        }
    }

    return baseScore * modulation;
}
```

### 4.6 Nouvelles Clés Settings (SettingKeys.hpp)

```cpp
namespace Emeraude::SettingKeys
{
    // === VulkanDevice Settings ===

    /** @brief Mode de sélection automatique du GPU. */
    constexpr auto VkDeviceAutoSelectModeKey = "Core/Video/VulkanDevice/AutoSelectMode";

    /** @brief Valeur par défaut : Failsafe (nouveau défaut recommandé). */
    constexpr auto DefaultVkDeviceAutoSelectMode = "Failsafe";

    /** @brief Forcer un GPU spécifique par son nom. */
    constexpr auto VkDeviceForceGPUKey = "Core/Video/VulkanDevice/ForceGPU";

    /** @brief Liste des GPUs disponibles (read-only, peuplé au runtime). */
    constexpr auto VkDeviceAvailableGPUsKey = "Core/Video/VulkanDevice/AvailableGPUs";

    /** @brief Utiliser Vulkan Memory Allocator. */
    constexpr auto VkDeviceUseVMAKey = "Core/Video/VulkanDevice/UseVMA";
    constexpr auto DefaultVkDeviceUseVMA = true;

    // === Nouvelles clés Optimus ===

    /** @brief Activer le workaround Optimus en mode Failsafe. */
    constexpr auto VkDeviceOptimusWorkaroundKey = "Core/Video/VulkanDevice/OptimusWorkaround";
    constexpr auto DefaultVkDeviceOptimusWorkaround = true;

    /** @brief Configuration Optimus détectée (read-only, peuplé au runtime). */
    constexpr auto VkDeviceOptimusDetectedKey = "Core/Video/VulkanDevice/OptimusDetected";

    /** @brief GPU sélectionné et raison (read-only, peuplé au runtime). */
    constexpr auto VkDeviceSelectedGPUKey = "Core/Video/VulkanDevice/SelectedGPU";
    constexpr auto VkDeviceSelectionReasonKey = "Core/Video/VulkanDevice/SelectionReason";
}
```

### 4.7 Structure Settings JSON Finale

```json
{
  "Core": {
    "Video": {
      "VulkanDevice": {
        "AutoSelectMode": "Failsafe",
        "ForceGPU": "",
        "UseVMA": true,
        "OptimusWorkaround": true,
        "AvailableGPUs": [
          "AMD Radeon(TM) Graphics",
          "NVIDIA GeForce RTX 3060 Laptop GPU"
        ],
        "OptimusDetected": true,
        "SelectedGPU": "AMD Radeon(TM) Graphics",
        "SelectionReason": "Failsafe mode - Optimus excluded Nvidia dGPU"
      }
    }
  }
}
```

---

## 5. Logging et Diagnostic

### 5.1 Log au Démarrage

```cpp
void Instance::logGPUConfiguration() const noexcept
{
    TraceInfo{ClassId} << "========== GPU Configuration ==========";
    TraceInfo{ClassId} << "Physical devices found: " << m_physicalDevices.size();

    for (const auto& device : m_physicalDevices)
    {
        const auto& props = device->properties();
        TraceInfo{ClassId}
            << "  [" << getDeviceTypeShortName(props.deviceType) << "] "
            << props.deviceName
            << " (Vendor: " << getVendorName(props.vendorID) << ")";
    }

    TraceInfo{ClassId} << "---------------------------------------";

    if (m_hybridConfig.isOptimusDetected)
    {
        TraceWarning{ClassId} << "! Nvidia Optimus configuration DETECTED";
        TraceWarning{ClassId} << "  Integrated : " << m_hybridConfig.integratedGPUName;
        TraceWarning{ClassId} << "  Discrete   : " << m_hybridConfig.discreteGPUName;
        TraceWarning{ClassId} << "  Known driver issues may occur with discrete GPU";
    }
    else
    {
        TraceInfo{ClassId} << "Hybrid GPU config: None detected";
    }

    TraceInfo{ClassId} << "Selection mode: " << getModeName(m_deviceRunMode);
    TraceInfo{ClassId} << "Optimus workaround: "
        << (m_optimusWorkaroundEnabled ? "Enabled" : "Disabled");
    TraceInfo{ClassId} << "=======================================";
}
```

### 5.2 Log lors de la Sélection

```cpp
void Instance::logDeviceSelection(
    const PhysicalDevice& selected,
    size_t score,
    const std::string& reason) const noexcept
{
    TraceInfo{ClassId} << ">>> GPU Selected: " << selected.properties().deviceName;
    TraceInfo{ClassId} << "    Score  : " << score;
    TraceInfo{ClassId} << "    Reason : " << reason;

    if (m_hybridConfig.isOptimusDetected &&
        m_deviceRunMode == DeviceRunMode::Failsafe)
    {
        TraceInfo{ClassId} << "    Note   : Nvidia dGPU excluded (Optimus Failsafe)";
    }
}
```

### 5.3 Raisons de Sélection

| Code | Message |
|------|---------|
| `HIGHEST_SCORE` | "Highest scoring device" |
| `FORCED_CONFIG` | "Forced by ForceGPU configuration" |
| `FAILSAFE_OPTIMUS` | "Failsafe mode - Optimus excluded Nvidia dGPU" |
| `ONLY_COMPATIBLE` | "Only compatible device available" |

---

## 6. Matrice des Comportements

### 6.1 Comportement par Mode

| Mode | Optimus | dGPU Nvidia | iGPU | Résultat |
|------|---------|-------------|------|----------|
| **Performance** | Non | Score ×5 | Score ×3 | dGPU sélectionné |
| **Performance** | Oui | Score ×5 | Score ×3 | dGPU sélectionné (risque crash) |
| **PowerSaving** | Non | Score ×1 | Score ×5 | iGPU sélectionné |
| **PowerSaving** | Oui | Score ×1 | Score ×5 | iGPU sélectionné |
| **DontCare** | Non | Score ×3 | Score ×2 | Selon scores bruts |
| **DontCare** | Oui | Score ×3 | Score ×2 | Selon scores bruts (risque crash) |
| **Failsafe** | Non | Score ×5 | Score ×3 | dGPU sélectionné |
| **Failsafe** | Oui | **Score = 0** | Score ×5 | **iGPU sélectionné (safe)** |

### 6.2 Priorité de Sélection

```
1. ForceGPU configuré et GPU trouvé → Utiliser ce GPU
2. Mode Failsafe + Optimus → Exclure dGPU Nvidia, prendre iGPU
3. Scoring standard selon le mode → Prendre le score le plus élevé
4. Aucun GPU compatible → Erreur fatale
```

### 6.3 Vendor IDs de Référence

| Vendor | ID | Constante |
|--------|-------|-----------|
| AMD | `0x1002` | `VendorID::AMD` |
| Intel | `0x8086` | `VendorID::Intel` |
| Nvidia | `0x10DE` | `VendorID::Nvidia` |
| ARM | `0x13B5` | `VendorID::ARM` |
| Qualcomm | `0x5143` | `VendorID::Qualcomm` |

---

## 7. UX côté Lychee

### 7.1 Interface Settings Proposée

```
┌─────────────────────────────────────────────────────────┐
│ Graphics Settings                                       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│ GPU Selection Mode:                                     │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ ● Automatic (Recommended)                           │ │
│ │ ○ Maximum Performance                               │ │
│ │ ○ Power Saving                                      │ │
│ │ ○ Manual Selection                                  │ │
│ └─────────────────────────────────────────────────────┘ │
│                                                         │
│ [If Manual selected]                                    │
│ Select GPU:                                             │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ AMD Radeon(TM) Graphics              [Integrated] ▼ │ │
│ └─────────────────────────────────────────────────────┘ │
│                                                         │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ ⚠ Nvidia Optimus configuration detected.            │ │
│ │   Some Nvidia driver versions may cause freezes.    │ │
│ │   "Automatic" mode will use the integrated GPU.     │ │
│ └─────────────────────────────────────────────────────┘ │
│                                                         │
│ Current GPU: AMD Radeon(TM) Graphics                    │
│ Status: ✓ Active (Failsafe mode)                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 7.2 Mapping UI → Config

| UI Option | `AutoSelectMode` | `ForceGPU` |
|-----------|------------------|------------|
| Automatic (Recommended) | `"Failsafe"` | `""` |
| Maximum Performance | `"Performance"` | `""` |
| Power Saving | `"PowerSaving"` | `""` |
| Manual Selection | `"DontCare"` | `"<GPU Name>"` |

### 7.3 Warning Conditionnel

Afficher un warning si :
- `OptimusDetected == true`
- **ET** user sélectionne "Maximum Performance" ou sélectionne manuellement le dGPU Nvidia

```
⚠ Warning: Your Nvidia Optimus configuration may cause
instability with this GPU. The integrated GPU is recommended.

[ Use Anyway ] [ Switch to Automatic ]
```

### 7.4 Status Bar / About

```
GPU: AMD Radeon(TM) Graphics
Mode: Failsafe (Optimus workaround active)
```

---

## 8. Plan d'Implémentation

### Phase 1 : Core Engine (Emeraude)

| # | Tâche | Fichier | Priorité |
|---|-------|---------|----------|
| 1 | Ajouter `Failsafe` à `DeviceRunMode` | `Types.hpp` | Haute |
| 2 | Ajouter `HybridGPUConfig` struct | `Instance.hpp` | Haute |
| 3 | Implémenter `detectHybridGPUConfiguration()` | `Instance.cpp` | Haute |
| 4 | Implémenter `shouldExcludeForFailsafe()` | `Instance.cpp` | Haute |
| 5 | Modifier `modulateDeviceScoring()` | `Instance.cpp` | Haute |
| 6 | Ajouter nouvelles clés settings | `SettingKeys.hpp` | Moyenne |
| 7 | Implémenter logging amélioré | `Instance.cpp` | Moyenne |
| 8 | Changer défaut à `Failsafe` | `SettingKeys.hpp` | Moyenne |
| 9 | Écrire settings read-only au runtime | `Instance.cpp` | Basse |

### Phase 2 : Tests

| # | Test | Description |
|---|------|-------------|
| 1 | Unit test détection | Mock 2 GPUs, vérifier `isOptimusDetected` |
| 2 | Unit test scoring Failsafe | Vérifier score = 0 pour Nvidia si Optimus |
| 3 | Integration test machine Optimus | Test réel sur laptop avec RTX + iGPU |
| 4 | Test régression Performance | Vérifier que Performance fonctionne toujours |

### Phase 3 : Lychee UI (Optionnel)

| # | Tâche | Description |
|---|-------|-------------|
| 1 | Dropdown GPU mode | 4 options avec "Automatic" par défaut |
| 2 | Dropdown GPU manuel | Liste des GPUs disponibles |
| 3 | Warning Optimus | Afficher si Optimus + choix risqué |
| 4 | Status GPU | Afficher GPU actif + mode |

---

## 9. Références

### 9.1 Bug Reports Nvidia Optimus + Vulkan

- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820) - Swapchain destruction corrupts driver state
- [Dolphin #13522](https://bugs.dolphin-emu.org/issues/13522) - Resize deadlock with Nvidia Vulkan
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091) - WSI layer deadlock
- [NVIDIA Forums](https://forums.developer.nvidia.com/t/windows-possible-vulkan-driver-bug-with-recent-driver-versions-likely-related-to-swapchain/69163) - Fullscreen switch hang

### 9.2 Documentation Vulkan

- [Vulkan Tutorial - Swapchain Recreation](https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation)
- [Khronos VK_EXT_full_screen_exclusive](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_full_screen_exclusive.html)

### 9.3 Nvidia Optimus Documentation

- [ArchWiki - NVIDIA Optimus](https://wiki.archlinux.org/title/NVIDIA_Optimus)
- [NVIDIA Developer - Vulkan Driver](https://developer.nvidia.com/vulkan-driver)

---

## Annexe A : Exemple de Log Complet

```
[INFO]  [Vulkan::Instance] ========== GPU Configuration ==========
[INFO]  [Vulkan::Instance] Physical devices found: 2
[INFO]  [Vulkan::Instance]   [iGPU] AMD Radeon(TM) Graphics (Vendor: AMD)
[INFO]  [Vulkan::Instance]   [dGPU] NVIDIA GeForce RTX 3060 Laptop GPU (Vendor: Nvidia)
[INFO]  [Vulkan::Instance] ---------------------------------------
[WARN]  [Vulkan::Instance] ! Nvidia Optimus configuration DETECTED
[WARN]  [Vulkan::Instance]   Integrated : AMD Radeon(TM) Graphics
[WARN]  [Vulkan::Instance]   Discrete   : NVIDIA GeForce RTX 3060 Laptop GPU
[WARN]  [Vulkan::Instance]   Known driver issues may occur with discrete GPU
[INFO]  [Vulkan::Instance] Selection mode: Failsafe
[INFO]  [Vulkan::Instance] Optimus workaround: Enabled
[INFO]  [Vulkan::Instance] =======================================
[WARN]  [Vulkan::Instance] Failsafe: Excluding 'NVIDIA GeForce RTX 3060 Laptop GPU' (Nvidia Optimus configuration detected - known driver issues)
[INFO]  [Vulkan::Instance] >>> GPU Selected: AMD Radeon(TM) Graphics
[INFO]  [Vulkan::Instance]     Score  : 15420
[INFO]  [Vulkan::Instance]     Reason : Failsafe mode - Optimus excluded Nvidia dGPU
[INFO]  [Vulkan::Instance]     Note   : Nvidia dGPU excluded (Optimus Failsafe)
```

---

## Annexe B : Rétrocompatibilité

| Ancien Setting | Nouveau Comportement |
|----------------|---------------------|
| `AutoSelectMode: "Performance"` | Inchangé - dGPU préféré (même si Optimus) |
| `AutoSelectMode: "PowerSaving"` | Inchangé - iGPU préféré |
| `AutoSelectMode: "DontCare"` | Inchangé - scoring neutre |
| `ForceGPU: "<name>"` | Inchangé - override total |
| (nouveau défaut) | `"Failsafe"` au lieu de `"Performance"` |

**Note:** Les utilisateurs existants avec `AutoSelectMode: "Performance"` explicite dans leur config ne seront pas affectés. Seuls les nouveaux utilisateurs (ou ceux sans config) auront `Failsafe` par défaut.
