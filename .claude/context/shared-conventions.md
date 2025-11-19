# Conventions Critiques Emeraude Engine - Context Agents

Règles non-négociables à valider dans TOUTE délégation d'agents.

## 🎯 Principes Philosophiques Fondamentaux

### Principle of Least Astonishment (POLA)
**Définition** : Le code doit se comporter comme un utilisateur raisonnable s'y attendrait.

**Application dans Emeraude Engine** :
```cpp
// ✅ CORRECT: Nomenclature intuitive
auto camera = scene->createCamera("main");  // On s'attend à créer une caméra
camera->lookAt(target);                     // Comportement évident

// ✅ CORRECT: Conventions cohérentes
geometry->getVertexCount();   // get* pour accesseurs
geometry->setVertexData();    // set* pour mutateurs

// ❌ VIOLATION: Comportement surprenant
auto camera = scene->createCamera("main");
camera->render();  // Surprise! Crée aussi les shaders et modifie la scène

// ❌ VIOLATION: Nomenclature trompeuse
auto count = geometry->vertices();  // On s'attend à un tableau, pas un count
```

**Règles dérivées** :
- Fonctions nommées selon leur action réelle (pas d'effets de bord cachés)
- APIs cohérentes entre subsystems similaires
- Valeurs par défaut prévisibles et sûres
- Messages d'erreur clairs expliquant ce qui s'est passé

### Pit of Success
**Définition** : Rendre le bon chemin plus facile que le mauvais chemin.

**Application dans Emeraude Engine** :
```cpp
// ✅ CORRECT: Fail-safe design (impossible de crasher)
auto texture = resources.get<TextureResource>("missing.png");
// → Retourne neutral texture, application continue

// ✅ CORRECT: RAII (cleanup automatique)
{
    auto buffer = device->createBuffer(size);
    // Pas besoin de delete, destruction automatique
}

// ✅ CORRECT: Y-down partout (pas de conversion possible)
Vector3 gravity(0, +9.81, 0);  // Seule façon logique
// Impossible de faire -9.81 sans warning explicite

// ❌ ANTI-PATTERN: Permettre l'erreur facilement
auto texture = resources.get<TextureResource>("missing.png");
if (texture == nullptr) {  // Developpeur DOIT vérifier, sinon crash
    // Handle error...
}

// ❌ ANTI-PATTERN: Cleanup manuel requis
auto buffer = device->createBuffer(size);
// Developpeur DOIT appeler destroy(), facile d'oublier
device->destroyBuffer(buffer);
```

**Règles dérivées** :
- Abstractions qui cachent la complexité (Vulkan → Graphics)
- Fail-safe par défaut (neutral resources, never nullptr)
- Types forts pour prévenir erreurs (pas de `int` pour IDs, utiliser types dédiés)
- Compiler errors plutôt que runtime errors quand possible
- Defaults sûrs (mieux aucune action que action dangereuse)

### Éviter le Gulf of Execution
**Définition** : Ne jamais créer un fossé entre l'intention de l'utilisateur et les actions nécessaires pour l'accomplir. Les APIs de haut niveau (user-facing) doivent être simples et directes, sans paramètres complexes ni logique alambiquée.

**Règle fondamentale** : **Les APIs de bout de ligne (end-user APIs) doivent être triviales à utiliser.**

**Application dans Emeraude Engine** :
```cpp
// ✅ CORRECT: API simple, intention claire, zéro complexité
auto texture = resources.get<TextureResource>("albedo.png");
auto buffer = device->createBuffer(size);
camera->lookAt(target);

// ✅ CORRECT: Paramètres minimum, valeurs par défaut intelligentes
auto renderTarget = std::make_shared<RenderTarget::Texture>(
    "MyTarget",
    precisions,
    extent,
    viewDistance
);
// Pas de 15 paramètres optionnels, pas de flags cryptiques

// ❌ VIOLATION Gulf of Execution: Trop de paramètres requis
auto texture = resources.get<TextureResource>(
    "albedo.png",
    VK_FORMAT_R8G8B8A8_SRGB,      // Pourquoi l'utilisateur doit savoir ça ?
    VK_IMAGE_TILING_OPTIMAL,       // Complexité inutile
    VK_IMAGE_USAGE_SAMPLED_BIT,    // Devrait être automatique
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    mipmapLevels,
    arrayLayers
);

// ❌ VIOLATION Gulf of Execution: Logique complexe requise
if (scene->hasCamera()) {
    auto camera = scene->getCamera();
    if (camera->isInitialized()) {
        if (camera->getType() == CameraType::Perspective) {
            // ... 10 lignes de setup
            camera->updateMatrices();
        }
    }
}
// L'utilisateur doit comprendre trop de détails internes
```

**Règles dérivées** :
- **APIs de haut niveau** : Minimum de paramètres, comportement évident
- **Pas de flags complexes** : Éviter VK_*, utiliser enums clairs si nécessaire
- **Pas de setup multi-étapes** : Une fonction fait une chose complètement
- **Pas de vérifications manuelles** : Le système gère les edge cases
- **Documentation par le code** : Le nom de la fonction suffit à comprendre

**Où cette règle s'applique** :
- ✅ **User-facing APIs** : Resources, Scene, Camera, Material (simplicité maximum)
- ⚠️ **Mid-level APIs** : Graphics, Physics (compromis complexité/contrôle)
- ❌ **Low-level APIs** : Vulkan abstractions (complexité acceptable, pas exposée)

**Synergie avec conventions existantes** :
- **Fail-safe Resources** = Pit of Success (impossible de crasher avec nullptr)
- **Y-down System** = Least Astonishment (cohérence totale, pas de surprise)
- **Vulkan Abstraction** = Évite Gulf of Execution (complexité cachée aux utilisateurs)
- **RAII Memory** = Pit of Success (cleanup automatique, pas d'oubli possible)

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