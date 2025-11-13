# Constraint Solver - Guide d'Intégration

## Vue d'Ensemble

Le **Sequential Impulse Solver** est maintenant implémenté dans Emeraude-Engine. Ce système résout les collisions et contraintes physiques de manière itérative pour obtenir une simulation robuste et stable.

## Composants Ajoutés

### 1. ContactPoint (ContactPoint.hpp)
Structure représentant un point de contact unique entre deux corps.

**Données principales :**
- `positionWorld` : Position du contact en world space
- `normal` : Normale de contact (de A vers B)
- `penetrationDepth` : Profondeur de pénétration
- `accumulatedNormalImpulse` : Impulsion accumulée (pour warm starting)

### 2. ContactManifold (ContactManifold.hpp)
Classe gérant un ensemble de contacts entre deux objets (jusqu'à 4 points).

**Méthodes principales :**
```cpp
ContactManifold manifold(bodyA, bodyB);
manifold.addContact(worldPos, normal, penetration);
manifold.prepare();  // Prépare les contacts pour le solving
```

### 3. MovableTrait - Nouvelles Méthodes

**Ajouts :**
```cpp
// Application d'impulsion (différent de force !)
void applyLinearImpulse(const Vector<3, float>& impulse);
void applyAngularImpulse(const Vector<3, float>& angularImpulse);

// Gestion du tenseur d'inertie
void updateInverseWorldInertia(const Matrix<3, float>& rotationMatrix);
const Matrix<3, float>& inverseWorldInertia() const;
```

**Variable membre ajoutée :**
```cpp
Matrix<3, float> m_inverseWorldInertia;  // I^-1 transformé en world space
```

### 4. ConstraintSolver (ConstraintSolver.hpp)
Le solveur itératif principal.

**Usage :**
```cpp
ConstraintSolver solver(8, 3);  // 8 velocity iterations, 3 position iterations
solver.solve(manifolds, deltaTime);
```

## Intégration dans la Boucle Physique

### Étape 1 : Mise à Jour du Tenseur d'Inertie

**Quand :** Après chaque rotation d'un objet MovableTrait.

```cpp
// Dans la méthode qui applique une rotation à l'entité
void Entity::applyRotation(const Quaternion<float>& rotation) {
    // Appliquer la rotation à la scène node...

    // Obtenir la matrice de rotation 3x3
    Matrix<3, float> rotationMatrix = rotation.toRotationMatrix();

    // Mettre à jour le tenseur d'inertie en world space
    if (hasMovableTrait()) {
        movableTrait().updateInverseWorldInertia(rotationMatrix);
    }
}
```

### Étape 2 : Génération des Manifolds depuis SAT

**Modification de Collider.cpp ou équivalent :**

```cpp
// Dans checkCollisionAgainstMovable() ou équivalent
bool Collider::checkCollisionAgainstMovable(
    Scenes::AbstractEntity& entityA,
    Scenes::AbstractEntity& entityB,
    std::vector<ContactManifold>& outManifolds
) {
    // 1. Détection de collision (SAT existant)
    Vector<3, float> MTV;
    if (!SAT::checkCollision(verticesA, verticesB, MTV)) {
        return false;  // Pas de collision
    }

    // 2. Créer un manifold
    ContactManifold manifold(
        &entityA.movableTrait(),
        &entityB.movableTrait()
    );

    // 3. Générer les points de contact
    // NOTE: Ici tu dois implémenter la génération de points de contact
    // Pour un prototype simple, on peut utiliser MTV :

    Vector<3, float> contactNormal = MTV.normalized();
    float penetration = MTV.length();

    // Point de contact approximatif (centre de A projeté)
    Vector<3, float> contactPos = entityA.getWorldPosition() + MTV * 0.5F;

    manifold.addContact(contactPos, contactNormal, penetration);

    // 4. Ajouter le manifold à la liste
    outManifolds.push_back(manifold);

    return true;
}
```

### Étape 3 : Pipeline Physique Complet

**Dans Physics::Manager ou équivalent :**

```cpp
void PhysicsManager::update(float deltaTime) {
    std::vector<ContactManifold> manifolds;

    // 1. Intégration des forces → vélocité
    for (auto* entity : movableEntities) {
        entity->movableTrait().updateSimulation(environmentProperties);
    }

    // 2. Broad Phase (détection grossière)
    auto potentialPairs = broadPhase.detectPotentialCollisions();

    // 3. Narrow Phase (détection précise avec SAT)
    for (auto& pair : potentialPairs) {
        collider.checkCollisionAgainstMovable(*pair.A, *pair.B, manifolds);
    }

    // 4. *** NOUVEAU : Résolution par impulsions ***
    ConstraintSolver solver(8, 3);
    solver.solve(manifolds, deltaTime);

    // 5. Intégration des vélocités → position
    for (auto* entity : movableEntities) {
        auto newPos = entity->getWorldPosition() +
                     entity->movableTrait().linearVelocity() * deltaTime;
        entity->setWorldPosition(newPos);

        // Rotation
        if (entity->movableTrait().isRotationPhysicsEnabled()) {
            auto angularVel = entity->movableTrait().angularVelocity();
            float angle = angularVel.length() * deltaTime;
            if (angle > 0.0F) {
                auto axis = angularVel.normalized();
                Quaternion<float> deltaRot(axis, angle);
                entity->applyRotation(deltaRot);
            }
        }
    }
}
```

## Génération de Points de Contact (Contact Generation)

### Méthode Simple (Prototype)

Pour commencer, tu peux utiliser une approche simplifiée :

```cpp
// Un seul point de contact au centre de pénétration
Vector<3, float> contactPos = centerA + MTV * 0.5F;
manifold.addContact(contactPos, MTV.normalized(), MTV.length());
```

### Méthode Robuste (Production)

Pour une physique de qualité production, il faut générer plusieurs points :

**Box-Box :** Jusqu'à 4 points (coins d'une face contre l'autre)
**Sphere-Box :** 1 point (point le plus proche sur la box)
**Box-Plan :** Jusqu'à 4 points (coins de la box en contact)

Référence : "Robust Contact Creation for Physics Simulations" (Dirk Gregorius, GDC 2013)

## Exemple Complet : Deux Boîtes en Collision

```cpp
// Scène : Box A tombe sur Box B (statique)
void setupScene() {
    // Box A (dynamique)
    auto* boxA = createEntity("BoxA");
    boxA->setPosition({0, 10, 0});
    boxA->setPhysicalProperties(10.0F, 1.0F, 0.5F, 0.3F, 0.5F);  // 10kg
    boxA->enableMovement(true);
    boxA->enableRotationPhysics(true);

    // Configurer tenseur d'inertie pour un cube (1m x 1m x 1m)
    float m = 10.0F;  // masse
    float I = m * (1.0F*1.0F + 1.0F*1.0F) / 12.0F;  // I = m*(h²+d²)/12
    Matrix<3, float> inertia;
    inertia[M3x3Col0Row0] = I;
    inertia[M3x3Col1Row1] = I;
    inertia[M3x3Col2Row2] = I;
    boxA->setInertia(inertia);

    // Box B (statique)
    auto* boxB = createEntity("BoxB");
    boxB->setPosition({0, 0, 0});
    boxB->enableMovement(false);  // Statique
}

// Boucle physique
void physicsStep(float dt) {
    std::vector<ContactManifold> manifolds;

    // Détection
    if (checkCollision(boxA, boxB, manifolds)) {
        // Résolution
        ConstraintSolver solver;
        solver.solve(manifolds, dt);
    }

    // Intégration
    updatePositions(dt);
}
```

## Tuning des Paramètres

### Iterations du Solver

```cpp
// Performance vs Précision
solver.setVelocityIterations(8);   // Défaut: 8 (range: 4-20)
solver.setPositionIterations(3);   // Défaut: 3 (range: 1-5)

// Jeux rapides (60+ fps) : (6, 2)
// Jeux standards : (8, 3)
// Simulations précises : (12, 4)
// Empilements complexes : (20, 5)
```

### Constantes Baumgarte

Dans `ConstraintSolver::prepareContacts()` :

```cpp
constexpr float baumgarteSlop = 0.01F;     // Pénétration tolérée (1cm)
constexpr float baumgarteFactor = 0.2F;    // Force de correction [0-1]

// Objets petits (< 10cm) : slop = 0.001F
// Objets standards : slop = 0.01F
// Objets énormes (> 10m) : slop = 0.1F
```

### Correction de Position

Dans `ConstraintSolver::solvePositionConstraints()` :

```cpp
constexpr float positionCorrectionSlop = 0.005F;    // Seuil de correction (5mm)
constexpr float positionCorrectionFactor = 0.3F;    // Intensité [0-1]

// Stable mais permissif : factor = 0.2F
// Équilibré : factor = 0.3F
// Strict (peut osciller) : factor = 0.5F
```

## Tests Recommandés

### Test 1 : Conservation du Momentum

```cpp
// Deux boîtes de même masse, collision frontale
BoxA: mass=1kg, velocity=+5 m/s →
BoxB: mass=1kg, velocity=-5 m/s ←

// Après collision avec restitution=1.0 :
// Momentum total doit rester ~0
// Les boîtes rebondissent avec vitesses inversées
```

### Test 2 : Rotation par Impact Décentré

```cpp
// Boîte immobile, sphère frappe un coin
Box: position=(0,0,0), rotation=(0,0,0), velocity=(0,0,0)
Sphere: velocity=(-10, 0, 0) → frappe coin en (0.5, 0.5, 0)

// Résultat attendu :
// - Box acquiert vélocité linéaire + angulaire
// - Rotation autour de l'axe perpendiculaire au plan d'impact
```

### Test 3 : Empilement Stable

```cpp
// 3 boîtes empilées verticalement
// Après 5 secondes :
// - Pas d'oscillations
// - Pas d'explosion
// - Pénétration < 1cm
```

## Limitations Actuelles & Prochaines Étapes

### ✅ Implémenté
- Résolution d'impulsion avec rotation
- Baumgarte stabilization
- Warm starting (accumulation d'impulsion)
- Support objets statiques

### ⚠️ À Implémenter
1. **Contact Generation** robuste (actuellement simplifié)
2. **Friction** (tangentielle, actuellement `accumulatedTangentImpulse` inutilisé)
3. **Continuous Collision Detection** (CCD) pour objets rapides
4. **Joints** (hinge, slider, distance constraints)
5. **Sleeping/Island management** pour performance

### 📚 Ressources

- "Iterative Dynamics with Temporal Coherence" - Erin Catto (GDC 2005)
- "Robust Contact Creation" - Dirk Gregorius (GDC 2013)
- "Physics for Game Programmers" - Ian Millington (livre)
- Box2D source code (référence 2D, concepts applicables en 3D)

---

**Note :** Ce solver est un excellent point de départ pour un moteur physique robuste. Avec l'ajout de la friction et du CCD, tu auras un système comparable à Bullet/PhysX pour des cas d'usage standards.
