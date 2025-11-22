# Testing System - Development Context

Context spécifique pour le développement des tests unitaires d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Tests unitaires du moteur utilisant **Google Test**. Focus actuel sur **Libs** (fondation critique), expansion future vers systèmes haut niveau.

## 📋 Règles spécifiques à Testing/

### Framework: Google Test
- **Google Test (gtest)** : Framework de tests unitaires C++
- **CTest integration** : Lancé via `ctest` pour CI/CD
- **Assertions standard** : EXPECT_*, ASSERT_*, etc.

### Conventions de test OBLIGATOIRES

**Organisation fichiers** :
- **Un fichier par classe/concept** : Un test file par unité testée
- Nommage : `ClassNameTest.cpp` ou `ConceptTest.cpp`

**Organisation tests** :
- **Un test par fonction** : Chaque fonction a son propre TEST()
- **Variantes dans le même test** : Appels alternatifs/edge cases regroupés
- Nommage test : `TEST(ClassName, FunctionName)` ou `TEST(ClassName, FunctionName_EdgeCase)`

**Exemple** :
```cpp
// VectorTest.cpp - teste Libs/Math/Vector

TEST(Vector, Constructor) {
    Vector3 v1(1.0f, 2.0f, 3.0f);
    EXPECT_EQ(v1.x, 1.0f);
    EXPECT_EQ(v1.y, 2.0f);
    EXPECT_EQ(v1.z, 3.0f);

    // Variante: constructeur par défaut
    Vector3 v2;
    EXPECT_EQ(v2.x, 0.0f);
    EXPECT_EQ(v2.y, 0.0f);
    EXPECT_EQ(v2.z, 0.0f);
}

TEST(Vector, Length) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);

    // Edge case: vecteur zéro
    Vector3 zero(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(zero.length(), 0.0f);
}

TEST(Vector, Normalize) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    v.normalize();
    EXPECT_FLOAT_EQ(v.length(), 1.0f);
    EXPECT_FLOAT_EQ(v.x, 0.6f);
    EXPECT_FLOAT_EQ(v.y, 0.8f);

    // Edge case: normaliser vecteur zéro
    Vector3 zero(0.0f, 0.0f, 0.0f);
    zero.normalize();  // Ne doit pas crash
    EXPECT_TRUE(std::isnan(zero.x) || zero.x == 0.0f);
}
```

### Priorité des tests

**Actuellement testé** :
- **Libs** : Priorité absolue (fondation du moteur)
  - Math (Vector, Matrix, Quaternion, CartesianFrame)
  - Algorithms
  - IO
  - ThreadPool
  - Observer/Observable
  - Etc.

**Future expansion** :
- Resources (chargement, fail-safe, dépendances)
- Physics (collisions, contraintes, intégration)
- Graphics (géométrie, matériaux, renderables)
- Scenes (nodes, components, transformations)
- Saphir (génération shaders, compatibilité)

### Stratégie de montée en pile
1. **Libs 100% testé** : Base solide garantie
2. **Systèmes fondamentaux** : Resources, Physics
3. **Systèmes graphiques** : Graphics, Saphir
4. **Systèmes haut niveau** : Scenes, Audio, Overlay
5. **Integration tests** : Tests inter-systèmes

## 🛠️ Commandes de développement

```bash
# Lancer tous les tests
ctest

# Lancer tests en parallèle
ctest --parallel $(nproc)

# Lancer tests spécifiques
ctest -R Vector           # Tests contenant "Vector"
ctest -R Libs             # Tous tests Libs
./test --gtest_filter="Vector.*"  # Google Test filter

# Mode verbose
ctest --verbose
ctest --output-on-failure

# Lancer l'exécutable de test directement
./test                    # Tous les tests
./test --gtest_list_tests # Lister tests disponibles
```

## 🔗 Fichiers importants

### Structure Testing/
```
Testing/
├── Libs/                  # Tests Libs (priorité actuelle)
│   ├── Math/
│   │   ├── VectorTest.cpp
│   │   ├── MatrixTest.cpp
│   │   ├── QuaternionTest.cpp
│   │   └── CartesianFrameTest.cpp
│   ├── IO/
│   │   └── FileSystemTest.cpp
│   ├── ThreadPoolTest.cpp
│   └── ObserverTest.cpp
├── Resources/             # Future
├── Physics/               # Future
└── CMakeLists.txt
```

## ⚡ Patterns de développement

### Créer un nouveau test
```cpp
// 1. Créer fichier Testing/Category/ClassTest.cpp
#include <gtest/gtest.h>
#include "Libs/Math/Vector.hpp"

// 2. Un test par fonction
TEST(Vector, Add) {
    Vector3 a(1, 2, 3);
    Vector3 b(4, 5, 6);
    Vector3 result = a + b;

    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 7);
    EXPECT_EQ(result.z, 9);
}

// 3. Tester edge cases
TEST(Vector, Add_WithZero) {
    Vector3 a(1, 2, 3);
    Vector3 zero(0, 0, 0);
    Vector3 result = a + zero;

    EXPECT_EQ(result, a);
}

// 4. Tester cas limites
TEST(Vector, Add_Overflow) {
    Vector3 a(FLT_MAX, 0, 0);
    Vector3 b(1, 0, 0);
    Vector3 result = a + b;
    // Vérifier comportement overflow
}
```

### Types d'assertions Google Test
```cpp
// Égalité
EXPECT_EQ(a, b);      // a == b
EXPECT_NE(a, b);      // a != b

// Comparaison
EXPECT_LT(a, b);      // a < b
EXPECT_LE(a, b);      // a <= b
EXPECT_GT(a, b);      // a > b
EXPECT_GE(a, b);      // a >= b

// Flottants (avec epsilon)
EXPECT_FLOAT_EQ(a, b);   // ~equal pour float
EXPECT_DOUBLE_EQ(a, b);  // ~equal pour double
EXPECT_NEAR(a, b, eps);  // |a - b| <= eps

// Booléens
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Pointeurs
EXPECT_EQ(ptr, nullptr);
EXPECT_NE(ptr, nullptr);

// ASSERT_* variants: stop test on failure
ASSERT_EQ(a, b);  // Si échec, arrête le test
```

### Tests avec fixtures (setup/teardown)
```cpp
class VectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup avant chaque test
        v1 = Vector3(1, 2, 3);
        v2 = Vector3(4, 5, 6);
    }

    void TearDown() override {
        // Cleanup après chaque test
    }

    Vector3 v1, v2;
};

TEST_F(VectorTest, Add) {
    // Utilise v1 et v2 du fixture
    Vector3 result = v1 + v2;
    EXPECT_EQ(result.x, 5);
}
```

## 🚨 Points d'attention

- **Libs prioritaire** : Fondation critique, doit être 100% testée
- **Un test = une fonction** : Clarté et isolation
- **Edge cases obligatoires** : Tester valeurs limites, zéro, négatif, overflow
- **EXPECT vs ASSERT** : EXPECT continue après échec, ASSERT arrête
- **Float comparison** : Utiliser EXPECT_FLOAT_EQ, pas EXPECT_EQ
- **Tests rapides** : Éviter tests longs (< 1 seconde idéalement)
- **Pas de randomness** : Tests reproductibles, pas de valeurs aléatoires
- **CI/CD integration** : Tests lancés automatiquement sur commits

## 📚 Documentation détaillée

Systèmes testés:
→ **@src/Libs/AGENTS.md** - Priorité actuelle des tests
→ **Google Test documentation** - Pour assertions et features avancées
→ **CTest documentation** - Pour intégration CMake
