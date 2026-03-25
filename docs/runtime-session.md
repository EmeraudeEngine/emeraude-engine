# Engine Runtime Session

Guide pour interagir avec une application Emeraude-Engine en cours d'exécution via la console TCP. Ce document couvre le protocole de connexion, les commandes disponibles, le cycle de vie d'une session et l'arrêt propre.

## 1. Console TCP (RemoteListener)

Le moteur expose une console de commandes via **TCP port 7777** (configurable via le setting `Core/Console/RemoteListenerPort`). Toute application construite sur Emeraude-Engine hérite de cette interface.

### Connexion

```bash
# Linux — commande unique
echo "<commande>" | nc -q 2 localhost 7777

# macOS — commande unique (BSD netcat, -q non supporté)
echo "<commande>" | nc -w 2 localhost 7777

# Commandes multiples séquentielles
(
echo "commande1"
sleep 1
echo "commande2"
) | nc localhost 7777

# Python (cross-platform, recommandé)
python3 tools/remote-console.py "<commande>"
```

Les réponses sont en texte brut ou JSON, sans codes ANSI ni préfixes de log. Chaque réponse est envoyée uniquement au client demandeur.

### Disponibilité

La console est prête dès que le log affiche :
```
[Info][RemoteListener] Starting ASIO AI Remote Console on port 7777.
```

## 2. Découverte des services

Le moteur utilise une hiérarchie d'objets adressables par chemin (ex: `Core.RendererService.screenshot()`).

```bash
# Lister les objets racine
echo "listObjects" | nc -q 1 localhost 7777

# Lister les sous-services d'un objet
echo "Core.lsobj()" | nc -q 1 localhost 7777

# Lister les commandes disponibles sur un service
echo "Core.RendererService.lsfunc()" | nc -q 1 localhost 7777
```

### Hiérarchie des services

```
Core
├── ArgumentsService        (getJson, get, print)
├── AudioManagerService
│   └── TrackMixerService   (play, pause, stop, volume, playlist, etc.)
├── ConsoleControllerService
│   └── RemoteListener      (TCP 7777)
├── FileSystemService       (getJson, get, print)
├── RendererService         (screenshot, getStatus)
├── ResourcesManagerService (listContainers, listResources)
├── SceneManagerService     (createScene, setGround, setBackground, addMesh, etc.)
├── SettingsService         (getJson, set, save, print)
└── WindowService           (resize, getState)
```

## 3. Référence des commandes

### Rendu

| Commande | Description |
|----------|-------------|
| `Core.RendererService.screenshot()` | Capture le framebuffer en PNG (chemin retourné dans la réponse) |
| `Core.RendererService.getStatus()` | FPS, frame time, résolution |

### Fenêtre

| Commande | Description |
|----------|-------------|
| `Core.WindowService.resize(w, h)` | Redimensionner la fenêtre |
| `Core.WindowService.getState()` | État courant (JSON) |

### Ressources

| Commande | Description |
|----------|-------------|
| `Core.ResourcesManagerService.listContainers()` | JSON des conteneurs (types de ressources) |
| `Core.ResourcesManagerService.listResources(containerNameOrId)` | JSON des noms de ressources disponibles |

### Scènes

