---
description: Configure Debug build (library only, no tests)
---

Configure the project for Debug build.

**Purpose:** Set up Debug configuration for compilation and debugging.

**Build directory:** `.claude-build-debug`

**Task:**

1. **Ask if tests should be enabled (BEFORE configuring):**
   Use AskUserQuestion to ask:
   - "Do you want to enable unit tests for this build?"

   If yes, add `-DEMERAUDE_ENABLE_TESTS:BOOL=ON` to the cmake command.

2. **Detect platform and set appropriate generator:**
   - Linux: Ninja
   - macOS: Ninja + architecture flag (arm64 or x86_64)
   - Windows: Visual Studio 17 2022

3. **Configure Debug build:**
```bash
# Linux (without tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -G "Ninja"

# Linux (with tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON -G "Ninja"

# macOS (Apple Silicon, without tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64 -G "Ninja"

# macOS (Apple Silicon, with tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64 -DEMERAUDE_ENABLE_TESTS:BOOL=ON -G "Ninja"

# macOS (Intel, without tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=x86_64 -G "Ninja"

# macOS (Intel, with tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=x86_64 -DEMERAUDE_ENABLE_TESTS:BOOL=ON -G "Ninja"

# Windows (without tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 17 2022" -A x64

# Windows (with tests):
cmake -S . -B .claude-build-debug -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON -G "Visual Studio 17 2022" -A x64
```

4. **Report configuration results:**
- Generator used
- Build directory: `.claude-build-debug`
- Configuration status


**Platform Detection:**
```bash
OS=$(uname -s)      # Linux, Darwin, MINGW*/MSYS*
ARCH=$(uname -m)    # arm64, x86_64
```

**Output:**
- Generator used
- Build directory path
- Tests enabled: Yes/No
- Configuration status

**Next step:** Run `/build-debug` to compile.