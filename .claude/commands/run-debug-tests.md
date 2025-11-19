---
description: Run Debug Google Tests with full output
---

Execute Google Tests from Debug build with complete verbose output.

**Purpose:** Run all tests and display full results, even when all tests pass.

**Prerequisites:**
- Debug build with tests must be compiled (run `/config-debug-tests` then `/build-debug`)
- Build directory: `.claude-build-debug`
- Tests must be enabled in configuration

**Task:**

1. **Verify test executable exists:**
   - Check for test executable in `.claude-build-debug/`
   - If missing, error: "Tests not found. Run /config-debug-tests and /build-debug first"

2. **Run tests with verbose output:**
   ```bash
   cd .claude-build-debug/Debug
   ctest --output-on-failure --verbose
   ```
   Or directly run the test executable (must be run from Debug directory):
   ```bash
   cd .claude-build-debug/Debug && ./EmeraudeUnitTests --gtest_color=yes
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
   - Test execution time

**Output format:**
- Individual test results (even successes)
- Summary statistics
- Failed test details with full logs
- Execution time per test suite

**Test filtering examples:**
- All tests: No filter
- Specific suite: `--gtest_filter="NodeTrait*"`
- Multiple patterns: `--gtest_filter="Math*:Hash*"`
- Exclude: `--gtest_filter="-*Slow*"`

**Error handling:**
- If tests not compiled: Guide to config and build
- If tests fail: Show full failure details
- If crash: Show crash location and stack trace

**Next steps:**
- If failures: Fix code and run `/build-debug` then `/run-debug-tests` again
- If all pass: Continue development or run `/config-release-tests`
