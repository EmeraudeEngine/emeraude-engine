# Text-to-Motion — Kimodo evaluation

Feasibility study and measurements for [kimodo.cpp](https://github.com/localai-org/kimodo.cpp), a
C++23/GGML port of NVIDIA Toronto AI Lab's [Kimodo](https://github.com/nv-tlabs/kimodo) kinematic
motion diffusion model: **a sentence in, a skeletal animation out**.

Evaluated 2026-08-30/31 on an RTX 3070 Ti (8 GB). Everything below is measured on this machine
unless stated otherwise.

**Owner decision**: integration form **B — dev-time inside the engine**. Not shipped to end users,
not a runtime feature. Tracked in [`todo/kimodo-text-to-motion-integration.md`](todo/kimodo-text-to-motion-integration.md),
blocked on the retargeter.

## 1. What it produces

```
kimodo_generate(prompt UTF-8)          // loads both models
kimodo_generate_embedding(float[4096]) // motion denoiser only
      ↓
local rotations  [T, J, 4]  XYZW quaternions
root translations[T, 3]     metres, ground at Y = 0
                            30 FPS, 100 DDIM steps
```

The C API is `include/kimodo/kimodo_capi.h`. Returned pointers are **borrowed**, valid until the
motion is freed. Joint count is queried from the result, never hardcoded.

⚠️ **The split between the two entry points is the architecturally interesting part.** The text
encoder is 15 GB; the embedding it produces is **16 KB and reusable forever**. A shipped product
would bake embeddings as assets and carry only the 0.3 Md denoiser.

## 2. Models and licences

| | file | size | licence | commercial |
|---|---|---|---|---|
| text encoder | `generated/llm2vec-text-bundle/` | **15.18 GB** | — | — |
| motion, SOMA RP v1.1 | `models/kimodo-soma-rp-v1.1-f32.gguf` | **1.13 GB** | NVIDIA Open Model | **yes** |
| motion, G1 RP v1 | `models/kimodo-g1-rp-v1-f32.gguf` | ~1 GB | NVIDIA Open Model | yes |
| motion, SMPL-X RP v1 | — | — | NVIDIA **R&D** | **no** |

The kimodo.cpp port itself is **Apache-2.0**, one-way compatible with the engine's LGPLv3.

- **SOMA (30 joints)** is the only usable human skeleton: commercially licensed and published.
- **G1 (34 joints)** is a Unitree *robot* rig — non-human proportions, worse retargeting target.
- **SMPL-X (22 joints)** is research-only and **deliberately absent** from the published weights.

Weights are on the Hugging Face org **`LocalAI-io`** (not `localai-org`, which is the GitHub org —
searching the wrong one returns nothing). Fetched by `scripts/download_gguf_weights.sh`, which
verifies SHA-256 against a manifest.

⚠️ The encoder ships as **32 files of 441 MB, one per layer**, plus a 1.05 GB embedding table. That
layout is what makes `KIMODO_TEXT_LAYER_CHUNK` (8 layers at a time) meaningful.

## 3. Measured cost

| | measured |
|---|---|
| generation, 120 frames, 100 DDIM steps | **18.4 s** wall clock, model load included |
| VRAM peak, Vulkan backend | **2779 MiB / 8192** on an RTX 3070 Ti |
| host RSS peak | 391 MB — weights are mmapped and streamed |
| subsequent generations | 14.5 s |

⚠️ **The 15 GB encoder fits comfortably on an 8 GB card** thanks to the per-layer streaming. The
naive reading — "an 8 Md parameter model needs 16 GB of VRAM" — is wrong here and led to a
premature "CPU mandatory" conclusion during this evaluation.

The encoder is an **encoder**: one forward pass and a mean pooling, not autoregressive generation.
Its cost is dominated by reading 15 GB from disk, not by compute.

## 4. Motion quality, measured

On a "walks forward at a steady, natural pace" prompt, 120 frames, seed 42:

| | |
|---|---|
| distance travelled | 6.41 m in 4.00 s → **1.60 m/s** (brisk; a natural walk is 1.2–1.4) |
| stance phases detected | 8 (4 per foot), 433–500 ms each — physiologically correct |
| **foot drift per stance** | **1.3 to 1.7 cm** on a ~160 cm stride, i.e. **~1 %** |
| ground penetration | none, toes never below +1.8 cm |

The raw motion is **effectively foot-locked**. This matters because the C API does **not** expose
the foot-contact labels the upstream model produces — the one signal that would make a footlock
pass cheap. It turns out to be barely needed.

⚠️ That measurement describes the **source**. Retargeting reintroduces sliding if the root
translation is not scaled by the leg-length ratio — see [`animation-retargeting.md`](animation-retargeting.md) § 3.

## 5. Limits that survived the evaluation

- **Maturity.** 23 commits at the time of evaluation. Two build defects found in one evening, both
  in the CPU-only path (see § 6).
- **Hands are coarse.** SOMA-30 carries `HandThumbEnd` and `HandMiddleEnd` only — two bones per
  hand, enough for a rough open/close, nothing more. The 77-joint SOMA expansion is **not
  implemented** in the port. For a sword-and-shield character this decides whether the model serves
  combat or only secondary body motion.
- **No foot-contact output** (§ 4).
- **Prompt following is loose on intensity** — "steady, natural pace" produced 1.60 m/s.

## 6. ⚠️ Upstream build defects (found 2026-08-30, patched locally)

Both break `KIMODO_ENABLE_VULKAN=OFF`, i.e. the CPU-only build has evidently never been compiled
upstream:

1. **`src/llm_text_encoder.cpp`** calls `ggml_backend_vk_get_device_count()` / `ggml_backend_vk_init()`
   and includes `<ggml-vulkan.h>` **without a guard**, while `src/ggml_weights.cpp` guards the same
   calls correctly with `#if defined(KIMODO_HAVE_GGML_VULKAN)`. Fixed by applying the same pattern.
2. **`CMakeLists.txt`** defines `KIMODO_HAVE_GGML_VULKAN=1` **unconditionally** on `kmd-encode`,
   `kmd-sample-fixture` and the parity targets. Worked around by building only the needed targets.

Building with Vulkan additionally needs `glslc` (Debian: `glslc`) and a `SPIRV-Headers` CMake
package. The latter is already produced by this workstation's `ext-deps-generator`; point CMake at
it with `-DCMAKE_PREFIX_PATH=<prefix> -DCMAKE_CXX_FLAGS=-I<prefix>/include` — the config file alone
does not propagate the include path to `ggml-vulkan`.

## 7. Where things are

The evaluation checkout sits at `dependencies/kimodo.cpp/` (17 GB with the weights). It is
**deliberately unversioned** — covered by the engine `.gitignore`'s `dependencies/*` rule, an
owner decision — and carries the local patch from § 6.

```
models/  generated/    the weights (already covered by the checkout's own .gitignore)
prompts/               the 8 evaluation prompts
out/<name>/            raw motion (local_rotations_xyzw.f32, root_positions.f32)
demo-output/<id>/      the Go demo's gallery, same file names
mannequins/            the produced glTF
```

```
tools/                 the retargeting prototype and its README
tools/0001-guard-vulkan-calls-in-the-text-encoder.patch
```

⚠️ **The tracked tree is kept pristine on upstream.** The Vulkan-guard fix of § 6 is NOT applied in
place — it sits beside the tools as a patch, to be applied before a CPU-only build (a Vulkan build
does not need it).

**Owner decision (2026-08-31): kimodo.cpp stays an unversioned directory** inside the engine's
`dependencies/`, a submodule of nobody, until a proper integration (a CMake option or otherwise) is
designed. It was briefly registered as a projet-alpha submodule, with the tools moved out to
projet-alpha, and **both moves were deliberately rolled back**.

⚠️⚠️ **Consequence: nothing here is backed up.** The weights are re-downloadable and the upstream
source is a clone, but the retargeting prototype, the patch and the generated motions exist on this
workstation only. Anyone re-cloning kimodo.cpp gets none of it.

Regenerate a motion:

```sh
./build/release/kmd-generate models/kimodo-soma-rp-v1.1-f32.gguf \
    generated/llm2vec-text-bundle prompts/walk.txt 120 100 42 out/walk
```

The Go demo (`go run ./demo -addr 127.0.0.1:8090`, needs Go ≥ 1.26) adds a browser viewer, a prompt
gallery and skeleton-only GLB export. It shells out to `kmd-generate` and serializes the GLB itself.

⚠️ A **skeleton-only** GLB has no `skins` array, so a glTF consumer classifies its channels as
**node** animation: the engine routes them to `Scenes::Component::NodeAnimation` and
`SkeletalAnimator` never sees them — and with no mesh, nothing is drawn either. Authoring a skinned
proxy (the mannequin tool) is what makes the motion land on the skeletal path.

## 8. What integration would actually cost

Roughly **85 % engine, 15 % Kimodo**. The expensive half is
[`animation-retargeting.md`](animation-retargeting.md), which is worth building on its own merits:
it unlocks any motion-capture library and any third-party glTF rig, whatever happens to Kimodo.

## Link Index

- [`animation-retargeting.md`](animation-retargeting.md) — the mathematics, the SOMA-30 reference and the traps
- [`todo/kimodo-text-to-motion-integration.md`](todo/kimodo-text-to-motion-integration.md) — the open work
- [`ai-runtime-control.md`](ai-runtime-control.md) § 3 — the viewer that displays the result
