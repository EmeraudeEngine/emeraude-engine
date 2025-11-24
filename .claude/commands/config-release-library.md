---
description: Configure Release build (library only, no tests)
---

Configure the project for Release build without enabling tests.

**Purpose:** Set up Release configuration for optimized library build without test overhead.

**Task:**

1. **Detect platform and set appropriate generator:**
   - Linux/macOS: Use Ninja generator
   - Windows: Use Visual Studio 17 2022
   - macOS: Add appropriate architecture flag (arm64 or x86_64)

2. **Configure Release build:**
# Linux example:
cmake -S . -B .claude-build-release -DCMAKE_BUILD_TYPE=Release -G "Ninja"

# macOS (Apple Silicon) example:
cmake -S . -B .claude-build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -G "Ninja"

# macOS (Intel) example:
cmake -S . -B .claude-build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 -G "Ninja"

# Windows example:
cmake -S . -B .claude-build-release -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
```

3. **Report configuration results:**
   - Generator used
   - Build directory: `.claude-build-release`
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
- Ready for `/build-release`

**Next steps:** Run `/build-release` to compile
