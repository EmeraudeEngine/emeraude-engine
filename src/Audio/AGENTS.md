# Audio System - Development Context

Context spécifique pour le développement du système audio spatial 3D d'Emeraude Engine.

## 🎯 Vue d'ensemble du module

Système audio 3D basé sur OpenAL-Soft avec support complet des effets spatiaux, streaming de musique, et intégration transparente avec le scene graph via des components SoundEmitter.

## 📋 Règles spécifiques à Audio/

### Convention de coordonnées
- **Y-DOWN obligatoire** dans l'abstraction Audio
- Conversions internes vers OpenAL si nécessaire
- Cohérence totale avec le reste du moteur (Physics, Graphics, Scenes)

### Philosophie Sons vs Musiques
- **Sons** : Chargés entièrement en RAM, format MONO, pour effets sonores courts
- **Musiques** : Streaming depuis RAM vers OpenAL, format STÉRÉO supporté, pour pistes longues
- Formats supportés : Délégués à `libsndfile` (WAV, OGG, FLAC, etc.)

### Architecture Component
- **SoundEmitter** : Component attachable aux Entity/Nodes du scene graph
- Nombre d'émetteurs : Illimité au niveau scene graph
- **Pooling** : Les émetteurs requièrent des sources OpenAL depuis un pool lors de la lecture
- Positionnement 3D automatique via le scene graph

### Integration Resources
- **OBLIGATOIRE** : Utilisation du système Resources pour chargement
- `SoundResource` : Gestion du chargement des sons
- `MusicResource` : Gestion du chargement des musiques
- Pattern fail-safe : Ressources audio neutres en cas d'échec

## 🛠️ Commandes de développement

```bash
# Tests audio
ctest -R Audio
./test --filter="*Audio*"

# Debug audio
./Emeraude --debug-audio
./Emeraude --list-audio-devices
./Emeraude --audio-device="Device Name"
```

## 🔗 Fichiers importants

- `Manager.cpp/.hpp` - Gestionnaire principal (devices, activation, capture)
- `SoundResource.cpp/.hpp` - Chargement et gestion des sons (mono)
- `MusicResource.cpp/.hpp` - Chargement et gestion des musiques (stéréo + streaming)
- `Source.cpp/.hpp` - Abstraction des sources OpenAL (pool)
- `Speaker.cpp/.hpp` - Abstraction point d'écoute (non-OpenAL, cohérence API)
- `TrackMixer.cpp/.hpp` - Jukebox pour gestion de playlists
- `Ambiencer.cpp/.hpp` - Sons d'ambiance (piste unique + effets aléatoires)
- `Recorder.cpp/.hpp` - Enregistrement audio depuis microphone
- `@docs/coordinate-system.md` - Convention Y-down (CRITIQUE)

## ⚡ Patterns de développement

### Ajout d'un SoundEmitter à une entité
1. Créer le component SoundEmitter
2. L'attacher au Node du scene graph
3. Charger la SoundResource via le système Resources
4. Le positionnement 3D suit automatiquement le Node

### Configuration des effets audio
- Tous les effets OpenAL-Soft disponibles via système de filtres
- Effets 3D : Atténuation distance, Doppler, directivité
- Effets d'environnement : Réverbération, occlusion, etc.
- Application par filtres configurables

### Gestion du pooling de sources
- Pool de sources OpenAL géré par le Manager
- Requête automatique lors du `play()` d'un émetteur
- Libération automatique en fin de lecture
- Priorités gérables si pool saturé

### Sons d'ambiance avec Ambiencer
1. Configurer une piste de fond continue
2. Ajouter des effets sonores aléatoires (probabilités, intervalles)
3. Le système gère automatiquement le mix

## 🚨 Points d'attention

- **Device detection** : OpenAL-Soft latest peut être capricieux sur laptops
- **Format sons** : MONO obligatoire pour positionnement 3D spatial
- **Format musiques** : STÉRÉO supporté (pas de spatialisation)
- **Thread safety** : OpenAL gère le threading en interne
- **Coordinate system** : Toujours Y-down dans l'API Audio
- **Resource integration** : Jamais de chargement direct, passer par Resources
