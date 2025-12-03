# Specification: Cross-Platform Build Commands (macOS + Linux)

**Date**: 2025-11-22
**Objective**: Ensure slash command build compatibility between macOS and Linux
**Status**: In development

## Context

Current slash commands in `.claude/commands/` must work seamlessly on:
- **macOS** (Darwin, Apple Silicon ARM64 and Intel x86_64)
- **Linux** (Debian 13, Ubuntu 24.04, x86_64 and ARM64)

Currently, commands have been developed and tested primarily on Linux. This spec documents necessary adaptations for macOS while preserving Linux compatibility.

## Platform Differences

### 1. Processor Count

**Linux**:
```bash
nproc
```

**macOS**:
```bash
sysctl -n hw.ncpu
```

**Cross-Platform Solution**:
```bash
# Automatic CPU count detection
if [[ "$OSTYPE" == "darwin"* ]]; then
    NCPU=$(sysctl -n hw.ncpu)
else
    NCPU=$(nproc)
fi
```

### 2. CMake Generator

**Linux (default)**:
- Unix Makefiles (make)
- Or Ninja if installed

**macOS (recommended)**:
- Unix Makefiles (make) - Compatible everywhere
- Xcode - For IDE development
- Ninja - For optimized builds

**Current Solution**:
Don't specify a generator - CMake automatically chooses the best available.

```bash
# Let CMake decide (recommended)
cmake ..

# OR specify explicitly if needed
cmake -G "Unix Makefiles" ..
cmake -G "Ninja" ..           # If Ninja installed
cmake -G "Xcode" ..           # macOS only
```

### 3. Paths and Dependencies

**Potential differences**:
- Vulkan SDK location
- System libraries (OpenAL, etc.)
- Compilers (GCC vs Clang)

**Handling**:
CMake handles automatically via `find_package()` and environment variables.

### 4. Build Environment Variables

**macOS specific**:
```bash
# Vulkan SDK (if installed via LunarG)
export VULKAN_SDK="/Users/$USER/VulkanSDK/[version]/macOS"

# Architectures (Apple Silicon)
export CMAKE_OSX_ARCHITECTURES="arm64"  # Or "x86_64" for Intel
```

**Linux specific**:
```bash
# Generally no special variables needed
# Vulkan via package manager
```

## Command Modifications

### Commands to Adapt

#### 1. `/build-test`
**File**: `.claude/commands/build-test.md`

**Changes**:
```bash
# BEFORE (Linux-only)
cmake --build . --parallel $(nproc)

# AFTER (Cross-platform)
if [[ "$OSTYPE" == "darwin"* ]]; then
    cmake --build . --parallel $(sysctl -n hw.ncpu)
else
    cmake --build . --parallel $(nproc)
fi
```

**OR use helper function**:
```bash
# Function to add at command start
get_ncpu() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sysctl -n hw.ncpu
    else
        nproc
    fi
}

# Usage
cmake --build . --parallel $(get_ncpu)
```

#### 2. `/quick-test`
**File**: `.claude/commands/quick-test.md`

**Changes**: Same as `/build-test`

#### 3. `/full-test`
**File**: `.claude/commands/full-test.md`

**Changes**: Same as `/build-test`

#### 4. `/build-only`
**File**: `.claude/commands/build-only.md`

**Changes**: Same as `/build-test`

#### 5. `/clean-rebuild`
**File**: `.claude/commands/clean-rebuild.md`

**Changes**: Same as `/build-test`

#### 6. `/build-release`
**File**: `.claude/commands/build-release.md`

**Changes**: Same as `/build-test`

### Commands Without Modification Needed

- `/check-conventions` - Python/Grep scripts (portable)
- `/verify-y-down` - Python/Grep scripts (portable)
- `/show-architecture` - File reading (portable)
- `/doc-system` - File reading (portable)
- `/find-usage` - Grep (portable)
- `/check-references` - Python scripts (portable)
- `/add-resource-type` - File generation (portable)

## Recommended Implementation

### Option 1: Global Helper Function (RECOMMENDED)

Create a common functions file:

**`.claude/commands/common.sh`**:
```bash
#!/bin/bash

# Returns CPU count in a portable way
get_ncpu() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sysctl -n hw.ncpu
    else
        nproc
    fi
}

# Detects the platform
get_platform() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    else
        echo "unknown"
    fi
}

# Export for use in commands
export -f get_ncpu
export -f get_platform
```

**Usage in commands**:
```bash
# At start of each command .md
source "$(dirname "$0")/common.sh"

# Then
cmake --build . --parallel $(get_ncpu)
```

