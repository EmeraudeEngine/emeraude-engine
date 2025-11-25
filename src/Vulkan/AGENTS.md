# Vulkan System

Context spécifique pour le développement de la couche d'abstraction Vulkan d'Emeraude Engine.

## Vue d'ensemble du module

Couche d'abstraction Vulkan qui masque la complexité de l'API tout en offrant un contrôle précis pour les applications 3D modernes. JAMAIS d'appels directs à Vulkan depuis le code haut niveau.

## Règles spécifiques à Vulkan/

### Abstraction obligatoire
- **JAMAIS** d'appels directs aux fonctions Vulkan depuis `Graphics/` ou code client
- Utiliser les classes d'abstraction : `Device`, `Buffer`, `Image`, `Pipeline`, etc.
- Toutes les ressources Vulkan doivent être encapsulées

### Gestion mémoire GPU
- **VMA obligatoire** pour toutes les allocations mémoire GPU
- Utiliser `MemoryRegion` et `DeviceMemory` pour l'encapsulation
- RAII pour la gestion automatique des ressources Vulkan

### Synchronisation
- Gestion rigoureuse des fences et semaphores
- Éviter les deadlocks par un ordre strict d'acquisition
- `CommandBuffer` thread-safe avec pools dédiés

### Convention de coordonnées
- Matrices de projection configurées pour Y-down
- Viewport Vulkan Y-inversé géré automatiquement
- Pas de conversion dans les shaders

### Performance: std::span pour APIs de barrières

`CommandBuffer` utilise `std::span` pour les méthodes de synchronisation:

```cpp
void pipelineBarrier(std::span< const VkImageMemoryBarrier > barriers, ...);
void waitEvents(std::span< const VkEvent > events, ...);
```

**Avantages:**
- Accepte `StaticVector`, `std::vector`, `std::array` sans copie
- Zero allocation côté appelant avec `StaticVector`
- Rétro-compatible avec le code existant utilisant `std::vector`

## Fichiers importants

- `Device.cpp/.hpp` - Abstraction du device logique Vulkan
- `Buffer.cpp/.hpp` - Gestion des buffers avec VMA
- `Image.cpp/.hpp` - Gestion des textures et images
- `GraphicsPipeline.cpp/.hpp` - Pipelines de rendu
- `CommandBuffer.cpp/.hpp` - Enregistrement des commandes (utilise std::span)
- `TransferManager.cpp/.hpp` - Transferts CPU-GPU

## Patterns de développement

### Création d'une nouvelle ressource GPU
1. Hériter de `AbstractDeviceDependentObject`
2. Implémenter RAII avec destructeur approprié
3. Utiliser VMA pour les allocations mémoire
4. Ajouter les synchronisations nécessaires

### Ajout d'une nouvelle pipeline
1. Définir les descriptors et layouts
2. Configurer les états de rendu
3. Compiler et cacher les shaders SPIR-V
4. Intégrer avec le `LayoutManager`

### Transferts de données
1. Utiliser `TransferManager` pour les transferts async
2. Staging buffers automatiques pour les gros transferts
3. Synchronisation par fences pour la cohérence
4. Batching des petits transferts

## Points d'attention

- **Destruction ordonnée** : Détruire les ressources dans l'ordre inverse de création
- **Thread safety** : CommandPool par thread, CommandBuffer non partagés
- **Memory barriers** : Transitions d'état correctes pour les images
- **Validation layers** : Toujours actives en développement
- **Jamais d'appels directs** : Graphics, Resources, Saphir utilisent abstractions Vulkan
- **VMA obligatoire** : Toute allocation GPU via VMA, jamais vkAllocateMemory direct
- **Y-down setup** : Viewport et projection configurés pour Y-down moteur

## Documentation détaillée

Pour la plateforme Vulkan:
- Vulkan documentation officielle - Spécifications API complètes

Systèmes liés:
- @docs/coordinate-system.md - Configuration Y-down pour Vulkan
- @src/Graphics/AGENTS.md - Utilise abstractions Vulkan (Buffer, Image, Pipeline)
- @src/Saphir/AGENTS.md - Génère SPIR-V pour pipelines Vulkan
- @src/Resources/AGENTS.md - Upload GPU via TransferManager