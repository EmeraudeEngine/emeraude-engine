---
description: Build Release configuration (as configured)
---

Build the project in Release mode using the existing configuration.

**Purpose:** Compile the Release build based on current CMake configuration.

**Prerequisites:**
- Configuration must exist (run `/config-release-library` or `/config-release-tests` first)
- Build directory: `.claude-build-release`

**Task:**

1. **Verify configuration exists:**
   - Check if `.claude-build-release/` exists
   - If missing, error: "No Release configuration found. Run /config-release-library or /config-release-tests first"

2. **Build with appropriate command:**
   ```bash
   # All platforms:
   cmake --build .claude-build-release --config Release
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
- If tests enabled: Run `/run-release-tests`
- If library-only: Test manually or integrate in application
