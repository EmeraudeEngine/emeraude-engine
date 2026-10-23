---
id: scene-world-one-shot-sound
title: A world one-shot sound — Scene::playSoundAt(position, sound, gain, pitch) that outlives its emitter
status: open
priority: unranked
scope: src/Scenes/Scene.* (with src/Audio/Manager.hpp's source pool)
opened: 2026-10-23
tags: [audio, scenes, api]
---

# A world one-shot sound — Scene::playSoundAt(position, sound, gain, pitch) that outlives its emitter

## Why

`SoundEmitter::playOneShot()` (2026-10-23, `docs/subsystems/audio/14-soundemitter-one-shot-voices.md`) layers a
sound on an entity, but the voice dies with its emitter. An entity destroyed at the moment of its sound (a grenade
that explodes, a projectile that hits) has to stay alive for the sound's tail: projet-alpha's `Explosion` keeps its
whole actor until its sound ends (`Explosion.cpp`, `m_duration = max(sprite, sound)`), `Grenade` / `SmokeGrenade`
drop their emitter pointer right after `play()`. UE5's `UGameplayStatics::PlaySoundAtLocation()` is the model.

Proposed with the one-shots (owner pick 2026-10-23: the mechanism in the engine, the actor helpers in projet-alpha);
left out of that lot because no example of it (the car, the sword) needed it.

## What remains

- A scene-owned, bounded set of voices (`SourceRequest`), positioned once, released in `Scene::processLogics()` when
  stopped, stopped and released at `Scene::disable()` / suspension and in the destructor (a `SourceRequest` held at the
  audio shutdown is a contract fault).
- Thread contract: logic thread (actors and the physics impacts run there), or a mutex if a console command drives it.
- A console adapter answer (how many world voices play) so an AI can verify it.
- First consumer: projet-alpha's actors destroyed with their sound (projet-alpha item `actors-sound-cue-migration`).
