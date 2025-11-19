# Délégation Intelligente - Routing Rules

Règles de routage pour l'orchestrateur Emeraude vers les agents spécialisés.

## 🎯 Agent Routing Matrix

### 1. Code Review Agent
**Triggers** :
```typescript
const CODE_REVIEW_TRIGGERS = [
  // Quality & Analysis  
  "review", "code review", "pr review", "quality",
  "analyze", "analysis", "complexity", "performance",
  
  // Algorithmic
  "algorithm", "big o", "optimization", "bottleneck",
  "hotspot", "profiling", "benchmark",
  
  // STL & C++ 
  "stl", "standard library", "c++20", "template",
  "container", "iterator", "smart pointer",
  
  // Format & Style
  "format", "style", "clang", "tidy", "lint"
];
```

**Context Automatique** :
```markdown
→ Context: ["@shared-conventions.md", "@code-review-patterns.md"]  
→ Subagents: complexity-analyzer, stl-advisor, format-checker, performance-optimizer
→ Focus: Algorithm complexity, STL usage, Emeraude conventions
```

### 2. Debug Assistant Agent
**Triggers** :
```typescript
const DEBUG_TRIGGERS = [
  // Crashes & Issues
  "debug", "crash", "segfault", "core dump", "abort",
  "exception", "assert", "hang", "freeze",
  
  // Memory Issues  
  "memory leak", "valgrind", "sanitizer", "heap",
  "corruption", "buffer overflow", "use after free",
  
  // Tools
  "gdb", "lldb", "breakpoint", "watchpoint", "backtrace",
  "disassemble", "stack trace"
];
```

**Context Automatique** :
```markdown
→ Context: ["@debug-context.md", subsystem-specific debug patterns]
→ Subagents: breakpoint-manager, memory-analyzer, root-cause-analyzer  
→ Focus: Systematic debugging, memory analysis, crash investigation
```

### 3. Test Orchestrator Agent
**Triggers** :
```typescript
const TEST_TRIGGERS = [
  // Testing
  "test", "testing", "unit test", "integration test",
  "ctest", "google test", "coverage", "lcov",
  
  // Validation
  "validate", "verification", "regression", "benchmark",
  "performance test", "stress test",
  
  // CI/CD
  "build test", "pipeline", "automation"
];
```

**Context Automatique** :
```markdown
→ Context: ["@test-patterns.md", "@convention-validation.md"]
→ Subagents: unit-test-runner, coverage-analyzer, integration-validator
→ Focus: Smart test selection, convention validation, coverage analysis
```

## 🔧 Subsystem-Specific Routing

### Physics Subsystem
**Detection Patterns** :
```typescript
const PHYSICS_PATTERNS = [
  // Keywords
  "physics", "collision", "gravity", "constraint", "impulse",
  "force", "velocity", "acceleration", "mass", "friction",
  
  // Y-down specific
  "y-down", "coordinate", "y axis", "gravity direction",
  
  // Components  
  "MovableTrait", "BodyPhysicalProperties", "ContactManifold",
  
  // Files
  "src/Physics/", "Physics::", "MovableTrait::"
];
```

**Routing Logic** :
```typescript
if (detectPhysicsContext(userPrompt, files)) {
  return {
    agent: primaryAgent,  // code-reviewer, debug-assistant, etc.
    context: [
      "@shared-conventions.md",
      "@physics-context.md", 
      "@y-down-rules.md"
    ],
    specialFocus: "y-down-compliance",
    validation: "critical-physics-patterns"
  };
}
```

### Graphics/Vulkan Subsystem
**Detection Patterns** :
```typescript
const GRAPHICS_PATTERNS = [
  // Graphics
  "graphics", "render", "shader", "texture", "material",
  "geometry", "vertex", "pipeline", "framebuffer",
  
  // Vulkan specific
  "vulkan", "vk", "buffer", "image", "memory", "command",
  "descriptor", "pipeline layout",
  
  // Saphir
  "saphir", "glsl", "generation", "compatibility",
  
  // Files
  "src/Graphics/", "src/Vulkan/", "src/Saphir/"
];
```

