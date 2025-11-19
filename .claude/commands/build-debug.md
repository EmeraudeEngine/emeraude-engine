---
description: Build Debug configuration (as configured)
---

Build the project in Debug mode using the existing configuration.

**Purpose:** Compile the Debug build based on current CMake configuration.

**Prerequisites:**
- Configuration must exist (run `/config-debug-library` or `/config-debug-tests` first)
- Build directory: `.claude-build-debug`

**Task:**

1. **Verify configuration exists:**
   - Check if `.claude-build-debug/` exists
   - If missing, error: "No Debug configuration found. Run /config-debug-library or /config-debug-tests first"

2. **Build with appropriate command:**
   ```bash
   # All platforms:
   cmake --build .claude-build-debug --config Debug
   ```

3. **Report build results:**
   - Build progress
   - Compilation warnings/errors
   - Build time
   - Number of targets built
   - Success/failure status

**Output:**
- Compiler output
- Warning summary
- Build statistics
- Final status

**Error handling:**
- If configuration missing: Prompt to run config command
- If build fails: Show detailed error messages
- If warnings: Summarize warning count and types

**Note:** Build speed depends on configuration (library-only is faster than with tests)

**Next steps:**
- If tests enabled: Run `/run-debug-tests`
- If library-only: Test manually or integrate in application
