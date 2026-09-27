## Important Files

- `Generator/Abstract.cpp/.hpp` - Base class for all generators, implements cache lookup
- `Generator/SceneRendering.cpp/.hpp` - 3D scene rendering generator
- `Generator/ShadowCasting.cpp/.hpp` - Shadow map generator
- `Generator/OverlayRendering.cpp/.hpp` - 2D overlay generator
- `LightGenerator.cpp/.hpp` - Lighting code generation (PerFragment, PerVertex, PBR, NormalMap, color projection)
- `AbstractVertexStage.cpp/.hpp` - The PER-VERTEX stage feeding the rasterizer: the whole synthesis machinery (`requestSynthesizeInstruction()`, the synthetic variables, model-matrix sources, skinning/wind/heightfield). `VertexShader.hpp` is a thin concrete stage on top of it
- `Program.cpp/.hpp` - Shader program (shaders + pipeline layout)
- `ShaderManager.cpp/.hpp` - ShaderModule cache and compilation
