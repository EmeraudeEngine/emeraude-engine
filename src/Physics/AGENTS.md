# Physics System - Development Context

Context spécifique pour le développement du système physique d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Le système physique d'Emeraude Engine implémente une architecture à 4 types d'entités avec gestion différenciée des collisions pour équilibrer réalisme, performance et design de jeu.

## 📋 Règles spécifiques à Physics/

### Convention de coordonnées CRITIQUE
- **Y-DOWN obligatoire** dans tous les calculs physiques
- Gravité : `+9.81` sur l'axe Y (tire vers le bas)
- Impulsion de saut : valeur Y négative (pousse vers le haut)
- Poussée vers l'avant : valeur Z négative

### Types d'entités (4 types distincts)
1. **Boundaries** : Contraintes de jeu (murs invisibles)
2. **Ground** : Surfaces physiques hybrides (stabilité + réalisme)  
3. **StaticEntity** : Objets statiques avec masse définie
4. **Nodes** : Entités dynamiques complètes

### Ordre d'exécution physique
1. Intégration des forces → 2. Broad phase → 3. Narrow phase → 4. Résolution inter-entités → 5. Collision sol → 6. Collision limites → 7. Résolution sol

## 🛠️ Commandes de développement

```bash
# Tests spécifiques physique
ctest -R Physics
./test --filter="*Physics*"

# Debug physique
./Emeraude --debug-physics
./Emeraude --show-collisions
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire principal du système physique
- `ConstraintSolver.cpp/.hpp` - Résolution des contraintes par impulsions
- `ContactManifold.cpp/.hpp` - Structure de données de collision
- `Collider.cpp/.hpp` - Détection de collision
- `@docs/physics-system.md` - Architecture détaillée

## ⚡ Patterns de développement

### Ajout d'un nouveau type de collision
1. Définir la méthode dans `Collider`
2. Créer les manifolds appropriés
3. Tester avec les 4 types d'entités
4. Vérifier la cohérence des coordonnées Y-down

### Modification du solveur
1. Maintenir les 8 itérations de vélocité, 3 de position
2. Appliquer les impulsions selon le type d'entité
3. Respecter la séparation entités/sol
4. Préserver la séparation boundaries (pas de manifolds)

## 🚨 Points d'attention

- **JAMAIS** de conversion de coordonnées Y
- Calcul de `penetrationDepth` AVANT hard clipping (sol)
- Mass matters pour StaticEntity (pas de masse infinie)
- Deux appels séparés au solveur (entités puis sol)