# Specification: Intelligent GPU Selection with Optimus Detection

**Version:** 2.0
**Date:** 2025-12-04
**Author:** Claude (assistant) / Sheldon
**Status:** Implemented

---

## Summary

1. [Context and Problem](#1-context-and-problem)
2. [Implemented Solution](#2-implemented-solution)
3. [Technical Architecture](#3-technical-architecture)
4. [Settings](#4-settings)
5. [Runtime Behavior](#5-runtime-behavior)
6. [References](#6-references)

---

## 1. Context and Problem

### 1.1 Identified Bug

On **Nvidia Optimus** configurations (laptop with Intel/AMD iGPU + mobile Nvidia dGPU), a **deadlock** occurs in the Nvidia driver during Windows WSI surface recreation. The problem occurs randomly during window resize or swapchain recreation.

### 1.2 Symptoms

- Complete application freeze
- Deadlock in `win32u.dll` (Nvidia driver callstack)
- `vkAcquireNextImageKHR` never returns
- Problem **not reproducible** with iGPU alone

### 1.3 Root Cause

Bug confirmed on Nvidia driver side for hybrid **laptop** configurations where the dGPU has no physical display outputs and routes through the iGPU. Documented in several projects:
- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820)
- [Dolphin Emulator #13522](https://bugs.dolphin-emu.org/issues/13522)
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091)

### 1.4 Manifestation Condition

**Important:** The bug only manifests during **swapchain recreation in windowed mode** (resize). In **exclusive fullscreen** mode, the Nvidia dGPU is safe because there is no dynamic surface resize.

### 1.5 Desktop vs Laptop

A desktop with a CPU having an integrated iGPU (e.g., Intel UHD) + a discrete RTX is **NOT** an Optimus system. On desktop, the RTX has its own display outputs (HDMI, DisplayPort) and does not route through the iGPU - no bug.

### 1.6 Solution

**FailSafe** system independent of selection mode, enabled by default, that automatically detects Optimus **laptop** configurations (mobile GPU) and excludes the problematic Nvidia dGPU **only in windowed mode**.

---

## 2. Implemented Solution

### 2.1 v2.0 Architecture

The system separates two distinct concerns:

| Concept | Setting | Role |
|---------|---------|------|
| **Scoring mode** | `AutoSelectMode` | Determines how to score GPUs (Performance, PowerSaving, DontCare) |
| **FailSafe protection** | `EnableFailSafe` | Enables/disables safety exclusions (Optimus, etc.) |

This separation allows FailSafe to be active regardless of the chosen scoring mode.

### 2.2 Selection Modes

| Mode | Behavior |
|------|----------|
| **DontCare** | First available device |
| **Performance** | Best score (favors dGPU) - **default** |
| **PowerSaving** | Inverted score (favors iGPU) |

### 2.3 FailSafe Protection

FailSafe is an **independent protection layer** that applies before scoring:

- **Enabled by default** (`EnableFailSafe = true`)
- Detects known problematic configurations
- Excludes affected devices from selection

### 2.4 Optimus Definition

A configuration is considered **Optimus** if:
1. Presence of a `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU` (Intel or AMD)
2. **AND** presence of a `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` with `vendorID == 0x10DE` (Nvidia)
3. **AND** the Nvidia GPU is a **mobile/laptop model** (detected by its name)

### 2.5 Exclusion Condition

The Nvidia dGPU is excluded **if and only if**:
- `EnableFailSafe = true`
- Optimus configuration detected
- **AND** exclusive fullscreen mode **not enabled**

In exclusive fullscreen, the Nvidia dGPU is usable even on Optimus.

### 2.6 Mobile GPU Detection

The `isLikelyMobileGPU()` method checks if the GPU name contains a mobile indicator:

| Pattern | Example |
|---------|---------|
| `"Mobile"` | "GeForce RTX 3080 Mobile" |
| `"Laptop"` | "GeForce RTX 4070 Laptop GPU" |
| `"Max-Q"` | "Quadro RTX 3000 Max-Q" |
| `" MX"` | "GeForce MX450" (MX series = always mobile) |

Desktop GPUs (e.g., "GeForce RTX 4080") never have these keywords.

### 2.7 Execution Flow

```
readSettings()
├── m_autoSelectMode = Performance | PowerSaving | DontCare
├── m_enableFailSafe = true (default)
├── m_fullscreenExclusiveEnabled
└── m_useVulkanMemoryAllocator

getScoredGraphicsDevices(window)
├── detectHybridGPUConfiguration() -> HybridGPUConfig
├── logGPUConfiguration(hybridConfig)
├── For each device:
│   ├── If m_enableFailSafe && shouldExcludeForFailsafe(hybridConfig, device) -> skip
│   ├── checkDeviceCompatibility() -> base score
│   └── modulateDeviceScoring() according to m_autoSelectMode
└── Return map sorted by score

getGraphicsDevice(window)
├── ForceGPU configured? -> Use that GPU
└── Otherwise -> Device with best score
```

---

## 3. Technical Architecture

### 3.1 Modified Files

| File | Modifications |
|------|---------------|
| `src/Vulkan/Types.hpp` | `DeviceAutoSelectMode` enum, `Vendor` enum, `HybridGPUConfig` struct |
| `src/Vulkan/Instance.hpp` | Members and methods |
| `src/Vulkan/Instance.cpp` | Detection and exclusion implementation |
| `src/SettingKeys.hpp` | Settings keys |

### 3.2 DeviceAutoSelectMode Enum

```cpp
enum class DeviceAutoSelectMode : uint8_t
{
    DontCare = 0,      // First available device
    Performance = 1,   // Favors dGPU (best score) - DEFAULT
    PowerSaving = 2    // Favors iGPU (inverted score)
};
```

### 3.3 Vendor Enum

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

### 3.4 HybridGPUConfig Structure

```cpp
struct HybridGPUConfig
{
    bool isOptimusDetected{false};      // Laptop: iGPU + mobile Nvidia dGPU
    bool isHybridNonOptimus{false};     // Desktop: CPU iGPU + discrete RTX (safe)
    std::string integratedGPUName{};
    std::string discreteGPUName{};
    uint32_t integratedVendorID{0};
    uint32_t discreteVendorID{0};
};
```

### 3.5 Key Methods

| Method | Role |
|--------|------|
| `readSettings()` | Reads settings at startup |
| `isLikelyMobileGPU(string)` | Detects if GPU name indicates a mobile model |
| `detectHybridGPUConfiguration()` | Detects iGPU + Nvidia dGPU + checks if mobile |
| `shouldExcludeForFailsafe(config, device)` | Returns true if GPU should be excluded |
| `logGPUConfiguration(config)` | Logs GPU config |
| `modulateDeviceScoring(props, score)` | Adjusts score according to m_autoSelectMode |

---

## 4. Settings

### 4.1 JSON Structure

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

### 4.2 Settings Description

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `AutoSelectMode` | string | `"Performance"` | Scoring mode: Performance, PowerSaving, DontCare |
| `EnableFailSafe` | bool | `true` | Enables safety exclusions (Optimus, etc.) |
| `ForceGPU` | string | `""` | Force a GPU by name (overrides everything) |
| `UseVMA` | bool | `true` | Use Vulkan Memory Allocator |
| `AvailableGPUs` | array | (runtime) | List of all detected GPUs |

### 4.3 Notes

- `AvailableGPUs` is **recalculated at each startup** (not persisted)
- All GPUs are listed, even those excluded by FailSafe
- Allows user to copy-paste a name for `ForceGPU`

---

## 5. Runtime Behavior

### 5.1 Behavior Matrix

| AutoSelectMode | EnableFailSafe | Config | Fullscreen | Result |
|----------------|----------------|--------|------------|--------|
| Performance | true | Laptop Optimus | No | iGPU selected |
| Performance | true | Laptop Optimus | Yes | dGPU selected |
| Performance | true | Desktop hybrid | Any | dGPU selected |
| Performance | false | Laptop Optimus | Any | dGPU selected (risky!) |
| PowerSaving | Any | Any | Any | iGPU selected |
| DontCare | Any | Any | Any | First device |

### 5.2 Startup Logs

#### Case 1: Laptop Optimus in windowed mode (FailSafe active)

```
[Info]    Physical device [iGPU] 'AMD Radeon(TM) Graphics' (Vendor: AMD (4098))
[Info]    Physical device [dGPU] 'NVIDIA GeForce RTX 3060 Laptop GPU' (Vendor: Nvidia (4318))
[Warning] Nvidia Optimus (LAPTOP) detected: iGPU 'AMD Radeon(TM) Graphics' + dGPU 'NVIDIA GeForce RTX 3060 Laptop GPU' (mobile). Failsafe mode will avoid dGPU in windowed mode.
[Warning] Failsafe: Excluding 'NVIDIA GeForce RTX 3060 Laptop GPU' (Nvidia Optimus + windowed mode - known driver issues with swap-chain resize)
[Success] Graphics capable physical device selected: AMD Radeon(TM) Graphics (Performance mode)
```

#### Case 2: Laptop Optimus in fullscreen (FailSafe active)

```
[Info]    Physical device [iGPU] 'AMD Radeon(TM) Graphics' (Vendor: AMD (4098))
[Info]    Physical device [dGPU] 'NVIDIA GeForce RTX 3060 Laptop GPU' (Vendor: Nvidia (4318))
[Warning] Nvidia Optimus (LAPTOP) detected: iGPU 'AMD Radeon(TM) Graphics' + dGPU 'NVIDIA GeForce RTX 3060 Laptop GPU' (mobile). Failsafe mode will avoid dGPU in windowed mode.
[Success] Graphics capable physical device selected: NVIDIA GeForce RTX 3060 Laptop GPU (Performance mode)
```

#### Case 3: Desktop hybrid (FailSafe active)

```
[Info]    Physical device [iGPU] 'Intel(R) UHD Graphics 770' (Vendor: Intel (32902))
[Info]    Physical device [dGPU] 'NVIDIA GeForce RTX 3070 Ti' (Vendor: Nvidia (4318))
[Info]    Hybrid GPU (Desktop, non-Optimus): iGPU 'Intel(R) UHD Graphics 770' + dGPU 'NVIDIA GeForce RTX 3070 Ti'. All modes safe.
[Success] Graphics capable physical device selected: NVIDIA GeForce RTX 3070 Ti (Performance mode)
```

#### Case 4: Forced GPU

```
[Debug]   Trying to force the physical device (Graphics) named 'Intel(R) UHD Graphics 770' ...
[Success] Graphics capable physical device selected: Intel(R) UHD Graphics 770 (FORCED)
```

### 5.3 Selection Priority

```
1. ForceGPU configured -> Use that GPU (total override)
2. EnableFailSafe + exclusion condition -> Exclude the device
3. Scoring according to AutoSelectMode -> Take the best score
```

### 5.4 Extensibility

The `shouldExcludeForFailsafe()` method is designed to accommodate other exclusion cases:

```cpp
bool Instance::shouldExcludeForFailsafe(const HybridGPUConfig& config,
                                         const std::shared_ptr<PhysicalDevice>& device) const noexcept
{
    // Case 1: Nvidia Optimus in windowed mode
    if (!m_fullscreenExclusiveEnabled && config.isOptimusDetected) { ... }

    // NOTE: Other exclusion cases will follow...

    return false;
}
```

---

## 6. References

### 6.1 Bug Reports

- [SDL #11820](https://github.com/libsdl-org/SDL/issues/11820) - Swapchain destruction corrupts driver state
- [Dolphin #13522](https://bugs.dolphin-emu.org/issues/13522) - Resize deadlock with Nvidia Vulkan
- [Gamescope #1091](https://github.com/ValveSoftware/gamescope/issues/1091) - WSI layer deadlock

### 6.2 Documentation

- [Vulkan Tutorial - Swapchain Recreation](https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation)
- [ArchWiki - NVIDIA Optimus](https://wiki.archlinux.org/title/NVIDIA_Optimus)
