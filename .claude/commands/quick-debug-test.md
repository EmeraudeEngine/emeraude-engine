---
description: Quick incremental Debug build + filtered tests (fast iteration)
---

Quick incremental Debug build and test for fast development iteration.

**Purpose:** Fastest possible Debug build + test cycle for active development.

**Prerequisites:**
- Debug build must be configured (run `/config-debug-tests` once)
- Build directory: `.claude-build-debug`

**Task:**

1. **Verify configuration exists:**
   - Check if `.claude-build-debug/` exists
   - If missing, error: "No Debug configuration found. Run /config-debug-tests first"

2. **Incremental build:**
   - Use `/build-debug` command internally
   - Only recompiles changed files (fast)

3. **Run filtered tests:**
   - Use `/run-debug-tests` with filter parameter
   - Filter argument REQUIRED for quick iteration
   - Example filters: `Math*`, `*Compression*`, `NodeTrait*`

**Flow:**
```
/quick-debug-test <filter>
  ↓
/build-debug (incremental, fast)
  ↓
/run-debug-tests <filter>
```

**Examples:**
- `/quick-debug-test Math*` → Fast build + Math tests only
- `/quick-debug-test "*Physics*"` → Fast build + Physics tests only
- `/quick-debug-test NodeTrait*` → Fast build + NodeTrait suite only
- `/quick-debug-test "*Compression*"` → Fast build + Compression tests

**Output:**
- Incremental build time (should be very fast if few changes)
- Filtered test results only
- Immediate feedback for rapid iteration

**Comparison with other commands:**
- `/quick-debug-test` → Incremental build + filtered tests (FASTEST)
- `/run-debug-tests` → Just run tests, no build
- `/build-debug` → Just build, no tests
- `/clean-debug-rebuild` → Full clean rebuild (SLOWEST)

**Use cases:**
- Active development on specific feature
- Rapid iteration on failing test
- TDD workflow (test-driven development)
- Quick validation after small changes

**Time estimate:**
- Build: 5s-2min (incremental, depends on changes)
- Filtered tests: 1-10s (depends on filter)
- Total: Usually under 30s for small changes

**Note:** If you see build issues, try `/clean-debug-rebuild` for a fresh start