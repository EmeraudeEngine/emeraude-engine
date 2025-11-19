---
description: Configure Release build with Google Tests enabled
---

Configure the project for Release build with Google Tests enabled.

**Purpose:** Set up Release configuration for optimized testing and validation.

**Task:**

1. **Detect platform and set appropriate generator:**
   - Linux/macOS: Use Ninja generator
   - Windows: Use Visual Studio 17 2022
   - macOS: Add appropriate architecture flag (arm64 or x86_64)

2. **Configure Release build with tests:**
   ```bash
   # Linux example:
   cmake -S . -B .claude-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DEMERAUDE_ENABLE_TESTS:BOOL=ON

   # macOS (Apple Silicon) example:
   cmake -S . -B .claude-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DEMERAUDE_ENABLE_TESTS:BOOL=ON

   # Windows example:
   cmake -S . -B .claude-build-release -G "Visual Studio 17 2022" -A x64 -DEMERAUDE_ENABLE_TESTS:BOOL=ON
   ```

3. **Report configuration results:**
   - Generator used
   - Build directory: `.claude-build-release`
   - Tests: ENABLED (Google Test)
   - Configuration status

**Platform Detection:**
- Check `uname -s` for platform
- Check `uname -m` for architecture on macOS
- Use appropriate CMake generator and flags

**Output:**
- CMake configuration log
- Selected generator
- Build type confirmation
- Test status: ENABLED
- Ready for `/build-release` then `/run-release-tests`

**Next steps:**
1. Run `/build-release` to compile
2. Run `/run-release-tests` to execute tests
