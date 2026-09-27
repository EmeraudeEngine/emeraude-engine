## 12. Post-Processing Effects

### Physical Camera — Camera-Driven Photographic Pipeline (Jul 2026)

The `Scenes::Component::Camera` is the **single source of truth for the photographic
behaviour** of the rendered image, like a real camera body. Owner vision: ultimately ALL
image-rendering effects are camera-manageable (lens effects already are).

#### The near plane is DERIVED from the subject's scale (fixed 2026-08-28)

> [!CAUTION]
> `nearPlane = nearestObjectDistance / sqrt(1 + tan²(fov/2) · (aspectRatio² + 1))`, and
> `nearestObjectDistance` was the **literal constant `0.1F`** in FOUR hand-maintained copies —
> `ViewMatrices2DUBO`, `ViewMatrices3DUBO`, `ViewMatricesCascadedUBO` and `SpotLight`, the last of
> which carried a comment claiming consistency with the first, a coupling kept by hand. Every scene
> therefore got a near plane sized for a decimetre subject, about **0.089 m**.

**It was wrong in both directions, and the second half is the one nobody notices.**

- **Small:** a millimetric subject sits entirely INSIDE the near plane and renders nothing. Measured
  on the Khronos `MetalRoughSpheresNoTextures`, radius 0.00035 m — 89 times inside it. The glTF
  bench had to push the camera out and could only reach **5.5 %** of frame height; `BoomBox`
  (r = 0.0101 m) **14.1 %**. Both now plan at the nominal **38.9 %**.
- **Large:** the depth test is conventional — `VK_COMPARE_OP_LESS_OR_EQUAL`, `D32_SFLOAT`, **no
  reversed-Z anywhere in the engine** — so a float depth buffer concentrates its precision near the
  near plane. A 0.089 m near against a kilometre-scale far spends that precision in the first ten
  centimetres, where an outdoor scene needs it least. Any distant z-fighting should look here first.

The single derivation now lives in `ViewMatricesInterface::computeNearPlane(nearestObjectDistance,
fov, aspectRatio)` — pass `aspectRatio = 1` for a cube face or a spot light, which reduces the
bracket to 2 and reproduces the old hand-written expressions bit for bit. ⚠️ **Do not write the
formula again**; four copies is how it drifted.

The distance is a **camera property**, symmetric with the far distance:
`Camera::setNearestObjectDistance()` → `AbstractVirtualDevice::updateNearestObjectDistance()` (a
defaulted no-op, so the five other video devices and every audio device are untouched) →
`SceneRenderTarget` → `ViewMatricesInterface::setNearestObjectDistance()`. A caller that says nothing
keeps `DefaultNearestObjectDistance` (0.1 m) and behaves exactly as before.

⚠️⚠️ **The property has to be forwarded by FIVE separate owners.** `Vulkan::SwapChain`,
`Graphics::SceneRenderTarget` and the templated `RenderTarget::View`, `Texture` and `ShadowMap` each
hold their own `ViewMatrices2DUBO`; there is no common holder. **The one a scene camera actually
connects to is the SWAP CHAIN** — doing only `SceneRenderTarget` left the near plane at its default
and the two small conformance models rendered entirely empty. `ShadowMap` alone may ignore it (a
light's frustum has no subject).
⚠️⚠️ **`Camera::onOutputDeviceConnected()` is the FIRST push a target receives**, and every camera
setter is guarded by `hasOutputConnected()` — so a value set while the scene is still being built,
which is exactly what a viewer does, is stored and never sent unless that hook pushes it too.

⚠️ **`+ModelViewer` had TWO more decimetre assumptions of its own**, and the near plane alone would
not have fixed it: the framing radius was floored at `0.01F` (so every sub-centimetre asset was
re-framed as if it were a centimetre across — 28× too far for `MetalRoughSpheresNoTextures`), and the
orbit controller's lower distance limit at `max(radius * 0.05F, 0.01F)`, which `setDistance()`
CLAMPS against. Both are now scale-relative. When a scale bug survives one fix, look for the other
floors.
