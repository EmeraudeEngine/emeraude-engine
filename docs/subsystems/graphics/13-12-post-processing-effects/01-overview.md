## 12. Post-Processing Effects

### Overview

The engine provides a multi-pass post-processing pipeline via `PostProcessor`. Effects chain together: each effect's output becomes the next effect's input. All effects inherit from `PostProcessEffect`.

**Requirements contract** — The `PostProcessor` aggregates its chain's needs and exposes them as a formal contract:
- `requiresHDR()` / `requiresDepth()` / `requiresNormals()` / `requiresVelocity()`
- The Renderer queries these methods to decide scene target format, MRT attachments, etc.
- No manual toggle (the old `enableHDR()` has been removed). Requirements are inferred from the active effect chain.
- `requiresRayTracing()` effects are additionally gated per frame on
  `Renderer::isRayTracingReady()` (TLAS built AND RT descriptor sets live): during the async
  TLAS build of a scene's first frames — or in a scene with no RT geometry — the chain skips
  them and forwards the previous output. An RT effect's `execute()` can therefore assume a
  consumable TLAS; never record a trace pass behind a `if (rtDescSet != nullptr)` bind alone
  (drawing with set 0 unbound is what that guard silently allowed before Aug 2026 — see
  `docs/caution-points.md` § Vulkan Validation).
- `requiresJitter()` (temporal anti-aliasing) — when any **enabled** effect in the active stack
  declares it (⚠️ enabled, not resident — until 2026-09-13 a TAA switched off by selection left the
  projection jittering with nothing to resolve it, the whole frame shimmering on a parked camera:
  64 % of the brick cube's pixels moving on `light-and-shadow-debug`, measured),
  `Renderer::prepareFrameJitter()` advances a Halton (2,3) sub-pixel sequence and applies
  it to the MAIN view only, once per rendered frame, before the video-memory update. Implies
  `requiresVelocity()` in practice: a temporal effect without motion vectors smears.
  ⚠️ The jitter reaches the shaders through **per-draw push constants**, never through the
  view UBO — see § 16 Rule 4 for the data race that design cost, and
  `src/Saphir/AGENTS.md` § "TAA Sub-Pixel Jitter" for the three-site lockstep contract.
  `ViewMatricesInterface` exposes both forms and they are NOT interchangeable:
  `projectionMatrix(readStateIndex)` serves the **jittered** matrix (what the frame was
  rasterized with — post-process depth unprojection, CPU-computed MVP paths), while
  `unjitteredProjectionMatrix(readStateIndex)` serves the clean one and MUST be used by
  anything feeding a velocity clip position (the InstanceTransforms SSBO header, the pushed
  view-projection of the paths that jitter in the shader).

- `PushConstants::deltaTime` — duration of the previous RENDERED frame, in seconds, clamped to
  [1/1000, 1/15]. The chain's SINGLE source of truth for anything converting the per-frame
  velocity G-buffer into a physical duration: `MotionBlur` divides the camera's shutter speed by
  it to get the shutter angle (how many frames of motion the exposure covers). Effects must NOT
  measure time themselves — a chain where two effects disagree on the frame duration cannot be
  reasoned about. Zero on the direct (lens) chain, which has no temporal consumer.
- ⚠️ **Amplifying the velocity buffer requires a raw dead zone.** Its two clip positions come
  from differently-computed matrix products, so a static camera leaves ~1e-4 px of rounding
  noise. Any effect scaling velocity by more than 1 must reject that on the RAW per-frame
  magnitude, before its own factor — see `docs/caution-points.md` § "A velocity buffer has a
  floating-point noise floor".
