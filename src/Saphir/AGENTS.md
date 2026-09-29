# Saphir Shader System


> [!CRITICAL]
> **Before modifying ANY generator or cache code, READ [`docs/pipeline-caching-system.md`](../../docs/pipeline-caching-system.md) FIRST!**
>
> Key rule: `computeProgramCacheKey()` MUST include `renderPassHandle` as its first hash component.
> Forgetting this causes Vulkan validation errors: "sample count mismatch", "format mismatch".

Context for developing the Emeraude Engine automatic shader generation system.


## Where the content is (router)

> ⚠️ This file is a ROUTER since 2026-09-27: its sections were moved VERBATIM to
> `docs/subsystems/saphir/`, one file per topic (a very large section is a folder, one file per
> sub-topic). **Read only the file(s) matching the task** — never the whole folder. A reference
> elsewhere to "`src/Saphir/AGENTS.md` § <Section>" means the row of that title below.

| Section | File | Size |
|---|---|---|
| Module Overview | [`docs/subsystems/saphir/01-module-overview.md`](../../docs/subsystems/saphir/01-module-overview.md) | 1 KB |
| Saphir-Specific Rules | [`docs/subsystems/saphir/02-saphir-specific-rules.md`](../../docs/subsystems/saphir/02-saphir-specific-rules.md) | 4 KB |
| Development Commands | [`docs/subsystems/saphir/03-development-commands.md`](../../docs/subsystems/saphir/03-development-commands.md) | 1 KB |
| Important Files | [`docs/subsystems/saphir/04-important-files.md`](../../docs/subsystems/saphir/04-important-files.md) | 1 KB |
| The per-vertex stage contract — `AbstractVertexStage` (Sep 2026) | [`docs/subsystems/saphir/05-the-per-vertex-stage-contract-abstractvertexstage.md`](../../docs/subsystems/saphir/05-the-per-vertex-stage-contract-abstractvertexstage.md) | 2 KB |
| ShaderManager Include Contract (compile-time) | [`docs/subsystems/saphir/06-shadermanager-include-contract.md`](../../docs/subsystems/saphir/06-shadermanager-include-contract.md) | 2 KB |
| Quality Setting Architecture | [`docs/subsystems/saphir/07-quality-setting-architecture.md`](../../docs/subsystems/saphir/07-quality-setting-architecture.md) | 4 KB |
| Development Patterns | [`docs/subsystems/saphir/08-development-patterns.md`](../../docs/subsystems/saphir/08-development-patterns.md) | 1 KB |
| Fresnel Effect Generation (Reflection + Refraction) | [`docs/subsystems/saphir/09-fresnel-effect-generation.md`](../../docs/subsystems/saphir/09-fresnel-effect-generation.md) | 2 KB |
| IBL Ambient Pass (Jul 2026, IBL lot 3) | [`docs/subsystems/saphir/10-ibl-ambient-pass.md`](../../docs/subsystems/saphir/10-ibl-ambient-pass.md) | 3 KB |
| A surface variable handed to `LightGenerator` must be a NAME, never an expression | [`docs/subsystems/saphir/11-a-surface-variable-handed-to-lightgenerator-must-be-a-name-n.md`](../../docs/subsystems/saphir/11-a-surface-variable-handed-to-lightgenerator-must-be-a-name-n.md) | 1 KB |
| Legacy (Blinn-Phong) Specular — Energy Normalisation (Jul 2026) | [`docs/subsystems/saphir/12-legacy-specular-energy-normalisation.md`](../../docs/subsystems/saphir/12-legacy-specular-energy-normalisation.md) | 3 KB |
| PBR Advanced Material Features | [`docs/subsystems/saphir/13-pbr-advanced-material-features.md`](../../docs/subsystems/saphir/13-pbr-advanced-material-features.md) | 10 KB |
| Color Projection Code Generation | [`docs/subsystems/saphir/14-color-projection-code-generation.md`](../../docs/subsystems/saphir/14-color-projection-code-generation.md) | 27 KB |
| SSBO Memory Qualifiers | [`docs/subsystems/saphir/15-ssbo-memory-qualifiers.md`](../../docs/subsystems/saphir/15-ssbo-memory-qualifiers.md) | 2 KB |
| MDI Shader Generation | [`docs/subsystems/saphir/16-mdi-shader-generation.md`](../../docs/subsystems/saphir/16-mdi-shader-generation.md) | 3 KB |
| Critical Points | [`docs/subsystems/saphir/17-critical-points.md`](../../docs/subsystems/saphir/17-critical-points.md) | 1 KB |
| Descriptor set binding contract (Aug 2026) | [`docs/subsystems/saphir/18-descriptor-set-binding-contract.md`](../../docs/subsystems/saphir/18-descriptor-set-binding-contract.md) | 3 KB |
| Heightfield surface — a vertex stage that BUILDS the surface (Sep 2026) | [`docs/subsystems/saphir/19-heightfield-surface-a-vertex-stage-that-builds-the-surface.md`](../../docs/subsystems/saphir/19-heightfield-surface-a-vertex-stage-that-builds-the-surface.md) | 4 KB |
| Task and mesh stages (optional, Sep 2026) | [`docs/subsystems/saphir/20-task-and-mesh-stages.md`](../../docs/subsystems/saphir/20-task-and-mesh-stages.md) | 3 KB |
| The mesh-shading surface (Sep 2026) | [`docs/subsystems/saphir/21-the-mesh-shading-surface.md`](../../docs/subsystems/saphir/21-the-mesh-shading-surface.md) | 3 KB |
| Cubemap Rendering Mode (Multiview) | [`docs/subsystems/saphir/22-cubemap-rendering-mode.md`](../../docs/subsystems/saphir/22-cubemap-rendering-mode.md) | 15 KB |
| Shadow Map Code Generation | [`docs/subsystems/saphir/23-shadow-map-code-generation.md`](../../docs/subsystems/saphir/23-shadow-map-code-generation.md) | 2 KB |
| Detailed Documentation | [`docs/subsystems/saphir/24-detailed-documentation.md`](../../docs/subsystems/saphir/24-detailed-documentation.md) | 1 KB |
| clang-tidy — the six warnings that are LEFT ON PURPOSE | [`docs/subsystems/saphir/25-clang-tidy-the-six-warnings-that-are-left-on-purpose.md`](../../docs/subsystems/saphir/25-clang-tidy-the-six-warnings-that-are-left-on-purpose.md) | 2 KB |
| The three shader-cache stages — what each one actually caches (audited Aug 2026) | [`docs/subsystems/saphir/26-the-three-shader-cache-stages-what-each-one-actually-caches.md`](../../docs/subsystems/saphir/26-the-three-shader-cache-stages-what-each-one-actually-caches.md) | 9 KB |
| The beam ribbon — a vertex stage that BUILDS a beam; `prepareVertexStage()` runs before the velocity (Sep 2026) | [`docs/subsystems/saphir/27-the-beam-ribbon-a-vertex-stage-that-builds-a-beam.md`](../../docs/subsystems/saphir/27-the-beam-ribbon-a-vertex-stage-that-builds-a-beam.md) | 2 KB |
