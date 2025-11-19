---
description: Configure Debug build (library only, no tests)
---

Configure the project for Debug build without enabling tests.

**Purpose:** Set up Debug configuration for library development without test overhead.

**Task:**

1. **Detect platform and set appropriate generator:**
   - Linux/macOS: Use Ninja generator
   - Windows: Use Visual Studio 17 2022
   - macOS: Add appropriate architecture flag (arm64 or x86_64)

2. **Configure Debug build:**
   ```bash
   # Linux example:
   cmake -S . -B .claude-build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

   # macOS (Apple Silicon) example:
   cmake -S . -B .claude-build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64

   # Windows example:
   cmake -S . -B .claude-build-debug -G "Visual Studio 17 2022" -A x64
   ```

3. **Report configuration results:**
   - Generator used
   - Build directory: `.claude-build-debug`
   - Tests: DISABLED
   - Configuration status

**Platform Detection:**
- Check `uname -s` for platform
- Check `uname -m` for architecture on macOS
- Use appropriate CMake generator and flags

**Output:**
- CMake configuration log
- Selected generator
- Build type confirmation
- Ready for `/build-debug`

**Next steps:** Run `/build-debug` to compile
