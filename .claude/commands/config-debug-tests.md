---
description: Configure Debug build with Google Tests enabled
---

Configure the project for Debug build with Google Tests enabled.

**Purpose:** Set up Debug configuration for testing and development with full test suite.

**Task:**

1. **Detect platform and set appropriate generator:**
   - Linux/macOS: Use Ninja generator
   - Windows: Use Visual Studio 17 2022
   - macOS: Add appropriate architecture flag (arm64 or x86_64)

2. **Configure Debug build with tests:**
```bash
# Linux example:
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON -G "Ninja"

# macOS (Apple Silicon) example:
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON -DCMAKE_OSX_ARCHITECTURES=arm64 -G "Ninja"

# macOS (Intel) example:
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON -DCMAKE_OSX_ARCHITECTURES=x86_64 -G "Ninja"

# Windows example:
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON -G "Visual Studio 17 2022" -A x64 
```

3. **Report configuration results:**
   - Generator used
   - Build directory: `.claude-build-debug`
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
- Ready for `/build-debug` then `/run-debug-tests`

**Next steps:**
1. Run `/build-debug` to compile
2. Run `/run-debug-tests` to execute tests
