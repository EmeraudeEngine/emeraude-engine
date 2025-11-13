# Intégration du ConstraintSolver dans Scene.cpp

## Vue d'ensemble

Le **ConstraintSolver** est maintenant implémenté et testé. Ce guide explique comment l'intégrer dans le pipeline physique existant de `Scenes::Scene` pour remplacer la résolution de collision basique par une résolution basée sur les impulsions.

## Architecture actuelle (AVANT intégration)

### Pipeline physique dans Scene.cpp

```cpp
// Phase 1: Mise à jour de la physique (ligne ~790)
for (auto& entity : m_physicsOctree->elements()) {
    movableEntity->updateSimulation(environmentProperties);
}

// Phase 2: Détection de collisions (ligne ~800)
this->sectorCollisionTest(*m_physicsOctree);

// Phase 3: Résolution de collisions (ligne ~830)
for (auto& entity : m_physicsOctree->elements()) {
    if (collider.hasCollisions()) {
        collider.resolveCollisions(*entity);  // ← Résolution basique position-only
    }
}
```

### Problème avec l'approche actuelle

La méthode `Collider::resolveCollisions()` :
- Déplace uniquement les positions (correction directe)
- N'applique pas d'impulsions réalistes
- Ne gère pas la rotation lors des impacts
- Ne conserve pas le momentum
- Pas de rebond (restitution)

## Architecture proposée (APRÈS intégration)

### Nouveau pipeline avec ConstraintSolver

```cpp
// Phase 1: Mise à jour de la physique (inchangé)
for (auto& entity : m_physicsOctree->elements()) {
    movableEntity->updateSimulation(environmentProperties);
}

// Phase 2: Détection de collisions → Génération de manifolds
std::vector<ContactManifold> manifolds;
this->sectorCollisionTestWithManifolds(*m_physicsOctree, manifolds);

// Phase 3: Résolution par impulsions (NOUVEAU)
if (!manifolds.empty()) {
    m_constraintSolver.solve(manifolds, m_elapsedTime);
}

// Phase 4: Intégration des positions (déjà géré par updateSimulation)
```

## Modifications à apporter

### 1. Ajouter le ConstraintSolver dans Scene.hpp

```cpp
// Dans src/Scenes/Scene.hpp
#include "Physics/ConstraintSolver.hpp"  // ← Ajouter cette inclusion

class Scene {
    // ...
private:
    Physics::ConstraintSolver m_constraintSolver{8, 3};  // ← Ajouter ce membre
    // 8 velocity iterations, 3 position iterations (valeurs par défaut)
};
```

### 2. Modifier Collider pour générer des ContactManifolds

#### Option A: Modifier checkCollisionAgainstMovable (RECOMMANDÉ)

Ajouter une surcharge qui génère des manifolds au lieu de Collision :

```cpp
// Dans src/Physics/Collider.hpp
#include "ContactManifold.hpp"

class Collider {
public:
    /**
     * @brief Checks collision and creates a contact manifold if collision occurs.
     * @param movableEntityA Reference to first movable entity.
     * @param movableEntityB Reference to second movable entity.
     * @param outManifolds Vector to store generated manifolds.
     * @return bool True if collision detected.
     */
    bool checkCollisionAgainstMovableWithManifold(
        Scenes::AbstractEntity& movableEntityA,
        Scenes::AbstractEntity& movableEntityB,
        std::vector<ContactManifold>& outManifolds
    ) noexcept;
};
```

#### Option B: Convertir les Collisions existantes en ContactManifolds (TEMPORAIRE)

