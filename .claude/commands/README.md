# Emeraude Engine - Claude Code Slash Commands

Ce répertoire contient des commandes personnalisées pour faciliter le développement avec Claude Code sur Emeraude Engine.

## Commandes Disponibles

### 🔨 Build & Test

#### `/build-test [filter]`
Build Debug + run tests (optionnel: filtrer par sous-système ou test).

**Exemples:**
```
/build-test           # Tous les tests
/build-test Physics   # Tests Physics uniquement
/build-test Vector    # Tests contenant "Vector"
```

#### `/quick-test <filter>`
Quick incremental build + filtered tests (rapide pour itération active).

**Exemples:**
```
/quick-test Physics
/quick-test Collision
```

#### `/full-test`
Full rebuild Debug + tous les tests (vérification complète avant commit).

**Usage:**
```
/full-test
```

#### `/build-release`
Build Release library (optimisé, sans tests).

**Usage:**
```
/build-release
```

#### `/build-only`
Build Debug sans lancer les tests (vérification compilation rapide).

**Usage:**
```
/build-only
```

#### `/clean-rebuild [filter]`
Clean rebuild complet Debug (supprime tout + reconfigure + build + tests).

**Exemples:**
```
/clean-rebuild         # Clean + rebuild + tous tests
/clean-rebuild Physics # Clean + rebuild + tests Physics
```

**Build directories utilisés:**
- `.claude-build-debug/` - Debug avec tests
- `.claude-build-release/` - Release sans tests

### 📚 Documentation & Navigation

#### `/doc-system [system-name]`
Affiche rapidement le AGENTS.md d'un sous-système avec résumé des points critiques.

**Exemples:**
```
/doc-system physics
/doc-system graphics
/doc-system
```

#### `/show-architecture [subsystem]`
Affiche un diagramme ASCII de l'architecture du moteur ou d'un sous-système.

**Exemples:**
```
/show-architecture           # Architecture complète
/show-architecture graphics  # Détails Graphics
```

### 🔍 Recherche & Analyse

#### `/find-usage [concept]`
Cherche où un concept/classe est utilisé dans le codebase.

**Exemples:**
```
/find-usage CartesianFrame
/find-usage Y-down
/find-usage ResourceTrait
```

#### `/check-references`
Vérifie que toutes les références @docs/ et @src/ dans les AGENTS.md sont valides.

**Usage:**
```
/check-references
```

### ✅ Validation & Conventions

#### `/check-conventions`
Vérifie le respect des conventions critiques du moteur:
- Y-down coordinate system
- Fail-safe Resources
- Libs isolation
- Vulkan abstraction
- Platform isolation

**Usage:**
```
/check-conventions
```

#### `/verify-y-down [file]`
Scanne fichier(s) pour détecter conversions Y suspectes ou valeurs incorrectes.

**Exemples:**
```
/verify-y-down src/Physics/Manager.cpp
/verify-y-down                          # Scan Physics/, Graphics/, Audio/, Scenes/
```

### 🛠️ Génération de Code

#### `/add-resource-type [name]`
Génère un template complet pour nouveau type de resource avec fail-safe.

**Exemple:**
```
/add-resource-type MyResource
```

Génère:
- Header et implementation avec ResourceTrait
- Neutral resource (mandatory)
- File/JSON loading
- Dependency management
- Test template

## Structure des Commandes

Chaque fichier `.md` dans ce répertoire définit une slash command:
- **Frontmatter:** Description de la commande
- **Contenu:** Instructions détaillées pour Claude Code

## Ajouter une Nouvelle Commande

1. Créer `commands/my-command.md`
2. Ajouter frontmatter:
   ```yaml
   ---
   description: Description courte de la commande
   ---
   ```
3. Écrire les instructions pour Claude Code
4. Mettre à jour ce README

## Conventions d'Écriture

- **Tâches claires:** Décrire précisément ce que Claude doit faire
- **Exemples:** Inclure des exemples d'usage
- **Format de sortie:** Spécifier le format attendu
- **Tools à utiliser:** Mentionner Read, Grep, Write selon besoin

## Notes

- Ces commandes sont spécifiques à Emeraude Engine
- Elles respectent l'architecture et conventions du moteur
- Mise à jour régulière recommandée avec évolution du projet
