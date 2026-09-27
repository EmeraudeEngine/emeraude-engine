## 12. Post-Processing Effects

### FrameContext — Effect Chain Context (Jul 2026)

`IndirectPostProcessEffect::execute()` takes `(commandBuffer, inputColor, const
FrameContext &)` — the context groups the G-buffer inputs (depth/normals/matProps/albedo),
the LightSet, the ACTIVE CAMERA and the frame PushConstants. This replaced the former
8-parameter signature across all 16 effects (the GBufferInputs refactor). Any new
per-frame data belongs in FrameContext, NOT in a new parameter.
