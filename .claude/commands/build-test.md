---
description: Build Debug + run tests (optionnel: filtrer par sous-système ou test)
---

Build the project in Debug mode with tests enabled and run tests.

**Arguments:**
- No argument: Run ALL tests
- Subsystem name: Run tests for specific subsystem (e.g., `Physics`, `Libs`)
- Test pattern: Run specific test pattern (e.g., `Vector`, `Collision`)

**Task:**

1. **Configure if needed:**
   ```bash
   mkdir -p .claude-build-debug
   cd .claude-build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON
   ```

2. **Build:**
   ```bash
   cmake --build . --parallel $(nproc)
   ```

3. **Run tests:**
   - No filter: `ctest --parallel $(nproc) --output-on-failure`
   - With filter: `ctest --parallel $(nproc) --output-on-failure -R <pattern>`

**Examples:**
- `/build-test` → All tests
- `/build-test Physics` → Tests matching "Physics"
- `/build-test Vector` → Tests matching "Vector"
- `/build-test Libs/Math` → Tests matching "Libs/Math"

**Output:**
- Show build progress (errors/warnings)
- Show test results summary
- If failures: show detailed failure output
- Report: X/Y tests passed, build time, test time

Use `--output-on-failure` to only show details of failed tests.