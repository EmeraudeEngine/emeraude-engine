---
description: Build Debug configuration (incremental)
---

Build the project in Debug mode.

**Purpose:** Compile and verify code builds successfully.

**Build directory:** `.claude-build-debug`

**Prerequisites:**
- Must be configured first with `/config-build-debug`

**Task:**

1. **Verify configuration exists:**
```bash
if [ ! -d ".claude-build-debug" ]; then
    echo "Error: No Debug configuration found. Run /config-build-debug first."
    exit 1
fi
```

2. **Build:**
```bash
cmake --build .claude-build-debug --config Debug
```

**Output:**
- Compilation result (success/errors)
- Error details if failed
- Build time

**On error:** Fix the code and run `/build-debug` again.

**For tests:** Use the `emeraude-test-orchestrator` agent instead.