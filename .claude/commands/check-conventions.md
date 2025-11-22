---
description: Vérifie le respect des conventions critiques du moteur
---

Verify that critical engine conventions are respected across the codebase.

**Conventions to check:**

1. **Y-down coordinate system (CRITICAL)**
   - Search Physics/, Graphics/, Audio/, Scenes/ for suspicious patterns:
     - Hardcoded `-9.81` (should be `+9.81`)
     - Y-axis flips or conversions
     - Comments mentioning "flip Y" or "invert Y"
   - Flag any violations

2. **Fail-safe Resources**
   - Check Resources/ for ResourceTrait implementations
   - Verify all have `load(ServiceProvider&)` neutral resource method
   - Check for nullptr returns (should never happen)

3. **Libs isolation**
   - Verify Libs/ has NO includes from high-level systems:
     - ❌ No `#include "Scenes/`
     - ❌ No `#include "Physics/`
     - ❌ No `#include "Graphics/`
   - Libs must remain foundational

4. **Vulkan abstraction**
   - Check Graphics/, Resources/ for direct Vulkan calls
   - Should use Vulkan/ abstractions only
   - Flag any `vk*` function calls outside Vulkan/

5. **Platform isolation**
   - Check PlatformSpecific/ for contamination
   - Windows code shouldn't touch Linux/macOS sections

**Output format:**
```
✅ Convention Check Results

[Convention Name]:
✅ No violations found
OR
⚠️ [N] potential violations:
  - file.cpp:123 - [description]
  - file.hpp:45 - [description]

Summary: [X/5] conventions clean
```

Run checks using Grep with appropriate patterns for each convention.
