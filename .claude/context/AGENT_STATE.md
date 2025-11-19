# 🤖 Emeraude Engine - Agent Coordination State

**Purpose:** Shared state file for multi-agent coordination. All specialized agents MUST read this file before starting their task and update it when completing.

**Last Updated:** [Auto-updated by agents]
**Current Orchestrator:** [Agent name that initiated current workflow]

---

## 📊 Current Session Status

**Session Type:** [code-review | debug | test | build | convention-check | feature-development]
**Priority:** [low | medium | high | critical]
**Deadline:** [None | Date/Time]

### Overall Progress
- [ ] Task analysis completed
- [ ] Specialists assigned
- [ ] Execution in progress
- [ ] Results consolidated
- [ ] Final validation done

---

## 🎯 Current Objective

**Goal:** [One-sentence description of what we're trying to achieve]

**Context:** [Brief background - why this task matters, what triggered it]

**Success Criteria:**
- [ ] [Criterion 1]
- [ ] [Criterion 2]
- [ ] [Criterion 3]

---

## 🔍 Affected Subsystems

**Primary:**
- [ ] Physics (`src/Physics/`)
- [ ] Graphics (`src/Graphics/`)
- [ ] Vulkan (`src/Vulkan/`)
- [ ] Saphir (`src/Saphir/`)
- [ ] Resources (`src/Resources/`)
- [ ] Scenes (`src/Scenes/`)
- [ ] Audio (`src/Audio/`)
- [ ] Input (`src/Input/`)
- [ ] Libs (`src/Libs/`)
- [ ] Testing (`src/Testing/`)

**Secondary:** [List related subsystems that might be impacted]

---

## 📋 Agent Task Assignments

### ✅ Completed Tasks

| Agent | Task | Status | Duration | Verdict | Notes |
|-------|------|--------|----------|---------|-------|
| - | - | - | - | - | - |

### 🔄 In Progress

| Agent | Task | Started | ETA | Blocking Issues |
|-------|------|---------|-----|-----------------|
| - | - | - | - | - |

### ⏳ Pending

| Agent | Task | Dependencies | Priority |
|-------|------|--------------|----------|
| - | - | - | - |

---

## 🚨 Critical Validation Checkpoints

### Emeraude Engine Conventions (NON-NEGOTIABLE)

**Y-Down Coordinate System:**
- [ ] No Y-coordinate flips in code
- [ ] Gravity = +9.81f (positive Y)
- [ ] Jump forces use negative Y
- [ ] Status: [NOT_CHECKED | VALIDATED | VIOLATION_FOUND]
- [ ] Command: `/verify-y-down [files]`

**Vulkan Abstraction:**
- [ ] No direct Vulkan calls outside `src/Vulkan/`
- [ ] Graphics uses abstractions (Geometry, Material, Renderable)
- [ ] Saphir generates shaders (no manual GLSL files)
- [ ] Status: [NOT_CHECKED | VALIDATED | VIOLATION_FOUND]
- [ ] Check: `grep -r "vkCmd\|vkCreate\|vkDestroy" src/Graphics/ src/Saphir/`

**Fail-Safe Resource Management:**
- [ ] No nullptr returns from Containers
- [ ] All ResourceTrait implement neutral `load(ServiceProvider&)`
- [ ] `onDependenciesLoaded()` pattern used correctly
- [ ] Status: [NOT_CHECKED | VALIDATED | VIOLATION_FOUND]
- [ ] Check: Manual review of ResourceTrait implementations

**Code Quality:**
- [ ] `.clang-format` compliance verified
- [ ] `.clang-tidy` warnings checked
- [ ] Status: [NOT_CHECKED | VALIDATED | VIOLATION_FOUND]
- [ ] Command: `/check-conventions`

---

## 🧪 Test Execution Status

**Strategy:** [/quick-test | /full-test | /build-only | custom]

**Test Results:**
```
Command: [Test command executed]
Status: [NOT_RUN | RUNNING | PASSED | FAILED]
Duration: [Time taken]
Passed: [X/Y tests]
Failed Tests: [List if any]
```

**Failed Tests Analysis:**
- [Test name]: [Reason for failure] → [Fix needed]

---

## 📊 Agent Reports Summary

### Complexity Analysis
**Agent:** complexity-analyzer
**Status:** [PENDING | IN_PROGRESS | COMPLETED]
**Verdict:** [APPROVE | REQUEST_CHANGES | COMMENT]
**Key Findings:**
- [Finding 1]
- [Finding 2]

### STL & C++20 Review
**Agent:** stl-advisor
**Status:** [PENDING | IN_PROGRESS | COMPLETED]
**Verdict:** [APPROVE | REQUEST_CHANGES | COMMENT]
**Key Findings:**
- [Finding 1]
- [Finding 2]

### Format & Style Check
**Agent:** format-checker
**Status:** [PENDING | IN_PROGRESS | COMPLETED]
**Verdict:** [APPROVE | REQUEST_CHANGES | COMMENT]
**Key Findings:**
- [Finding 1]
- [Finding 2]

### Performance Analysis
**Agent:** performance-optimizer
**Status:** [PENDING | IN_PROGRESS | COMPLETED]
**Verdict:** [APPROVE | REQUEST_CHANGES | COMMENT]
**Key Findings:**
- [Finding 1]
- [Finding 2]

### Debug Analysis (if applicable)
**Agent:** emeraude-debug-assistant
**Status:** [PENDING | IN_PROGRESS | COMPLETED]
**Key Findings:**
- [Finding 1]
- [Finding 2]

### Build Validation (if applicable)
**Agent:** emeraude-build-agent
**Status:** [PENDING | IN_PROGRESS | COMPLETED]
**Platforms Tested:** [Windows | Linux | macOS]
**Key Findings:**
- [Finding 1]
- [Finding 2]

---

## 🎯 Action Items

### IMMEDIATE (Blocking - must fix before merge)
- [ ] [Action 1]
- [ ] [Action 2]

### RECOMMENDED (Quality improvements)
- [ ] [Action 1]
- [ ] [Action 2]

### FUTURE (Technical debt)
- [ ] [Action 1]
- [ ] [Action 2]

---

## 📝 Session Notes

**Challenges Encountered:**
- [Challenge 1]: [How it was resolved]

**Lessons Learned:**
- [Lesson 1]

**Follow-Up Required:**
- [Item 1]

---

## 🏁 Final Decision

**Overall Verdict:** [APPROVE | REQUEST_CHANGES | COMMENT | IN_PROGRESS]

**Decision Rationale:**
[Explanation based on all agent reports, test results, and convention compliance]

**Sign-Off:**
- Orchestrator: [Agent name] - [Timestamp]
- Final Reviewer: [Agent name] - [Timestamp]

---

## 🔄 State Management Instructions

**For Orchestrator (emeraude-orchestrator):**
1. Create/Reset this file at workflow start
2. Fill "Current Objective" and "Affected Subsystems"
3. Assign tasks to specialists
4. Monitor progress and update "Agent Task Assignments"
5. Consolidate results into "Final Decision"

**For Specialist Agents:**
1. READ this file FIRST before starting
2. Check "Current Objective" and your assigned task
3. Execute your specialized task
4. UPDATE your section in "Agent Reports Summary"
5. MARK your task as completed in "Agent Task Assignments"
6. ADD any critical issues to "Action Items"

**For Convention Validator:**
1. Execute all convention checks
2. Update "Critical Validation Checkpoints"
3. BLOCK workflow if violations found
4. Provide fix guidance in "Action Items"

**State Transitions:**
```
[INIT] → Orchestrator creates file
       ↓
[ASSIGNED] → Tasks distributed to specialists
       ↓
[IN_PROGRESS] → Specialists execute and update
       ↓
[CONSOLIDATION] → Orchestrator gathers results
       ↓
[DECISION] → Final verdict and sign-off
```

---

**Template Version:** 1.0
**Last Template Update:** 2025-01-22
