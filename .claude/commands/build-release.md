---
description: Build Release library (no tests, optimized for production)
---

Build the Emeraude Engine library in Release mode without tests.

**Task:**

1. **Configure Release:**
   ```bash
   mkdir -p .claude-build-release
   cd .claude-build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release -DEMERAUDE_ENABLE_TESTS:BOOL=OFF
   ```

2. **Build optimized:**
   ```bash
   cmake --build . --parallel $(nproc)
   ```

**Output:**
- Build progress
- Optimization warnings (if any)
- Build time
- Location of built library
- Success/failure status

**Use case:**
- Production library build
- Performance testing
- Release candidate verification
- Distribution packaging

**Note:** No tests in Release mode (tests only in Debug).