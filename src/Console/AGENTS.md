# Console System

Context spécifique pour le développement du système de console de commandes d'Emeraude Engine.

## Vue d'ensemble du module

**Statut : EN DÉVELOPPEMENT (non pressant)** - Système de terminal/console pour piloter le moteur via commandes textuelles.

## Règles spécifiques à Console/

### Objectif
- **Terminal de commandes** : Interface textuelle pour contrôler le moteur
- **Parsing d'entrées** : Analyse de commandes saisies par l'utilisateur
- **Interface Controllable** : Système d'objets contrôlables par commandes

### Architecture prévue
- Console pour saisie et affichage
- Parser pour interpréter commandes
- Interface Controllable pour objets pilotables
- Système de commandes extensible

## Commandes de développement

```bash
# Tests console (quand disponibles)
ctest -R Console
./test --filter="*Console*"
```

## Fichiers importants

- `Controllable.hpp` - Interface pour objets contrôlables par console
- À documenter lors de la stabilisation du système

## Patterns de développement

À documenter lors de la stabilisation du système.

## Points d'attention

- **En développement** : Architecture susceptible de changer
- **Non pressant** : Priorité faible actuellement
- **Ne pas confondre** : Aucun lien avec AVConsole (Audio Video Console)

## Documentation détaillée

Documentation à créer une fois le système stabilisé.
