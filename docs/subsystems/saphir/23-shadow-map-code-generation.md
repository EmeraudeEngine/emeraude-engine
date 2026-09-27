## Shadow Map Code Generation

Shadow map sampling code is generated in `LightGenerator.ShadowMap.cpp`. The code is separated by light type and PCF mode.

### Generation Functions

| Function | Purpose |
|----------|---------|
| `generate2DShadowMapCode()` | Non-PCF 2D shadow (directional, spot) |
| `generate2DShadowMapPCFCode()` | PCF-enabled 2D shadows |
| `generate3DShadowMapCode()` | Non-PCF cubemap shadow (point light) |
| `generate3DShadowMapPCFCode()` | PCF-enabled cubemap shadows |
| `generateCSMShadowMapCode()` | Cascaded Shadow Maps |

### PCF Methods

The `PCFMethod` enum controls soft shadow sampling:

| Method | Description |
|--------|-------------|
| `Grid` | Regular grid pattern, (2n+1)² samples |
| `VogelDisk` | Vogel spiral disk, even distribution |
| `PoissonDisk` | Pre-computed Poisson disk |
| `OptimizedGather` | 4-tap `textureGather` optimization |

**Code references:**
- `LightGenerator.ShadowMap.cpp` - All shadow map code generation
- `LightGenerator.hpp:PCFMethod` - PCF method enum
- `LightGenerator.hpp:m_PCFMethod` - Active PCF method

### Vertex Shader Projection Output

`generateVertexShaderShadowMapCode()` outputs position data for fragment shadow sampling AND color projection:

- **2D (directional/spot):** `PositionLightSpace` (vec4) - fragment position in light clip space (used by shadow maps AND color projection)
- **Cubemap (point):** `DirectionWorldSpace` (vec4) - direction from light to fragment (used by cubemap shadow AND cubemap color projection)

These outputs are generated whenever shadow maps OR color projection is enabled (not only for shadow maps).

### Settings Integration

Shadow mapping settings are read during generator construction:

| Setting | Member | Effect |
|---------|--------|--------|
| `GraphicsShadowMappingPCFEnabledKey` | `m_usePCF` | Enable/disable PCF |
| `GraphicsShadowMappingPCFMethodKey` | `m_PCFMethod` | PCF sampling method |
| `GraphicsShadowMappingPCFSampleKey` | `m_PCFSample` | Grid sample count |

See [`docs/shadow-mapping.md`](../../shadow-mapping.md) for complete shadow mapping architecture.
