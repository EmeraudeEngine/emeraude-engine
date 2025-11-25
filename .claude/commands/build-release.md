---
description: Build Release configuration (incremental)
---

Build the project in Release mode.

**Purpose:** Compile with optimizations and verify code builds successfully.

**Build directory:** `.claude-build-release`

**Prerequisites:**
- Must be configured first with `/config-build-release`

**Task:**

1. **Verify configuration exists:**
```bash
if [ ! -d ".claude-build-release" ]; then
    echo "Error: No Release configuration found. Run /config-build-release first."
    exit 1
fi
```

2. **Build:**
```bash
cmake --build .claude-build-release --config Release
```

**Output:**
- Compilation result (success/errors)
- Error details if failed
- Build time

**On error:** Fix the code and run `/build-release` again.

**For tests:** Use the `emeraude-test-orchestrator` agent instead.