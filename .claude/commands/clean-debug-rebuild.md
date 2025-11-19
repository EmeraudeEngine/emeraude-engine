---
description: Clean rebuild Debug (remove build directory + reconfigure + build + tests)
---

Complete clean rebuild from scratch in Debug mode with tests.

**Purpose:** Nuclear option for when incremental builds fail or after major changes.

**Task:**

1. **Remove build directory:**
   ```bash
   rm -rf .claude-build-debug
   ```

2. **Reconfigure with tests enabled:**
   - Use `/config-debug-tests` command internally
   - This handles platform detection and generator selection

3. **Build from scratch:**
   - Use `/build-debug` command internally
   - Fresh compilation of all targets

4. **Run tests:**
   - Use `/run-debug-tests` command internally
   - Full verbose output
   - Accept optional filter parameter

**Flow:**
```
/clean-debug-rebuild [optional-filter]
  ↓
Remove .claude-build-debug/
  ↓
/config-debug-tests (platform-aware)
  ↓
/build-debug
  ↓
/run-debug-tests [filter]
```

**Examples:**
- `/clean-debug-rebuild` → Clean, rebuild, test all
- `/clean-debug-rebuild Math*` → Clean, rebuild, test Math suite only
- `/clean-debug-rebuild "*Compression*"` → Clean, rebuild, test Compression tests

**Output:**
- Confirmation of deletion
- CMake configuration output (platform-specific)
- Full build progress
- Complete test results
- Total time breakdown

**Use cases:**
- CMake configuration changes
- After updating dependencies
- Suspicious incremental build issues
- After pulling major changes
- When build cache is corrupted

**Warning:** This is the slowest option - deletes everything and rebuilds from scratch. Use `/build-debug` for incremental builds when possible.

**Time estimate:**
- Clean: <1s
- Configure: 3-5s
- Build: 2-10 minutes (depends on hardware)
- Tests: 5-30s
