---
description: Verify all critical Emeraude Engine conventions are respected
---

Comprehensive check of all critical engine conventions across the codebase.

**Usage:**
- No argument: Full scan of all conventions
- With file path: Focused scan on specific file(s)

## Conventions Checked

### 1. Y-Down Coordinate System (CRITICAL)

**Directories:** Physics/, Graphics/, Audio/, Scenes/

**Correct values:**
- Gravity: `+9.81` (down is positive Y)
- Up vector: `vec3(0, -1, 0)`
- Down vector: `vec3(0, 1, 0)`
- Jump impulse: Negative Y

**Suspicious patterns to flag:**
```
# Incorrect gravity
gravity.*-[0-9]           # BAD: -9.81
-9\.81|-9\.8              # BAD: hardcoded negative

# Y-axis flips
\.y\s*\*=\s*-1            # BAD: y *= -1
y\s*=\s*-y                # BAD: y = -y
invert.*[Yy]|flip.*[Yy]   # BAD: invertY(), flipY()

# Y-up assumptions
[Yy]-up                   # BAD: comments mentioning Y-up
toOpenGL|fromVulkan       # BAD: coordinate conversions
```

### 2. Fail-Safe Resources

**Directory:** Resources/

**Check:**
- All ResourceTrait implementations have `load(ServiceProvider&)` method
- No `nullptr` returns from resource loading
- Neutral/fallback resources always available

**Patterns:**
```
return\s+nullptr          # BAD: should never return nullptr
if\s*\(.*==\s*nullptr     # WARNING: null checks suggest possible nullptr
```

### 3. Libs Isolation

**Directory:** src/Libs/

**Check:** Libs must NOT include high-level systems:
```
#include.*"Scenes/        # BAD
#include.*"Physics/       # BAD
#include.*"Graphics/      # BAD
#include.*"Resources/     # BAD
#include.*"Audio/         # BAD
```

Libs/ is foundational - it must remain dependency-free from engine systems.

### 4. Vulkan Abstraction

**Directories:** Graphics/, Resources/, Scenes/

**Check:** No direct Vulkan API calls outside src/Vulkan/:
```
vkCreate|vkDestroy|vkCmd  # BAD: direct Vulkan calls
VkDevice|VkBuffer|VkImage # WARNING: direct Vulkan types (should use abstractions)
```

All Vulkan interaction must go through src/Vulkan/ abstractions.

### 5. Platform Isolation

**Directory:** PlatformSpecific/

**Check:** Platform code properly isolated:
- Windows code in Windows/
- Linux code in Linux/
- macOS code in macOS/
- No cross-contamination

```
#ifdef _WIN32             # Should only appear in Windows/ or guarded sections
#ifdef __APPLE__          # Should only appear in macOS/ or guarded sections
#ifdef __linux__          # Should only appear in Linux/ or guarded sections
```

## Output Format

```
Emeraude Conventions Check
==========================

[1/5] Y-Down Coordinate System
    OK - No violations found
    OR
    WARNING - [N] potential violations:
      - src/Physics/Gravity.cpp:45 - gravity = -9.81f (should be +9.81f)
      - src/Scenes/Camera.cpp:123 - position.y *= -1 (suspicious Y flip)

[2/5] Fail-Safe Resources
    OK - No violations found

[3/5] Libs Isolation
    OK - No high-level includes in Libs/

[4/5] Vulkan Abstraction
    OK - No direct Vulkan calls outside Vulkan/

[5/5] Platform Isolation
    OK - Platform code properly isolated

==========================
Summary: [X/5] conventions clean
Status: OK / WARNINGS / CRITICAL
```

## Grep Commands

```bash
# Y-down violations
grep -rn "gravity.*-[0-9]" src/Physics/ src/Graphics/ src/Scenes/
grep -rn "\-9\.81\|\-9\.8" src/
grep -rn "\.y\s*\*=\s*-1" src/
grep -rn -i "invert.*y\|flip.*y" src/
grep -rn -i "y-up" src/

# Fail-safe violations
grep -rn "return\s*nullptr" src/Resources/

# Libs isolation
grep -rn '#include.*"Scenes/\|#include.*"Physics/\|#include.*"Graphics/' src/Libs/

# Vulkan abstraction
grep -rn "vkCreate\|vkDestroy\|vkCmd" src/Graphics/ src/Resources/ src/Scenes/

# Platform isolation
grep -rn "#ifdef _WIN32" src/PlatformSpecific/Linux/ src/PlatformSpecific/macOS/
grep -rn "#ifdef __APPLE__" src/PlatformSpecific/Linux/ src/PlatformSpecific/Windows/
grep -rn "#ifdef __linux__" src/PlatformSpecific/Windows/ src/PlatformSpecific/macOS/
```

## Severity Levels

| Level | Meaning |
|-------|---------|
| **CRITICAL** | Y-down violation, must fix immediately |
| **WARNING** | Potential issue, needs review |
| **INFO** | Suspicious but may be intentional |

Y-down violations are always CRITICAL as they affect the entire engine's coordinate system.