---
description: Clean rebuild Release (remove build directory + reconfigure + build + tests)
---

Complete clean rebuild from scratch in Release mode with tests.

**Purpose:** Nuclear option for when incremental builds fail or after major changes in Release.

**Task:**

1. **Remove build directory:**
   ```bash
   rm -rf .claude-build-release
   ```

2. **Reconfigure with tests enabled:**
   - Use `/config-release-tests` command internally
   - This handles platform detection and generator selection

3. **Build from scratch:**
   - Use `/build-release` command internally
   - Fresh compilation of all targets with optimizations

4. **Run tests:**
   - Use `/run-release-tests` command internally
   - Full verbose output
   - Accept optional filter parameter

**Flow:**
```
/clean-release-rebuild [optional-filter]
  ↓
Remove .claude-build-release/
  ↓
/config-release-tests (platform-aware)
  ↓
/build-release
  ↓
/run-release-tests [filter]
```

**Examples:**
- `/clean-release-rebuild` → Clean, rebuild, test all
- `/clean-release-rebuild Math*` → Clean, rebuild, test Math suite only
- `/clean-release-rebuild "*Compression*"` → Clean, rebuild, test Compression tests

**Output:**
- Confirmation of deletion
- CMake configuration output (platform-specific)
- Full build progress with optimizations
- Complete test results
- Total time breakdown

**Use cases:**
- CMake configuration changes for Release
- After updating dependencies
- Suspicious incremental build issues in Release
- Before creating release candidate
- Final validation before deployment

**Warning:** This is the slowest option - deletes everything and rebuilds from scratch with optimizations. Use `/build-release` for incremental builds when possible.

**Time estimate:**
- Clean: <1s
- Configure: 3-5s
- Build: 2-10 minutes (depends on hardware, optimizations may take longer)
- Tests: 5-30s (usually faster than Debug due to optimizations)

**Note:** Release tests run with optimizations and may behave differently than Debug. Always validate in both configurations for critical features.