```cpp
// Dans src/Scenes/Scene.cpp, après sectorCollisionTest()
std::vector<Physics::ContactManifold> manifolds;

for (auto& entity : m_physicsOctree->elements()) {
    auto* movableEntity = entity->getMovableTrait();
    if (!movableEntity || !movableEntity->isMovable()) continue;

    auto& collider = movableEntity->collider();
    if (!collider.hasCollisions()) continue;

    // Convertir les collisions en manifolds
    for (const auto& collision : collider.collisions()) {
        if (collision.type() != Physics::CollisionType::WithEntity) continue;

        auto* otherEntity = collision.entity();
        if (!otherEntity) continue;

        Physics::ContactManifold manifold(
            movableEntity,
            otherEntity->getMovableTrait()
        );

        // Créer un point de contact depuis la collision
        auto contactPos = collision.position();
        auto contactNormal = collision.direction().normalized();
        float penetration = 0.1F;  // Estimation (améliorer avec MTV du SAT)

        manifold.addContact(contactPos, contactNormal, penetration);
        manifolds.push_back(manifold);
    }

    collider.reset();  // Clear old collisions
}

// Résoudre avec le solver
if (!manifolds.empty()) {
    m_constraintSolver.solve(manifolds, m_elapsedTime);
}
```

### 3. Implémenter checkCollisionAgainstMovableWithManifold (PRODUCTION)

```cpp
// Dans src/Physics/Collider.cpp
bool Collider::checkCollisionAgainstMovableWithManifold(
    Scenes::AbstractEntity& entityA,
    Scenes::AbstractEntity& entityB,
    std::vector<ContactManifold>& outManifolds
) noexcept
{
    auto* movableA = entityA.getMovableTrait();
    auto* movableB = entityB.getMovableTrait();

    if (!movableA || !movableB) return false;

    float overflow = 0.0F;
    Libs::Math::Vector<3, float> direction;

    // Détection de collision (utilise SAT existant)
    bool hasCollision = false;

    if (entityA.collisionModelType() == Scenes::CollisionModelType::Sphere &&
        entityB.collisionModelType() == Scenes::CollisionModelType::Sphere)
    {
        hasCollision = isSphereCollisionWith(entityA, entityB, overflow, direction);
    }
    else if (entityA.collisionModelType() == Scenes::CollisionModelType::Box &&
             entityB.collisionModelType() == Scenes::CollisionModelType::Box)
    {
        hasCollision = isBoxCollisionWith(entityA, entityB, overflow, direction);
    }

    if (!hasCollision) return false;

    // Créer un manifold
    ContactManifold manifold(movableA, movableB);

    // Point de contact approximatif (centre entre les deux objets)
    auto posA = entityA.getWorldPosition();
    auto posB = entityB.getWorldPosition();
    auto contactPos = (posA + posB) * 0.5F;

    // Normale pointe de A vers B
    auto normal = direction.normalized();

    // Profondeur de pénétration
    float penetration = overflow;

    manifold.addContact(contactPos, normal, penetration);
    outManifolds.push_back(manifold);

    return true;
}
```

### 4. Modifier Scene.cpp pour utiliser le nouveau système

```cpp
// Dans src/Scenes/Scene.cpp, méthode sectorCollisionTest() (ligne ~1050)

// Remplacer l'appel à checkCollisionAgainstMovable par :
if (entityBHasMovableAbility) {
    colliderA.checkCollisionAgainstMovableWithManifold(*entityA, *entityB, m_manifolds);
}
```

```cpp
// Dans src/Scenes/Scene.cpp, méthode onFrame() (ligne ~830)

// Remplacer la section de résolution de collisions par :
std::vector<Physics::ContactManifold> manifolds;

for (const auto& entity : m_physicsOctree->elements()) {
    auto* movableEntity = entity->getMovableTrait();
    if (!movableEntity || !movableEntity->isMovable()) continue;

    auto& collider = movableEntity->collider();
    if (!collider.hasCollisions()) continue;

    // Convertir collisions en manifolds (Option B temporaire)
    // OU les manifolds ont déjà été créés dans sectorCollisionTest (Option A)

    collider.reset();
}

// Résolution avec le ConstraintSolver
if (!manifolds.empty()) {
    m_constraintSolver.solve(manifolds, m_elapsedTime);
}
```

