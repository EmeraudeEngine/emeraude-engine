# Tool (Utilities) - Development Context

Context spécifique pour le développement des outils utilitaires d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

**Statut : PEU UTILISÉ** - Utilitaires annexes utilisant la logique du moteur pour tâches spécifiques, lancés via arguments ligne de commande. Concept présent mais pas activement utilisé.

## 📋 Règles spécifiques à Tool/

### Concept: Utilitaires annexes
- **Pas l'application principale** : Outils pour tâches spécifiques, pas pour lancer le jeu/app
- **Utilise logique moteur** : Réutilise systèmes du moteur (Geometry, Vulkan, etc.)
- **Runtime via CLI** : Lancés par arguments en ligne de commande

### Outils disponibles (exemples)

**GeometryDataPrinter** :
- Affiche structure data d'une géométrie en format texte
- Inspection/debug de fichiers géométrie

**VulkanCapabilities** :
- Affiche capacités Vulkan du système
- Info GPU, extensions supportées, limites

### Invocation (syntaxe approximative)
```bash
# Exemple conceptuel (syntaxe exacte à définir)
./Emeraude --tool geometry-printer file.obj
./Emeraude --tool vulkan-capabilities
```

### Architecture
- Outils enregistrés avec nom unique
- Dispatch selon argument `--tool`
- Accès aux systèmes moteur (Resources, Graphics, etc.)

## 🛠️ Commandes de développement

```bash
# Lister outils disponibles (si implémenté)
./Emeraude --list-tools

# Lancer un outil
./Emeraude --tool <tool-name> [args...]
```

## 🔗 Fichiers importants

- GeometryDataPrinter - Inspection géométries
- VulkanCapabilities - Info capacités GPU
- À documenter lors de l'activation du système

## ⚡ Patterns de développement

### Ajout d'un nouvel outil (concept)
```cpp
// Créer classe outil
class MyTool {
public:
    static int run(const std::vector<std::string>& args) {
        // Utiliser systèmes moteur
        auto geometry = loadGeometry(args[0]);
        processGeometry(geometry);
        return 0;  // Success
    }
};

// Enregistrer
ToolRegistry::register("my-tool", &MyTool::run);
```

### Utilisation depuis main()
```cpp
int main(int argc, char* argv[]) {
    // Détecter mode outil
    if (hasArg("--tool")) {
        std::string toolName = getArg("--tool");
        return ToolRegistry::run(toolName, remainingArgs);
    }

    // Sinon lancer application normale
    return runApplication();
}
```

## 🚨 Points d'attention

- **Peu utilisé actuellement** : Concept présent mais pas activement développé
- **Syntaxe à définir** : Interface CLI exacte à standardiser
- **Accès systèmes moteur** : Outils peuvent utiliser Resources, Graphics, etc.
- **Pas pour production** : Outils de développement/debug uniquement
- **Exit propre** : Retourner code exit approprié (0 = succès)

## 📚 Documentation détaillée

À créer si le système Tool devient activement utilisé.

Systèmes utilisables par Tools:
→ **@src/Libs/AGENTS.md** - Bibliothèques fondamentales
→ **@src/Graphics/AGENTS.md** - Pour outils graphiques
→ **@src/Resources/AGENTS.md** - Chargement de resources
