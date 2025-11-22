# Spécification : Commandes Build Multi-Plateforme (macOS + Linux)

**Date**: 2025-11-22
**Objectif**: Garantir la compatibilité des commandes slash de build entre macOS et Linux
**Statut**: En développement

## 🎯 Contexte

Les commandes slash actuelles dans `.claude/commands/` doivent fonctionner de manière transparente sur :
- **macOS** (Darwin, Apple Silicon ARM64 et Intel x86_64)
- **Linux** (Debian 13, Ubuntu 24.04, x86_64 et ARM64)

Actuellement, les commandes ont été développées et testées principalement sur Linux. Cette spec documente les adaptations nécessaires pour macOS tout en préservant la compatibilité Linux.

## 📋 Différences Plateforme

### 1. Nombre de Processeurs

**Linux**:
```bash
nproc
```

**macOS**:
```bash
sysctl -n hw.ncpu
```

**Solution Multi-Plateforme**:
```bash
# Détection automatique du nombre de CPU
if [[ "$OSTYPE" == "darwin"* ]]; then
    NCPU=$(sysctl -n hw.ncpu)
else
    NCPU=$(nproc)
fi
```

### 2. Générateur CMake

**Linux (par défaut)**:
- Unix Makefiles (make)
- Ou Ninja si installé

**macOS (recommandé)**:
- Unix Makefiles (make) - Compatible partout
- Xcode - Pour développement IDE
- Ninja - Pour builds optimisés

**Solution Actuelle**:
Ne pas spécifier de générateur → CMake choisit automatiquement le meilleur disponible.

```bash
# Laisse CMake décider (recommandé)
cmake ..

# OU spécifier explicitement si besoin
cmake -G "Unix Makefiles" ..
cmake -G "Ninja" ..           # Si Ninja installé
cmake -G "Xcode" ..           # macOS uniquement
```

### 3. Chemins et Dépendances

**Différences potentielles** :
- Emplacement de Vulkan SDK
- Librairies système (OpenAL, etc.)
- Compilateurs (GCC vs Clang)

**Gestion** :
CMake gère automatiquement via `find_package()` et variables d'environnement.

### 4. Variables d'Environnement Build

**macOS spécifiques** :
```bash
# Vulkan SDK (si installé via LunarG)
export VULKAN_SDK="/Users/$USER/VulkanSDK/[version]/macOS"

# Architectures (Apple Silicon)
export CMAKE_OSX_ARCHITECTURES="arm64"  # Ou "x86_64" pour Intel
```

**Linux spécifiques** :
```bash
# Généralement pas besoin de variables spéciales
# Vulkan via package manager
```

## 🔧 Modifications des Commandes

### Commandes à Adapter

#### 1. `/build-test`
**Fichier**: `.claude/commands/build-test.md`

**Changements** :
```bash
# AVANT (Linux-only)
cmake --build . --parallel $(nproc)

# APRÈS (Multi-plateforme)
if [[ "$OSTYPE" == "darwin"* ]]; then
    cmake --build . --parallel $(sysctl -n hw.ncpu)
else
    cmake --build . --parallel $(nproc)
fi
```

**OU utiliser fonction helper** :
```bash
# Fonction à ajouter en début de commande
get_ncpu() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sysctl -n hw.ncpu
    else
        nproc
    fi
}

# Utilisation
cmake --build . --parallel $(get_ncpu)
```

#### 2. `/quick-test`
**Fichier**: `.claude/commands/quick-test.md`

**Changements** : Identiques à `/build-test`

#### 3. `/full-test`
**Fichier**: `.claude/commands/full-test.md`

**Changements** : Identiques à `/build-test`

#### 4. `/build-only`
**Fichier**: `.claude/commands/build-only.md`

**Changements** : Identiques à `/build-test`

#### 5. `/clean-rebuild`
**Fichier**: `.claude/commands/clean-rebuild.md`

**Changements** : Identiques à `/build-test`

#### 6. `/build-release`
**Fichier**: `.claude/commands/build-release.md`

**Changements** : Identiques à `/build-test`

### Commandes Sans Modification Nécessaire

- `/check-conventions` - Scripts Python/Grep (portable)
- `/verify-y-down` - Scripts Python/Grep (portable)
- `/show-architecture` - Lecture fichiers (portable)
- `/doc-system` - Lecture fichiers (portable)
- `/find-usage` - Grep (portable)
- `/check-references` - Scripts Python (portable)
- `/add-resource-type` - Génération fichiers (portable)

## 🛠️ Implémentation Recommandée

### Option 1: Fonction Helper Globale (RECOMMANDÉ)

Créer un fichier de fonctions communes :

**`.claude/commands/common.sh`** :
```bash
#!/bin/bash

# Retourne le nombre de CPU de manière portable
get_ncpu() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sysctl -n hw.ncpu
    else
        nproc
    fi
}

# Détecte la plateforme
get_platform() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    else
        echo "unknown"
    fi
}

# Export pour utilisation dans les commandes
export -f get_ncpu
export -f get_platform
```

**Utilisation dans les commandes** :
```bash
# En début de chaque commande .md
source "$(dirname "$0")/common.sh"

# Puis
cmake --build . --parallel $(get_ncpu)
```

### Option 2: Inline dans Chaque Commande

Inclure directement la détection dans chaque commande :

