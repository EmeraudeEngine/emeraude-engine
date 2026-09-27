## 12. Post-Processing Effects

### RTR followed, for a defect that was already live (Sep 2026)

> [!CAUTION]
> `RTR::TracePushConstants` was **148 bytes**, the only block in the engine over the floor — the
> engine printed it every time an RTR pipeline layout was built: *"A push constant range ends at
> 148 bytes, above the 128-byte Vulkan minimum guarantee ! This pipeline layout will fail to
> create on min-spec devices."* On a device exposing exactly 128, `PipelineLayout::create()`
> returned `false`, so **RTR was never created and the scene silently lost its reflections**.
> It now reads `TraceFrameUBOData` from **set 1, binding 5**.
>
> **The port kept the member list and its order UNCHANGED**, and that is the interesting part:
> in std140 a `vec3` has a base alignment of 16 and a size of 12, so each `vec3 + float` pair
> packs into 16 bytes exactly like the C++ `float[3] + float` it mirrors, and `vec4 ambientLight`
> lands on 128 by itself. Verified member by member against the SPIR-V the compiler emits:
>
> | Member | `offsetof` (C++) | `OpMemberDecorate … Offset` (SPIR-V) |
> |---|---|---|
> | `invRelativeViewProj` (named `invViewProj` until 2026-09-25) | 0 | 0 |
> | `invViewCol0` / `viewPosX` | 64 / 76 | 64 / 76 |
> | `invViewCol1` / `viewPosY` | 80 / 92 | 80 / 92 |
> | `invViewCol2` / `viewPosZ` | 96 / 108 | 96 / 108 |
> | `maxDistance` … `lightCount` | 112, 116, 120, 124 | 112, 116, 120, 124 |
> | `ambientLight` (vec4) | 128 | 128 |
> | `coneScale` | 144 | 144 |
>
> Because **not one line of the shader BODY changed**, the port cannot have altered the maths —
> the risk was entirely in the packing, and the packing is now pinned by 13 `static_assert`s in
> `RTR.hpp`. Keep them: a reordered member reads garbage with **no compile error**.
>
> ⚠️ **Cost: none measurable.** `vkCmdPushConstants` ran once per draw, not per pixel, and the
> values are uniform across the pass — both forms end up in scalar registers. Measured on
> `reflexion-debug --demo-options=0,6,0`, `RTREffect/trace` cumulative average over ~2500 frames:
> **0.521 ms before, 0.519 / 0.522 ms after** (two runs). Passes the port does not touch drifted
> more between runs than that (`ScenePass` 1.544 / 1.473 / 1.483). The trace is dominated by BVH
> traversal; a handful of scalar loads per wave does not register.
>
> ⚠️ **This bench cannot prove a render is unchanged by pixel diff**: `reflexion-debug` carries an
> ANIMATED dragon, so two runs of the SAME binary already differ on 59.3 % of pixels (mean 1.873).
> The pre/post delta came out at mean 1.150 — *below* that control. Always shoot the same-binary
> control before reading a pixel diff in this scene.
