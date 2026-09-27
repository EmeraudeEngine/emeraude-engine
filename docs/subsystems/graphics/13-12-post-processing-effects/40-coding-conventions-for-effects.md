## 12. Post-Processing Effects

### Coding Conventions for Effects

**Use engine types for semantic data, raw floats for GPU push constants.**

The engine provides rich types in `Base/` that should be used for all user-facing parameters, member variables, and API signatures. Push constant structs are the only exception — they must remain raw `float` fields for GPU memory layout compliance.

| Semantic | Engine type | Header |
|----------|------------|--------|
| Direction (3D) | `Base::Math::Vector< 3, float >` | `Base/Math/Vector.hpp` |
| Position (3D) | `Base::Math::Vector< 3, float >` | `Base/Math/Vector.hpp` |
| Color (RGB/RGBA) | `Base::PixelFactory::Color<>` | `Base/PixelFactory/Color.hpp` |
| Rotation | `Base::Math::Quaternion< float >` | `Base/Math/Quaternion.hpp` |
| Transform | `Base::Math::Matrix< 4, float >` | `Base/Math/Matrix.hpp` |

**Aliases**: `Vector3F` = `Vector< 3, float >`, `ColorF` = `Color< float >`.

**Rules:**
1. **`Parameters` struct** — Use `Color<>` for colors, `Vector< 3, float >` for directions/positions. Never use `float xxxR, xxxG, xxxB` or `float dirX, dirY, dirZ`.
2. **Member variables** — Same rule: `m_lightDirection` (`Vector< 3, float >`), not `m_lightDirX/Y/Z`.
3. **Setter methods** — Accept engine types: `setLightDirection(const Vector< 3, float > &)`, not `(float x, float y, float z)`.
4. **Push constant structs** — Keep as raw `float` fields (POD with `static_assert` on size). These are GPU-uploaded verbatim.
5. **Populating push constants from engine types** — Use accessors: `color.red()`, `color.green()`, `color.blue()`, `vec.x()`, `vec.y()`, `vec.z()`.
6. **Normalization** — Use `vector.normalized()` instead of manual `sqrt()` + division.
7. **Constructor** — Accept `const Parameters & parameters = {}` to allow inline initialization at construction.

**Example (AtmosphericFog):**
```cpp
// Parameters struct — engine types
struct Parameters {
    float density{0.02F};
    Base::PixelFactory::Color<> fogColor{0.5F, 0.6F, 0.7F};
    // ...
};

// Member — engine type
Base::Math::Vector< 3, float > m_lightDirection{0.0F, -1.0F, 0.0F};

// Setter — engine type
void setLightDirection(const Base::Math::Vector< 3, float > & direction) noexcept;

// Push constants — raw floats (GPU layout)
struct FogPushConstants {
    float fogColorR, fogColorG, fogColorB;
    float lightDirX, lightDirY, lightDirZ;
};

// Populate — accessors
.fogColorR = m_parameters.fogColor.red(),
.lightDirX = lightDir.x(),
```

**Reference implementation:** `Effects/Atmosphere/AtmosphericFog.hpp/cpp`
