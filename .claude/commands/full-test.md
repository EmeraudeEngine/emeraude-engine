---
description: Full rebuild Debug + run ALL tests (comprehensive verification)
---

Complete rebuild in Debug mode with all tests - for comprehensive verification before commit.

**Task:**

1. **Clean build:**
   ```bash
   rm -rf .claude-build-debug
   mkdir .claude-build-debug
   cd .claude-build-debug
   ```

2. **Configure:**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON
   ```

3. **Build from scratch:**
   ```bash
   cmake --build . --parallel $(nproc)
   ```

4. **Run ALL tests:**
   ```bash
   ctest --parallel $(nproc) --output-on-failure
   ```

**Output:**
- Full build progress
- Complete test suite results
- Detailed report:
  - Total build time
  - Total test time
  - Tests passed/failed breakdown by subsystem
  - Overall success/failure status

**Use case:** Before commits, after major changes, CI/CD verification locally.

**Note:** This is the slowest but most thorough option (clean build + all tests).