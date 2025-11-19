# Animations System - Development Context

Context spécifique pour le développement du système d'animations d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

**Statut : EN CHANTIER** - Système d'animations squelettales en cours de développement.

## 📋 Règles spécifiques à Animations/

### Périmètre du système
- **Animations squelettales** : Skinning, déformation de mesh par squelette
- **Interfaces d'animation** : Possibilité d'animer des valeurs via interfaces
- **PAS d'animations de transform** : Utiliser scene graph pour animer position/rotation/scale

### Architecture (à définir)
Le système d'animations squelettales est actuellement en développement actif. L'architecture et les fonctionnalités seront documentées une fois stabilisées.

### Fonctionnalités prévues
- Squelettes et bones
- Skinning (déformation vertices par bones)
- Timeline et keyframes
- Blending d'animations squelettales
- Interfaces pour animation de valeurs personnalisées

## 🛠️ Commandes de développement

```bash
# Tests animations (quand disponibles)
ctest -R Animations
./test --filter="*Animation*"
```

## 🔗 Fichiers importants

À documenter lors de la stabilisation du système.

## ⚡ Patterns de développement

À documenter lors de la stabilisation du système.

## 🚨 Points d'attention

- **En développement actif** : Architecture susceptible de changer
- Consulter les développeurs avant modifications majeures

## 📚 Documentation détaillée

Documentation à créer une fois le système stabilisé.