| Commande | Description |
|----------|-------------|
| `Core.SceneManagerService.createScene(name, boundary, camNode, x, y, z [, skybox [, ground]])` | Créer une scène complète |
| `Core.SceneManagerService.deleteScene(name)` | Supprimer une scène |
| `Core.SceneManagerService.enableScene(name)` | Activer une scène |
| `Core.SceneManagerService.getActiveSceneName()` | Nom de la scène active |
| `Core.SceneManagerService.getSceneInfo()` | Résumé de la scène active |
| `Core.SceneManagerService.listScenes()` | Lister toutes les scènes |
| `Core.SceneManagerService.setGround([material])` | Ajouter/remplacer le sol (`default` = gris basique) |
| `Core.SceneManagerService.setBackground(skyboxName)` | Changer le skybox |
| `Core.SceneManagerService.addMesh(meshRes, entityName, x, y, z [, scale])` | Placer un mesh 3D |
| `Core.SceneManagerService.createNode(name [, x, y, z])` | Créer un node |
| `Core.SceneManagerService.destroyNode(name)` | Supprimer un node |
| `Core.SceneManagerService.setNodePosition(name, x, y, z)` | Déplacer un node |
| `Core.SceneManagerService.setNodeLookAt(name, x, y, z)` | Orienter un node vers un point |
| `Core.SceneManagerService.getNode(name)` | Inspecter un node (JSON) |
| `Core.SceneManagerService.listNodes()` | Lister les nodes (cibler la scène d'abord) |
| `Core.SceneManagerService.listStaticEntities()` | Lister les entités statiques |
| `Core.SceneManagerService.attachCamera(node, camName)` | Attacher la caméra primaire |
| `Core.SceneManagerService.attachMicrophone(node, micName)` | Attacher le microphone primaire |
| `Core.SceneManagerService.targetActiveScene()` | Cibler la scène active pour inspection |
| `Core.SceneManagerService.targetScene(name)` | Cibler une scène nommée |
| `Core.SceneManagerService.targetNode(name)` | Cibler un node |
| `Core.SceneManagerService.moveNodeTo(x, y, z)` | Déplacer le node ciblé |
| *(entrée JSON)* | Envoyer `{...}` pour créer une scène depuis une définition JSON |

### Audio

| Commande | Description |
|----------|-------------|
| `Core.AudioManagerService.TrackMixerService.play([song])` | Lecture / reprise |
| `Core.AudioManagerService.TrackMixerService.pause()` | Pause |
| `Core.AudioManagerService.TrackMixerService.stop()` | Arrêt |
| `Core.AudioManagerService.TrackMixerService.volume(0-100)` | Volume |
| `Core.AudioManagerService.TrackMixerService.next()` / `previous()` | Navigation playlist |
| `Core.AudioManagerService.TrackMixerService.playlist([clear\|play,N\|add,name])` | Gestion playlist |
| `Core.AudioManagerService.TrackMixerService.seek(seconds)` | Position de lecture |
| `Core.AudioManagerService.TrackMixerService.shuffle(on/off)` | Mode aléatoire |
| `Core.AudioManagerService.TrackMixerService.loop(on/off)` | Mode boucle |
| `Core.AudioManagerService.TrackMixerService.crossfade(on/off)` | Fondu enchaîné |
| `Core.AudioManagerService.TrackMixerService.status()` | État complet du mixer |

### Configuration

| Commande | Description |
|----------|-------------|
| `Core.SettingsService.getJson()` | Tous les settings (JSON) |
| `Core.SettingsService.set(key, value)` | Modifier un setting |
| `Core.SettingsService.save()` | Sauvegarder sur disque |
| `Core.SettingsService.print()` | Dump texte |

### Système de fichiers et arguments

| Commande | Description |
|----------|-------------|
| `Core.FileSystemService.getJson()` | Tous les chemins (JSON) |
| `Core.FileSystemService.get(name)` | Chemin spécifique |
| `Core.ArgumentsService.getJson()` | Tous les arguments de lancement (JSON) |

### Commandes intégrées

| Commande | Description |
|----------|-------------|
| `quit` / `exit` / `shutdown` | Arrêt gracieux |
| `hardExit` | Arrêt immédiat (pas de sauvegarde) |
| `help` / `lsfunc()` | Lister les commandes |
| `listObjects` / `lsobj()` | Lister les services |

## 4. Cycle de vie d'une session

```
Lancement de l'application
        │
        ▼
Initialisation Vulkan, chargement des ressources
        │
        ▼
RemoteListener prêt (port 7777)  ◄── la console TCP est utilisable à partir d'ici
        │
        ▼
┌─── Boucle de session ───┐
│                          │
│  Envoyer des commandes   │
│  Prendre des screenshots │
│  Manipuler la scène      │
│  Inspecter l'état         │
│                          │
└──────────────────────────┘
        │
        ▼
echo "quit" | nc -q 1 localhost 7777
        │
        ▼
Sauvegarde settings, libération Vulkan, fermeture TCP
        │
        ▼
Processus terminé
```

## 5. Arrêt propre

```bash
# Fermeture gracieuse (sauvegarde les settings si SavePropertiesAtExit est activé)
echo "quit" | nc -q 1 localhost 7777
```

La commande `quit` garantit :
- La libération propre des ressources Vulkan (devices, swapchain, pipelines)
- La sauvegarde des settings si configuré
- L'arrêt des sous-systèmes (audio, réseau, etc.)
- La fermeture du RemoteListener TCP

**Ne jamais utiliser `kill -9` directement** sauf en dernier recours. Cela provoque des fuites de ressources GPU et peut laisser le port TCP occupé.

## 6. Dépannage

| Problème | Cause probable | Solution |
|----------|----------------|----------|
| `Failed to bind to port 7777` | Instance précédente encore active | Tuer le processus, attendre 2s, relancer |
| Pas de réponse TCP | Application pas encore initialisée | Attendre le log `RemoteListener` avant d'envoyer |
| Screenshot image noire | Scène pas encore rendue | Attendre 1-2 frames supplémentaires |
| Ressource invisible après `addMesh` | Chargement async en cours | Attendre 1-2s, reprendre un screenshot |
| Commande non reconnue | Mauvais chemin de service | Utiliser `lsobj()` et `lsfunc()` pour découvrir |