### Option 2: Inline in Each Command

Include detection directly in each command:

```bash
# Platform detection
if [[ "$OSTYPE" == "darwin"* ]]; then
    NCPU=$(sysctl -n hw.ncpu)
else
    NCPU=$(nproc)
fi

# Build
cmake --build . --parallel $NCPU
```

**Advantages**: No external dependency
**Disadvantages**: Duplicated code

### Option 3: CMAKE Variable (CLEANEST)

Use CMake to handle parallelism:

```bash
# CMake 3.12+ supports --parallel without argument
# It automatically detects CPU count
cmake --build . --parallel

# Works on macOS AND Linux!
```

**FINAL RECOMMENDATION**: Use **Option 3** (simplest and most portable)

## Implementation Plan

### Phase 1: macOS Testing
- [x] Test build-test on macOS (functional with .claude-build-debug/)
- [ ] Test full-test on macOS
- [ ] Test quick-test on macOS
- [ ] Test build-release on macOS
- [ ] Test clean-rebuild on macOS

### Phase 2: Command Modifications
- [ ] Modify `/build-test` with Option 3
- [ ] Modify `/quick-test` with Option 3
- [ ] Modify `/full-test` with Option 3
- [ ] Modify `/build-only` with Option 3
- [ ] Modify `/clean-rebuild` with Option 3
- [ ] Modify `/build-release` with Option 3

### Phase 3: Cross-Platform Validation
- [ ] Test all commands on macOS ARM64 (Apple Silicon)
- [ ] Test all commands on macOS x86_64 (Intel)
- [ ] Test all commands on Linux x86_64 (Debian/Ubuntu)
- [ ] Document results

### Phase 4: Documentation
- [ ] Update `.claude/commands/README.md`
- [ ] Add compatibility notes to each command
- [ ] Create "Multi-Platform Support" section in CLAUDE.md

## Notes

### macOS Specific

1. **Apple Silicon (ARM64)**
   - Rosetta 2 may be needed for some x86_64 dependencies
   - Verify all dependencies have native ARM64 binaries
   - CMake variable: `CMAKE_OSX_ARCHITECTURES=arm64`

2. **Vulkan SDK**
   - Manual installation required (LunarG)
   - Verify `VULKAN_SDK` is defined: `echo $VULKAN_SDK`
   - If empty, export: `export VULKAN_SDK="/Users/$USER/VulkanSDK/1.x.xxx.x/macOS"`

3. **OpenAL**
   - Provided by system (AudioToolbox framework)
   - No external installation normally needed

4. **Clang vs GCC**
   - macOS uses Apple Clang by default
   - C++20 compatibility: Clang 17.0+ required (verified: OK)

### Linux Specific

1. **nproc available**
   - Part of GNU coreutils (always installed)
   - No fallback needed

2. **Vulkan SDK**
   - Via package manager: `vulkan-sdk`, `libvulkan-dev`
   - No environment variable needed

3. **Dependencies**
   - All via apt/yum/pacman
   - List in README.md

## Validation Tests

### Test 1: Basic Build
```bash
# On macOS AND Linux
cd .claude-build-debug
cmake ..
cmake --build . --parallel
```

**Expected result**: Build succeeds without specifying CPU count

### Test 2: Build with Tests
```bash
# On macOS AND Linux
/build-test
```

**Expected result**: Build + tests pass on both platforms

### Test 3: Clean Rebuild
```bash
# On macOS AND Linux
/clean-rebuild
```

**Expected result**: Complete rebuild succeeds

## Compatibility Matrix

| Command | Linux x86_64 | macOS ARM64 | macOS x86_64 | Status |
|---------|--------------|-------------|--------------|--------|
| `/build-test` | OK | OK | ? | OK |
| `/quick-test` | OK | ? | ? | To test |
| `/full-test` | OK | ? | ? | To test |
| `/build-only` | OK | ? | ? | To test |
| `/build-release` | OK | ? | ? | To test |
| `/clean-rebuild` | OK | ? | ? | To test |

OK = Tested and functional
? = Not yet tested
X = Problem detected

## Changelog

### 2025-11-22
- Initial spec creation
- Platform differences identification
- Recommendation: `cmake --build . --parallel` (without argument)
- Build test successful on macOS ARM64 with `.claude-build-debug/`

## References

- CMake documentation: https://cmake.org/cmake/help/latest/manual/cmake.1.html#build-tool-mode
- Emeraude Engine AGENTS.md: Cross-platform build instructions
- Emeraude Engine README.md: Dependencies per platform

---

**Next step**: Implement Option 3 in all build commands and test on macOS ARM64 + Linux.
