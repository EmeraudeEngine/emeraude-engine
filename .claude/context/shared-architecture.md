# Architecture Emeraude Engine - Context Agents

Vue d'ensemble condensée de l'architecture pour délégation intelligente des agents.

## 🏗️ Systèmes Principaux

### Core Rendering
- **Vulkan/** : Abstraction API Vulkan (Device, Buffer, Pipeline, etc.)
- **Graphics/** : Interface haut niveau (Geometry, Material, Renderable, Renderer)
- **Saphir/** : Génération automatique shaders GLSL (élimine variantes manuelles)

### Scene & Entities
- **Scenes/** : Scene graph (Nodes hiérarchiques, StaticEntity plates, Components)
- **Physics/** : Système 4-entités (Boundaries, Ground, StaticEntity, Nodes)

### Assets & Resources  
- **Resources/** : Chargement asynchrone fail-safe (jamais nullptr)

### Foundation
- **Libs/** : Bibliothèques fondamentales (Math, IO, ThreadPool, etc.)

## 🎯 Conventions Critiques (Non-négociables)

### 1. Y-down Coordinate System
- **Gravité** : +9.81 sur axe Y (vers le bas)
- **Impulsion saut** : valeur Y négative (vers le haut)  
- **Normale sol** : (0, -1, 0)
- **JAMAIS** de flip Y coordinates

### 2. Fail-safe Resource Management
- Les resources ne retournent **JAMAIS** nullptr
- Assets manquants/cassés → neutral fallback resources
- Application ne crash **JAMAIS** sur erreur asset

### 3. Vulkan Abstraction
- **JAMAIS** appeler Vulkan directement depuis user code
- Utiliser abstractions Graphics/ exclusivement
- Complexité Vulkan cachée derrière interface déclarative

### 4. Saphir Shader System
- Shaders générés automatiquement (Material + Geometry)
- Strict compatibility checking (Material requirements ↔ Geometry attributes)
- Pas de fichiers shader manuels

## 🔗 Relations Inter-Systèmes

### Graphics ↔ Vulkan ↔ Saphir
```
User Code → Graphics → Vulkan → GPU
              ↓
           Saphir (shader generation)
```

### Scenes ↔ Physics ↔ Resources
```
Scene Nodes → Physics (MovableTrait) → Collision → Forces
     ↓
Resources (Meshes, Materials) → Graphics → Rendering
```

### Resources ↔ Dependency Chain
```
MeshResource → MaterialResource → TextureResources
                    ↓
            Chargement asynchrone → onDependenciesLoaded()
```

## 📊 Délégation par Subsystem

### Physics (Y-down critique)
- **Focus** : Coordinate compliance, collision accuracy
- **Patterns** : 4-entity system, constraint solver
- **Validation** : Gravité positive, normal vectors corrects

### Graphics/Vulkan (Abstraction critique)  
- **Focus** : No direct Vulkan calls, Saphir usage
- **Patterns** : Geometry+Material→Renderable, instancing
- **Validation** : Shader generation, pipeline creation

### Resources (Fail-safe critique)
- **Focus** : Never nullptr, dependency management  
- **Patterns** : Neutral resources, async loading
- **Validation** : ResourceTrait compliance, error handling

### Scenes (Component architecture)
- **Focus** : Node hierarchy, component lifecycle
- **Patterns** : AbstractEntity + Components, double buffering
- **Validation** : Memory management, observer patterns

## 🚨 Common Violation Patterns

### Y-down Violations
```cpp
// ❌ WRONG  
physics.setGravity(Vector3(0, -9.81f, 0));  // Negative Y gravity
entity.applyForce(Vector3(0, +jumpForce, 0)); // Positive Y jump

// ✅ CORRECT
physics.setGravity(Vector3(0, +9.81f, 0));   // Positive Y gravity  
entity.applyForce(Vector3(0, -jumpForce, 0)); // Negative Y jump
```

### Vulkan Abstraction Violations
```cpp
// ❌ WRONG - Direct Vulkan calls
vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

// ✅ CORRECT - Use abstractions
auto buffer = vulkanDevice.createBuffer(bufferSize, usage);
```

### Fail-safe Violations
```cpp
// ❌ WRONG - Checking nullptr
auto resource = container->getResource("missing.png");
if (resource == nullptr) { /* handle error */ }

// ✅ CORRECT - Never nullptr
auto resource = container->getResource("missing.png");
// Always valid (neutral resource if missing)
```

## 🎯 Agent Routing Guidelines

### Route vers Physics Specialist si :
- Mots-clés : gravity, collision, constraint, y-down, physics
- Fichiers : src/Physics/, MovableTrait usages
- Problèmes : Coordinate system, force applications

### Route vers Graphics Specialist si :
- Mots-clés : shader, vulkan, rendering, saphir, graphics
- Fichiers : src/Graphics/, src/Vulkan/, src/Saphir/
- Problèmes : Vulkan abstraction, shader generation

### Route vers Resources Specialist si :
- Mots-clés : resource, loading, dependency, fail-safe
- Fichiers : src/Resources/, ResourceTrait implémentations  
- Problèmes : Asset loading, dependency chains

### Route vers Scene Specialist si :
- Mots-clés : scene, node, component, entity
- Fichiers : src/Scenes/, Component implementations
- Problèmes : Hierarchy, component lifecycle

Cette architecture guide la délégation intelligente vers les agents appropriés selon l'expertise requise.