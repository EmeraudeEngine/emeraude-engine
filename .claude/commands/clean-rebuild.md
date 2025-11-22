---
description: Clean rebuild Debug (supprime build directory + reconfigure + build + tests)
---

Complete clean rebuild from scratch in Debug mode with tests.

**Task:**

1. **Remove build directory:**
   ```bash
   rm -rf .claude-build-debug
   ```

2. **Fresh configure:**
   ```bash
   mkdir .claude-build-debug
   cd .claude-build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON
   ```

3. **Build from scratch:**
   ```bash
   cmake --build . --parallel $(nproc)
   ```

4. **Run tests (optional filter):**
   - No argument: All tests
   - With argument: Filtered tests
   ```bash
   ctest --parallel $(nproc) --output-on-failure [-R <pattern>]
   ```

**Examples:**
- `/clean-rebuild` → Clean all, rebuild all, test all
- `/clean-rebuild Physics` → Clean all, rebuild all, test Physics only

**Output:**
- Confirmation of clean
- Full build progress
- Test results
- Total time (clean + build + test)

**Use case:**
- CMake configuration changes
- Suspicious build issues
- After updating dependencies
- Nuclear option when incremental build acts weird

**Warning:** This is the slowest option - deletes everything and starts fresh.