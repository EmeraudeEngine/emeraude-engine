## 12. Post-Processing Effects

### FrameContext — Effect Chain Context (Jul 2026)

`IndirectPostProcessEffect::execute()` takes `(commandBuffer, inputColor, const
FrameContext &)` — the context groups the G-buffer inputs (depth/normals/matProps/albedo),
the LightSet, the ACTIVE CAMERA and the frame PushConstants. This replaced the former
8-parameter signature across all 16 effects (the GBufferInputs refactor). Any new
per-frame data belongs in FrameContext, NOT in a new parameter.

`FrameContext::view` (Oct 2026) is the frame's view reconstruction block — the ONLY way an effect turns a depth into a
position or a distance; the scalar `constants.nearPlane / farPlane / tanHalfFovY` are gone (they were frozen at
0.1 / 1000): [`41-the-view-reconstruction`](41-the-view-reconstruction-one-matrix-path-from-depth-to-position.md).
