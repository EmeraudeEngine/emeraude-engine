# Conventions Critiques Emeraude Engine - Context Agents

Règles non-négociables à valider dans TOUTE délégation d'agents.

## 🚨 Conventions CRITIQUES (Blocking)

### 1. Y-down Coordinate System (BREAKING)
**Règle** : Y-down partout, jamais de conversions
```cpp
// VALIDATION PATTERNS
✅ CORRECT: gravity.y > 0      // Gravity pulls DOWN (+Y)
✅ CORRECT: jumpForce.y < 0    // Jump pushes UP (-Y)  
✅ CORRECT: groundNormal = (0, -1, 0)  // Ground points UP (-Y)

❌ VIOLATION: gravity.y < 0    // Wrong direction
❌ VIOLATION: "flip Y" comments // Conversion attempts
❌ VIOLATION: -9.81 hardcoded  // Should be +9.81
```

**Auto-detection** :
```bash
# Patterns à détecter automatiquement
grep -r "\-9\.81\|flip.*[Yy]\|invert.*[Yy]" src/Physics/ src/Graphics/ src/Audio/
```

### 2. Fail-safe Resource Management (BREAKING)
**Règle** : Jamais de nullptr, toujours ressource valide
```cpp
// VALIDATION PATTERNS  
✅ CORRECT: container->getResource() → always valid pointer
✅ CORRECT: neutral resource si loading fail
✅ CORRECT: application continue même avec assets cassés

❌ VIOLATION: return nullptr     // Banned in Resources/
❌ VIOLATION: if (resource == nullptr)  // Unnecessary check
❌ VIOLATION: crash on missing asset    // Must be fail-safe
```

**Auto-detection** :
```bash
# Rechercher violations fail-safe
grep -r "return nullptr" src/Resources/
grep -r "== nullptr.*resource" src/
```

### 3. Vulkan Abstraction (BREAKING)  
**Règle** : Jamais d'appels Vulkan directs hors Vulkan/
```cpp
// VALIDATION PATTERNS
✅ CORRECT: vulkanDevice.createBuffer()  // Use abstractions
✅ CORRECT: Graphics classes only
✅ CORRECT: Saphir pour shader generation

❌ VIOLATION: vkCreateBuffer()     // Direct Vulkan call
❌ VIOLATION: vk* functions        // Outside Vulkan/
❌ VIOLATION: Manual GLSL files    // Use Saphir instead
```

**Auto-detection** :
```bash
# Rechercher appels Vulkan directs
grep -r "vk[A-Z]" src/Graphics/ src/Resources/ --exclude-dir=Vulkan
```

### 4. Memory Management (CRITICAL)
**Règle** : RAII partout, VMA pour GPU
```cpp
// VALIDATION PATTERNS
✅ CORRECT: std::shared_ptr, std::unique_ptr
✅ CORRECT: VMA pour allocations GPU
✅ CORRECT: Automatic cleanup via destructors

❌ VIOLATION: raw pointers new/delete
❌ VIOLATION: manual vkAllocateMemory 
❌ VIOLATION: forgot destroy/cleanup
```

## 🔍 Validation Commands par Convention

### Y-down Validation
```bash
# Agent command pour validation Y-down
function validateYDown(files: string[]) {
  const violations = [];
  
  // Check gravity values  
  if (grep("-9\.81", files)) {
    violations.push("Gravity should be +9.81 (Y-down)");
  }
  
  // Check comment patterns
  if (grep("flip.*[Yy]|invert.*[Yy]", files)) {
    violations.push("Y-axis conversion detected - avoid flips");
  }
  
  return violations;
}
```

### Fail-safe Validation
```bash
# Agent command pour validation fail-safe
function validateFailSafe(files: string[]) {
  const violations = [];
  
  // Check nullptr returns
  if (grep("return nullptr", files.filter(f => f.includes("Resources/")))) {
    violations.push("Resources must never return nullptr");
  }
  
  // Check unnecessary nullptr checks
  if (grep("== nullptr.*resource", files)) {
    violations.push("Unnecessary nullptr check - resources are always valid");
  }
  
  return violations;
}
```

### Vulkan Abstraction Validation
```bash
# Agent command pour validation Vulkan
function validateVulkanAbstraction(files: string[]) {
  const violations = [];
  
  // Check direct Vulkan calls outside Vulkan/
  const nonVulkanFiles = files.filter(f => !f.includes("Vulkan/"));
  if (grep("vk[A-Z]", nonVulkanFiles)) {
    violations.push("Direct Vulkan calls forbidden outside Vulkan/ - use abstractions");
  }
  
  return violations;
}
```

## 📋 Action Guidelines par Agent

### Pour Code Review Agent
1. **TOUJOURS valider** ces 4 conventions en premier
2. **BLOQUER** si violation critique détectée  
3. **RÉFÉRENCER** docs/ pour explications complètes
4. **SUGGÉRER** fix concret avec exemples

### Pour Debug Assistant Agent  
1. **SUSPECTER** violation convention si crash
2. **CONFIGURER** breakpoints spécialisés selon convention
3. **ANALYSER** patterns de bugs liés aux conventions
4. **GUIDER** vers fix respectant conventions

### Pour Test Orchestrator Agent
1. **INCLURE** tests validation conventions automatiques
2. **CRÉER** tests spécialisés pour chaque convention 
3. **MONITORER** régression compliance
4. **RAPPORTER** métriques conformité

## 🚦 Severity Levels

### CRITICAL (Blocking) 🔴
- Y-down violation (crash physics/graphics)
- nullptr return (crash application)  
- Direct Vulkan calls (break abstraction)
- Memory leaks (performance degradation)

### WARNING (Review Required) ⚠️
- Suspicious patterns but not definitive
- Performance implications  
- Best practice violations
- Documentation gaps

### INFO (Suggestions) ℹ️
- Optimization opportunities
- Code style improvements
- Refactoring suggestions
- Architecture enhancements

## 📊 Métriques de Conformité

### Target Compliance Rates
- **Y-down violations** : 0% (zero tolerance)
- **Fail-safe violations** : 0% (zero tolerance)  
- **Vulkan abstraction** : 0% (zero tolerance)
- **Memory management** : <1% (très rare exceptions)

### Monitoring Commands
```bash
# Daily convention check
./check-conventions-compliance.sh

# Pre-commit validation  
./validate-emeraude-conventions.sh

# Metrics reporting
./generate-compliance-report.sh
```

Ces conventions sont la base de la stabilité et maintenabilité d'Emeraude Engine. Tout agent doit les respecter absolument.