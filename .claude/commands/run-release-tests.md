---
description: Run Release Google Tests with full output
---

Execute Google Tests from Release build with complete verbose output.

**Purpose:** Run all tests in optimized Release mode and display full results, even when all tests pass.

**Prerequisites:**
- Release build with tests must be compiled (run `/config-release-tests` then `/build-release`)
- Build directory: `.claude-build-release`
- Tests must be enabled in configuration

**Task:**

1. **Verify test executable exists:**
   - Check for test executable in `.claude-build-release/`
   - If missing, error: "Tests not found. Run /config-release-tests and /build-release first"

2. **Run tests with verbose output:**
   ```bash
   cd .claude-build-release/Release
   ctest --output-on-failure --verbose
   ```
   Or directly run the test executable (must be run from Release directory):
   ```bash
   cd .claude-build-release/Release && ./EmeraudeUnitTests --gtest_color=yes
   ```

3. **Support test filtering:**
   - Accept optional filter parameter for specific tests
   - Example: `--gtest_filter="Math*"` to run only Math tests
   - Example: `--gtest_filter="*Compression*"` for compression tests

4. **Report results:**
   - Total tests run
   - Passed/Failed counts
   - Full output even if all pass
   - Failed test details
   - Test execution time (usually faster than Debug)

**Output format:**
- Individual test results (even successes)
- Summary statistics
- Failed test details with full logs
- Execution time per test suite
- Performance comparison note (Release is typically faster)

**Test filtering examples:**
- All tests: No filter
- Specific suite: `--gtest_filter="NodeTrait*"`
- Multiple patterns: `--gtest_filter="Math*:Hash*"`
- Exclude: `--gtest_filter="-*Slow*"`

**Error handling:**
- If tests not compiled: Guide to config and build
- If tests fail: Show full failure details
- If crash: Show crash location (note: harder to debug in Release)

**Note:** Release tests are optimized and may behave differently than Debug tests. Always validate in both configurations for critical features.

**Next steps:**
- If failures: Switch to Debug for better diagnostics (`/config-debug-tests`)
- If all pass: Release build validated and ready for deployment
