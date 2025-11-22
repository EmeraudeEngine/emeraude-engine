---
description: Quick incremental build + filtered tests (no clean, fast iteration)
---

Quick incremental build and test for fast development iteration.

**Purpose:** Fastest possible build + test cycle for active development.

**Task:**

1. **Use existing build directory** (no clean, no reconfigure):
   ```bash
   cd .claude-build-debug
   ```

2. **Incremental build:**
   ```bash
   cmake --build . --parallel $(nproc)
   ```
   Only recompiles changed files.

3. **Run filtered tests:**
   - Argument required: subsystem or test pattern
   - `ctest --parallel $(nproc) --output-on-failure -R <pattern>`

**Examples:**
- `/quick-test Physics` → Fast build + Physics tests only
- `/quick-test Vector` → Fast build + Vector tests only
- `/quick-test Collision` → Fast build + Collision tests only

**Output:**
- Build time (should be very fast if few changes)
- Filtered test results only
- Immediate feedback for rapid iteration

**Note:** If build directory doesn't exist, run `/build-test` first to configure.