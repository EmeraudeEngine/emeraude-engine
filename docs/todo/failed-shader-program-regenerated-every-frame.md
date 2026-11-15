---
id: failed-shader-program-regenerated-every-frame
title: A shader program that fails to compile is regenerated and recompiled every frame — the log floods, the failure hook fires every frame
status: open
priority: high
scope: Graphics/PostProcessor (executeDirectPostProcessEffects), Graphics/Renderer (the program cache), Saphir/ShaderManager, Saphir/Generator
opened: 2026-11-15
tags: [defect, measured, shaders, log]
---

# A shader program that fails to compile is regenerated and recompiled every frame — the log floods, the failure hook fires every frame

## Why

`PostProcessor::executeDirectPostProcessEffects()` asks its generator for the shader program at every frame and relies
on the renderer's program cache for the deduplication ("same camera + same effects = cache hit"). The cache holds
SUCCESSES only: when the program fails, nothing remembers it, and the next frame generates the GLSL again, hands it to
the compiler again, fails again, writes the whole annotated source to the log again and notifies
`ShaderCompilationFailed` again.

Measured 2026-11-15 (Linux, RTX 3070 Ti, clang Release + ASan with the fault injection, the testbed's `default`,
`--inject-shader-compilation-failure=PostProcessingFragmentShader` on an empty cache directory, `--disable-cef`, three
runs of about 18 s after the start):

| | With the failing shader | Control (no failure) |
|---|---|---|
| Erroneous-GLSL blocks in the log | 3100 to 3182 (about 170 per second: one per frame) | 0 |
| `[Error]` lines | 15 500 to 15 825 | 0 |
| Journal size | 21.3 MB | 6.9 MB |
| `Core::onCoreShaderCompilationFailed()` calls | one per block | 0 |

That is about 0.8 MB of journal per second (close to 3 GB per hour) plus the same on the error stream, a GLSL
preprocess per frame on the rendering thread, and an application hook called 170 times a second — an application that
opens its interface from that hook (the testbed opens its shader debugger and its menu) does it at that rate.
The run itself holds: the console answers, the shutdown is clean (exit code 0, 0 VUID, no AddressSanitizer error, the
failures list of `Core` never full).
With a web interface (GCC Release configured with the fault injection, CEF, the same switch, 18 s): 3101 failures, 3101
calls of the testbed's hook — each one asks for its shader debugger page and opens its menu — so the menu cannot be
closed while the shader fails (`Stage.closeMenu()` answers "closed", the next state reads `menuOpen: true`); the main
thread still answers the console in 0.02 s and the shutdown is clean (exit 0 in 0.4 s, 0 VUID).

Only the post-processing program was measured. By reading, any generator whose caller asks again at each frame behaves
the same (the scene's renderable programs are asked from `getReadyForRender()`: to be measured with another injected
name, e.g. `RenderableInstanceSimplePassFragmentShader`).

## What remains

- Decide where a failure is remembered (owner): (a) the shader manager keeps the hash of a source that failed and
  refuses it at once, without recompiling or logging again; (b) the renderer's program cache stores a "failed" entry for
  the program's key; (c) each caller disables what it could not build (the post-processor drops the effect list until
  it changes). And what clears it: a changed source (a live shader edit must be able to retry), a device change.
- Whatever the choice: one log block and one hook call per distinct failing source, and the measure above again
  (blocks, journal size, hook calls) on the post-processing program and on a scene program.

## ⚠️ Traps

- The injection needs an EMPTY `--cache-directory`: a shader served by the binary cache is never compiled.
- CEF does not start in the ASan tree (SIGSEGV at the first browser subprocess): the hook's effect on a web interface is
  measured in a Release tree configured with `-DEMERAUDE_FAULT_INJECTION=On` and no sanitizer.

## References

- `docs/subsystems/source-tree/08-startup-fault-injection.md` (`--inject-shader-compilation-failure`).
- `docs/subsystems/source-tree/02-main-components.md` (`onCoreShaderCompilationFailed()`: main thread since 2026-11-15).