**Routing Logic** :
```typescript
if (detectGraphicsContext(userPrompt, files)) {
  return {
    agent: primaryAgent,
    context: [
      "@shared-conventions.md",
      "@graphics-context.md",
      "@vulkan-abstraction-rules.md", 
      "@saphir-patterns.md"
    ],
    specialFocus: "vulkan-abstraction-compliance",
    validation: "no-direct-vulkan-calls"
  };
}
```

### Resources Subsystem
**Detection Patterns** :
```typescript
const RESOURCES_PATTERNS = [
  // Resources
  "resource", "loading", "load", "asset", "dependency",
  "container", "manager", "cache", "fail-safe",
  
  // Patterns
  "ResourceTrait", "neutral", "default", "fallback",
  "nullptr", "null pointer",
  
  // Files
  "src/Resources/", "ResourceTrait::", "Container::"
];
```

**Routing Logic** :
```typescript
if (detectResourcesContext(userPrompt, files)) {
  return {
    agent: primaryAgent,
    context: [
      "@shared-conventions.md", 
      "@resources-context.md",
      "@fail-safe-rules.md"
    ],
    specialFocus: "fail-safe-compliance",
    validation: "never-nullptr-return"
  };
}
```

## 🎭 Multi-Agent Coordination

### Sequential Delegation
```typescript
// Example: Complex PR review  
const sequentialWorkflow = {
  step1: {
    agent: "emeraude-code-reviewer",
    task: "analyze code quality and conventions",
    output: "quality-report.md"
  },
  step2: {
    agent: "emeraude-test-orchestrator", 
    task: "run relevant tests based on changed files",
    input: "quality-report.md",
    output: "test-results.md"
  },
  step3: {
    agent: "emeraude-orchestrator",
    task: "consolidate results and provide final recommendation", 
    inputs: ["quality-report.md", "test-results.md"],
    output: "final-review.md"
  }
};
```

### Parallel Delegation
```typescript
// Example: Comprehensive debug session
const parallelWorkflow = {
  parallel: [
    {
      agent: "emeraude-debug-assistant",
      subagent: "memory-analyzer", 
      task: "analyze memory usage and leaks"
    },
    {
      agent: "emeraude-debug-assistant",
      subagent: "root-cause-analyzer",
      task: "identify crash patterns and causes"
    }
  ],
  consolidation: {
    agent: "emeraude-debug-assistant",
    task: "merge analysis and provide debugging strategy"
  }
};
```

## 🔄 Dynamic Context Updates

### Current Development State
```typescript
interface DynamicContext {
  recentCommits: GitCommit[];        // Last 10 commits for trend analysis
  modifiedFiles: string[];           // Currently staged changes
  activeFeature: string;             // Extracted from branch/commits
  runningTests: TestResult[];        // Recent test status
  buildStatus: BuildStatus;          // Current build state
  conventionViolations: Violation[]; // Recent compliance issues
}
```

### Context Auto-Update
```bash
# Executed before each delegation
function updateDynamicContext() {
  // Git state
  const recentCommits = git log --oneline -10
  const modifiedFiles = git diff --name-only HEAD
  
  // Test state  
  const lastTestRun = ctest --last-test-results
  
  // Convention state
  const violations = ./check-conventions.sh
  
  // Update context file
  updateFile(".claude/context/current-state.md", {
    commits: recentCommits,
    files: modifiedFiles, 
    tests: lastTestRun,
    violations: violations
  });
}
```

## 📊 Routing Success Metrics

### Accuracy Metrics
- **Correct routing** : 95%+ tasks routed to appropriate specialist
- **Context relevance** : Agents receive only pertinent information  
- **Resolution efficiency** : 3x faster than monolithic approach

### Quality Metrics
- **Convention compliance** : 100% critical conventions validated
- **False positives** : <5% unnecessary escalations
- **Knowledge preservation** : Patterns learned and reused

Cette matrice de délégation optimise l'orchestration intelligente vers les agents Emeraude spécialisés.