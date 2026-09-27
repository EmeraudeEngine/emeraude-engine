## Known Issues

- **TBN debug rendering**: TBNSpaceRendering generator does not support skeletal animation — TBN lines may not appear or may appear at bind-pose positions.
- **MD5 normal map convention**: id Tech normal maps use OpenGL convention (Y+ up). The engine uses DirectX convention (Y+ down). Use `"FlipNormalMapY": true` in the material JSON normal component to flip the green channel at load time. See: `Pixmap::flipNormalMapY()`, `TextureResource::Abstract::enableFlipNormalMapY()`, `Material::Component::Texture` (JSON key `"FlipNormalMapY"`).

### SkeletalAnimator Runtime API

The `SkeletalAnimator` provides runtime clip management:
- `clipNames()` — Returns sorted list of all registered clip names
- `activeClipName()` — Returns the name of the currently playing clip
- `play(clipName, wrap)` — Starts a clip by name (default: `PlaybackWrap::Loop`)
- `stop()` / `pause()` / `resume()` — Playback control
- `setSpeed(float)` — Playback speed multiplier

⚠️⚠️ **`play()` takes the CLIP's own name (`clip->clip().name()`), never the RESOURCE name.** The
loaders prefix their resource keys — `"FBX:<stem>/Animation/<clip>"`, `"<prefix>/animation/<clip>"` —
and `addClip()` indexes on `clip->clip().name()`. Passing a resource name looks up a key that never
existed, `play()` returns **false**, and a caller ignoring that bool sees *nothing happen, silently*.
That was the whole reason the `asset-loader` demo's animation cycling did nothing (Aug 2026).
**Always read the return value.**

`Scenes::Component::NodeAnimation` exposes the SAME surface (`play`/`stop`/`isPlaying`/`clipNames`/
`activeClipName`/`setSpeed`) on purpose: a caller cycling an asset's animations must not have to know
which evaluator the asset happens to need — see `Builtin/AssetLoader.cpp::applyAnimation()`, which
drives whichever answers and warns when none does.

Code references: `Animations/SkeletalAnimator.hpp/.cpp`, `Scenes/Component/NodeAnimation.hpp/.cpp`
