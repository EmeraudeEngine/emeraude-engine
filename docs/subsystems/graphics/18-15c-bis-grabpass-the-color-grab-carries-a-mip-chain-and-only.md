## 15c-bis. GrabPass — the COLOR grab carries a mip chain, and only it (Aug 2026)

Frosted glass is a `textureLod()` away, provided the levels exist. `GrabPass::create()` gives the
**colour** image a full chain (`1 + floor(log2(max(w, h)))`), and `recordBlit()` generates it by
blitting the image onto itself, level N-1 to level N, right after the swap-chain copy into level 0.
The shader reads the LOD from the material roughness — see `src/Saphir/AGENTS.md` §
"Screen-space refraction".

⚠️ **Three things must agree or the blur is silently absent**, and each was a real trap:

1. The image needs **`VK_IMAGE_USAGE_TRANSFER_SRC_BIT`** on top of `TRANSFER_DST | SAMPLED` — the
   chain reads the image it writes.
2. The image **view** must expose every level (`levelCount = mipLevels`, it was 1).
3. The **sampler's `maxLod`** must reach them. It was pinned at `1.0F`, which would have clamped the
   blur to the first level even with a full chain present — and `Renderer::getSampler()` keys on the
   **NAME**, so the cache would have handed that stale sampler back. The name now carries the level
   count (`"GrabPass-<levels>"`). ⚠️ This is the same name-keyed-cache trap that once let a dummy
   texture win a shadow-map slot; check the key whenever a sampler's parameters depend on anything.

⚠️ **Layouts after the chain are NOT uniform.** Levels `[0, N-1)` end as `TRANSFER_SRC_OPTIMAL`
(they were each the source of the next blit) while only the last is still `TRANSFER_DST_OPTIMAL`.
The post-copy transition is therefore **two** barriers with `targetMipLevel(offset, count)`, not one
whole-image barrier — which would declare the wrong old layout for every level but one, and the
validation layers say so.

⚠️ **The depth / normals / material-properties grabs stay single-level on purpose.** They are read
as exact per-pixel values; a linearly filtered mip of a depth buffer is meaningless.

**Cost, measured by construction rather than profiled**: the chain adds ~1/3 of the base image in
extra writes, once per frame, and only on frames that already pay for the grab pass (it is armed by
`Scene::hasTranslucentGBObjects()`). ⚠️ Every level is generated even when every transmissive
material in the frame is perfectly smooth and reads only level 0 — closing that would require the
renderer to know the roughest grab-pass material of the frame. If this ever shows up in a profile,
that is the lever.
