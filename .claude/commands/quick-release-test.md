---
description: Quick incremental Release build + filtered tests (fast iteration)
---

Quick incremental Release build and test for fast development iteration in optimized mode.

**Purpose:** Fastest possible Release build + test cycle for performance validation.

**Prerequisites:**
- Release build must be configured (run `/config-release-tests` once)
- Build directory: `.claude-build-release`

**Task:**

1. **Verify configuration exists:**
   - Check if `.claude-build-release/` exists
   - If missing, error: "No Release configuration found. Run /config-release-tests first"

2. **Incremental build:**
   - Use `/build-release` command internally
   - Only recompiles changed files with optimizations (fast)

3. **Run filtered tests:**
   - Use `/run-release-tests` with filter parameter
   - Filter argument REQUIRED for quick iteration
   - Example filters: `Math*`, `*Compression*`, `NodeTrait*`

**Flow:**
```
/quick-release-test <filter>
  ↓
/build-release (incremental, fast)
  ↓
/run-release-tests <filter>
```

**Examples:**
- `/quick-release-test Math*` → Fast build + Math tests only
- `/quick-release-test "*Physics*"` → Fast build + Physics tests only
- `/quick-release-test NodeTrait*` → Fast build + NodeTrait suite only
- `/quick-release-test "*Compression*"` → Fast build + Compression tests

**Output:**
- Incremental build time (slightly longer than Debug due to optimizations)
- Filtered test results only
- Performance metrics (usually faster execution than Debug)

**Comparison with other commands:**
- `/quick-release-test` → Incremental build + filtered tests (FASTEST in Release)
- `/run-release-tests` → Just run tests, no build
- `/build-release` → Just build, no tests
- `/clean-release-rebuild` → Full clean rebuild (SLOWEST)

**Use cases:**
- Performance testing on specific feature
- Validate optimizations didn't break functionality
- Benchmark iteration (optimize → build → test → measure)
- Pre-deployment validation on specific subsystem

**Time estimate:**
- Build: 10s-3min (incremental with optimizations, depends on changes)
- Filtered tests: 1-5s (usually faster than Debug)
- Total: Usually under 1-2min for small changes

**Note:**
- If you see build issues, try `/clean-release-rebuild` for a fresh start
- Release tests may behave differently than Debug. Always validate critical features in both configurations.
- Use this for performance validation, `/quick-debug-test` for debugging