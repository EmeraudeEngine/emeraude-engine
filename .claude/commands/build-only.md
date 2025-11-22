---
description: Build Debug sans lancer les tests (compilation rapide)
---

Build the project in Debug mode without running tests.

**Purpose:** Verify compilation without waiting for test execution.

**Task:**

1. **Configure if needed:**
   ```bash
   mkdir -p .claude-build-debug
   cd .claude-build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON
   ```

2. **Build only:**
   ```bash
   cmake --build . --parallel $(nproc)
   ```

3. **DO NOT run tests** - just report build success/failure.

**Output:**
- Build progress
- Compilation errors/warnings
- Build time
- Success: "Build complete, tests not run"
- Failure: Detailed error messages

**Use case:**
- Quick compilation check
- Verify code compiles before running tests
- Fix compilation errors first, test later

**Note:** Tests are compiled but not executed. Run `/quick-test` or `/build-test` to execute them.