```bash
# Détection plateforme
if [[ "$OSTYPE" == "darwin"* ]]; then
    NCPU=$(sysctl -n hw.ncpu)
else
    NCPU=$(nproc)
fi

# Build
cmake --build . --parallel $NCPU
```

**Avantages** : Pas de dépendance externe
**Inconvénients** : Code dupliqué

### Option 3: Variable CMAKE (PROPRE)

Utiliser CMake pour gérer le parallélisme :

```bash
# CMake 3.12+ supporte --parallel sans argument
# Il détecte automatiquement le nombre de CPU
cmake --build . --parallel

# Fonctionne sur macOS ET Linux !
```

**RECOMMANDATION FINALE** : Utiliser **Option 3** (la plus simple et portable)

## 📝 Plan de Mise en Œuvre

### Phase 1: Tests sur macOS
- [x] Tester build-test sur macOS ✅ (fonctionnel avec .claude-build-debug/)
- [ ] Tester full-test sur macOS
- [ ] Tester quick-test sur macOS
- [ ] Tester build-release sur macOS
- [ ] Tester clean-rebuild sur macOS

### Phase 2: Modification des Commandes
- [ ] Modifier `/build-test` avec Option 3
- [ ] Modifier `/quick-test` avec Option 3
- [ ] Modifier `/full-test` avec Option 3
- [ ] Modifier `/build-only` avec Option 3
- [ ] Modifier `/clean-rebuild` avec Option 3
- [ ] Modifier `/build-release` avec Option 3

### Phase 3: Validation Cross-Platform
- [ ] Tester toutes les commandes sur macOS ARM64 (Apple Silicon)
- [ ] Tester toutes les commandes sur macOS x86_64 (Intel)
- [ ] Tester toutes les commandes sur Linux x86_64 (Debian/Ubuntu)
- [ ] Documenter les résultats

### Phase 4: Documentation
- [ ] Mettre à jour `.claude/commands/README.md`
- [ ] Ajouter notes de compatibilité dans chaque commande
- [ ] Créer section "Multi-Platform Support" dans CLAUDE.md

## ⚠️ Points d'Attention

### macOS Spécifique

1. **Apple Silicon (ARM64)**
   - Rosetta 2 peut être nécessaire pour certaines dépendances x86_64
   - Vérifier que toutes les dépendances ont des binaries ARM64 natifs
   - Variable CMake : `CMAKE_OSX_ARCHITECTURES=arm64`

2. **Vulkan SDK**
   - Installation manuelle requise (LunarG)
   - Vérifier que `VULKAN_SDK` est défini : `echo $VULKAN_SDK`
   - Si vide, exporter : `export VULKAN_SDK="/Users/$USER/VulkanSDK/1.x.xxx.x/macOS"`

3. **OpenAL**
   - Fourni par le système (AudioToolbox framework)
   - Pas besoin d'installation externe normalement

4. **Clang vs GCC**
   - macOS utilise Apple Clang par défaut
   - Compatibilité C++20 : Clang 17.0+ requis (vérifié : OK)

### Linux Spécifique

1. **nproc disponible**
   - Fait partie de GNU coreutils (toujours installé)
   - Pas de fallback nécessaire

2. **Vulkan SDK**
   - Via package manager : `vulkan-sdk`, `libvulkan-dev`
   - Pas de variable d'environnement nécessaire

3. **Dépendances**
   - Toutes via apt/yum/pacman
   - Liste dans README.md

## 🧪 Tests de Validation

### Test 1: Build Basique
```bash
# Sur macOS ET Linux
cd .claude-build-debug
cmake ..
cmake --build . --parallel
```

**Résultat attendu** : Build réussit sans spécifier le nombre de CPU

### Test 2: Build avec Tests
```bash
# Sur macOS ET Linux
/build-test
```

**Résultat attendu** : Build + tests passent sur les deux plateformes

### Test 3: Clean Rebuild
```bash
# Sur macOS ET Linux
/clean-rebuild
```

**Résultat attendu** : Rebuild complet réussit

## 📊 Matrice de Compatibilité

| Commande | Linux x86_64 | macOS ARM64 | macOS x86_64 | Statut |
|----------|--------------|-------------|--------------|--------|
| `/build-test` | ✅ | ✅ | ? | OK |
| `/quick-test` | ✅ | ? | ? | À tester |
| `/full-test` | ✅ | ? | ? | À tester |
| `/build-only` | ✅ | ? | ? | À tester |
| `/build-release` | ✅ | ? | ? | À tester |
| `/clean-rebuild` | ✅ | ? | ? | À tester |

✅ = Testé et fonctionnel
? = Pas encore testé
❌ = Problème détecté

## 🔄 Changelog

### 2025-11-22
- Création initiale de la spec
- Identification des différences plateforme
- Recommandation : `cmake --build . --parallel` (sans argument)
- Build test réussi sur macOS ARM64 avec `.claude-build-debug/`

## 📚 Références

- CMake documentation: https://cmake.org/cmake/help/latest/manual/cmake.1.html#build-tool-mode
- Emeraude Engine AGENTS.md: Build instructions multi-plateforme
- Emeraude Engine README.md: Dépendances par plateforme

---

**Prochaine étape** : Implémenter Option 3 dans toutes les commandes de build et tester sur macOS ARM64 + Linux.