### 5. Mettre à jour l'inertia tensor lors des rotations

```cpp
// Dans src/Scenes/Node.cpp, après chaque rotation appliquée
void Node::applyRotation(const Quaternion& rotation) {
    // ... code existant pour appliquer la rotation ...

    // Mettre à jour l'inverse world inertia tensor
    if (this->hasMovableTrait()) {
        auto rotationMatrix = rotation.toRotationMatrix();
        this->movableTrait().updateInverseWorldInertia(rotationMatrix);
    }
}
```

## Configuration du Solver

### Paramètres par défaut

```cpp
// Dans Scene.hpp
Physics::ConstraintSolver m_constraintSolver{8, 3};
```

### Ajuster selon le type de scène

```cpp
// Scènes avec beaucoup d'empilements (ex: BallsOfSteelScene)
m_constraintSolver.setVelocityIterations(12);
m_constraintSolver.setPositionIterations(4);

// Scènes rapides avec peu d'objets (ex: SimpleRoomScene)
m_constraintSolver.setVelocityIterations(6);
m_constraintSolver.setPositionIterations(2);
```

## Tests recommandés

### Test 1: PhysicsDebugScene
- 3 sphères de masses différentes
- Vérifier les rebonds avec restitution
- Vérifier la stabilité (pas d'oscillations)

### Test 2: BallsOfSteelScene
- 1000 balles
- Vérifier les performances
- Vérifier la stabilité des empilements

### Test 3: CollisionScene
- Impacts avec rotation
- Vérifier le moment angulaire
- Vérifier la conservation du momentum

## Limitations connues

### Ce qui EST implémenté ✓
- Résolution d'impulsion avec rotation
- Baumgarte stabilization (correction de position)
- Warm starting (accumulation d'impulsion)
- Support objets statiques
- Restitution (bounciness)

### Ce qui N'EST PAS encore implémenté ✗
1. **Friction tangentielle** - Les objets glissent sans friction latérale
2. **Contact generation robuste** - Un seul point de contact approximatif
3. **Continuous Collision Detection (CCD)** - Les objets rapides peuvent traverser
4. **Joints et contraintes** - Pas de hinges, sliders, etc.
5. **Island management** - Tous les objets sont résolus même s'ils ne se touchent pas

## Prochaines étapes

### Court terme (intégration basique)
1. Ajouter `m_constraintSolver` dans Scene.hpp
2. Implémenter Option B (conversion temporaire)
3. Tester sur PhysicsDebugScene
4. Ajuster les paramètres d'itération

### Moyen terme (production)
1. Implémenter `checkCollisionAgainstMovableWithManifold()`
2. Améliorer la génération de points de contact (utiliser MTV du SAT)
3. Ajouter plusieurs points de contact pour box-box (jusqu'à 4)
4. Optimiser avec island management

### Long terme (features avancées)
1. Implémenter la friction (Coulomb friction model)
2. Ajouter CCD pour objets rapides
3. Implémenter les joints (hinge, slider, distance)
4. Optimiser avec sleeping objects

## Résumé des changements de fichiers

| Fichier | Changement | Type |
|---------|-----------|------|
| `Scenes/Scene.hpp` | Ajouter `m_constraintSolver` | Membre |
| `Scenes/Scene.cpp` | Remplacer résolution collisions | Logique |
| `Physics/Collider.hpp` | Ajouter `checkCollisionAgainstMovableWithManifold()` | Nouvelle méthode |
| `Physics/Collider.cpp` | Implémenter génération de manifolds | Nouvelle méthode |
| `Scenes/Node.cpp` | Mettre à jour `inverseWorldInertia` après rotation | Hook |

## Contact et support

- Documentation complète: `src/Physics/CONSTRAINT_SOLVER_INTEGRATION.md`
- Tests unitaires: `src/Testing/test_PhysicsConstraintSolver.cpp`
- Référence algorithme: "Iterative Dynamics with Temporal Coherence" - Erin Catto (GDC 2005)
