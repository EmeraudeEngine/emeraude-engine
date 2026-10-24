---
id: iwyu-debt-since-2026-10-21
title: IWYU debt — the 158 C++ files touched while include-what-you-use did not run (2026-10-21 to 2026-10-25)
status: open
priority: high
scope: emeraude-engine src/
opened: 2026-10-25
tags: [iwyu, static-analysis, gate, debt]
---

# IWYU debt — the 158 C++ files touched while include-what-you-use did not run

## Why

From emeraude-base 145fda6 (2026-10-21, 16:51) to the fix of 2026-10-25, IWYU aborted every TU on the base mapping's
`<features.h>` entry and the gate read the empty output as "IWYU-clean" (static-analysis ledger § include-what-you-use,
the base ledger). The files below were touched in that window and were never really checked. Owner decision (2026-10-25): the
fix first, this debt as an item; each lot cleans the files it touches, and a dedicated pass empties the rest when the
owner orders it.

## What remains

- [ ] Run `iwyu_tool` on the files below (the command of the ledger § include-what-you-use), apply with projet-alpha
      `tools/iwyu-apply.py`, add the forward declarations by hand, build the three cascades, run the pass again: it
      must converge. Remove a file from the list when a lot brings it clean.
- [ ] A removed include is checked against the file's platform branches (`#ifdef _WIN32`, macOS, `__cpp_exceptions`)
      before it goes; the peers build it.

The files (as of 2026-10-25; the sky lots' files were brought clean that day):

- `src/Audio/Manager.cpp`
- `src/Audio/Manager.hpp`
- `src/Audio/Source.cpp`
- `src/Audio/Source.hpp`
- `src/Core.cpp`
- `src/Core.hpp`
- `src/Core.startup.cpp`
- `src/CursorAtlas.cpp`
- `src/Fuzzing/fuzz_ktx2.cpp`
- `src/Graphics/CompressedImageResource.cpp`
- `src/Graphics/CompressedImageResource.hpp`
- `src/Graphics/DeferredLightResolve.cpp`
- `src/Graphics/Effects/Atmosphere/AtmosphericFog.cpp`
- `src/Graphics/Effects/Atmosphere/VolumetricClouds.cpp`
- `src/Graphics/Effects/Atmosphere/VolumetricLight.cpp`
- `src/Graphics/Effects/Atmosphere/VolumetricScattering.cpp`
- `src/Graphics/Effects/Camera/LensFlare.cpp`
- `src/Graphics/Effects/Camera/LensFlare.hpp`
- `src/Graphics/Effects/Camera/ToneMapping.cpp`
- `src/Graphics/Effects/Camera/VeilingGlare.cpp`
- `src/Graphics/Effects/Camera/VeilingGlare.hpp`
- `src/Graphics/Effects/Lighting/RTGI.cpp`
- `src/Graphics/Effects/Lighting/RTR.cpp`
- `src/Graphics/Effects/Lighting/RTR.hpp`
- `src/Graphics/Effects/Lighting/SSGI.cpp`
- `src/Graphics/Effects/Lighting/SSR.cpp`
- `src/Graphics/Effects/Resolve/TAA.cpp`
- `src/Graphics/Effects/Resolve/TAA.hpp`
- `src/Graphics/Effects/Shared/AtmosphereGLSL.hpp`
- `src/Graphics/Effects/Shared/IrradianceProbesGLSL.hpp`
- `src/Graphics/Effects/Shared/RTAlphaTestGLSL.hpp`
- `src/Graphics/FrameCapture.cpp`
- `src/Graphics/FrameCapture.hpp`
- `src/Graphics/FrameDiagnostics.hpp`
- `src/Graphics/Geometry/IndexedVertexResource.cpp`
- `src/Graphics/Geometry/RawIndexedVertexResource.cpp`
- `src/Graphics/Geometry/RawVertexResource.cpp`
- `src/Graphics/Geometry/VertexGridResource.cpp`
- `src/Graphics/Geometry/VertexResource.cpp`
- `src/Graphics/GIDenoiser.cpp`
- `src/Graphics/GIDenoiser.hpp`
- `src/Graphics/GPUTextureSet.cpp`
- `src/Graphics/GPUTextureSet.hpp`
- `src/Graphics/ImageResource.cpp`
- `src/Graphics/ImageResource.hpp`
- `src/Graphics/IndirectPostProcessEffect.hpp`
- `src/Graphics/IrradianceProbeVolume.cpp`
- `src/Graphics/KTX2Decoder.cpp`
- `src/Graphics/KTX2Decoder.hpp`
- `src/Graphics/KTX2Writer.cpp`
- `src/Graphics/KTX2Writer.hpp`
- `src/Graphics/Material/Component/Texture.cpp`
- `src/Graphics/Material/Component/Texture.hpp`
- `src/Graphics/Material/GPURTMaterialData.hpp`
- `src/Graphics/Material/SkyAtmosphereResource.cpp`
- `src/Graphics/Material/SkyAtmosphereResource.hpp`
- `src/Graphics/Material/StandardResource.cpp`
- `src/Graphics/Material/StandardResource.hpp`
- `src/Graphics/MipChain.cpp`
- `src/Graphics/MipChain.hpp`
- `src/Graphics/MovieResource.cpp`
- `src/Graphics/MovieResource.hpp`
- `src/Graphics/PostProcessor.cpp`
- `src/Graphics/PostProcessor.hpp`
- `src/Graphics/PostProcessStack.console.cpp`
- `src/Graphics/Renderable/DynamicSkyResource.cpp`
- `src/Graphics/Renderable/DynamicSkyResource.hpp`
- `src/Graphics/RenderableInstance/Abstract.cpp`
- `src/Graphics/RenderableInstance/Abstract.hpp`
- `src/Graphics/RenderableInstance/Multiple.cpp`
- `src/Graphics/RenderableInstance/Multiple.hpp`
- `src/Graphics/Renderable/SpriteResource.cpp`
- `src/Graphics/Renderer.console.cpp`
- `src/Graphics/Renderer.cpp`
- `src/Graphics/Renderer.hpp`
- `src/Graphics/RenderTarget/Texture.hpp`
- `src/Graphics/SkyAtmosphere.hpp`
- `src/Graphics/StorageTexture2D.cpp`
- `src/Graphics/StorageTexture2D.hpp`
- `src/Graphics/TextureCache.cpp`
- `src/Graphics/TextureCache.hpp`
- `src/Graphics/TextureCompressor.cpp`
- `src/Graphics/TextureCompressor.hpp`
- `src/Graphics/TexturePacker.cpp`
- `src/Graphics/TexturePacker.hpp`
- `src/Graphics/TextureResource/Abstract.cpp`
- `src/Graphics/TextureResource/Abstract.hpp`
- `src/Graphics/TextureResource/AnimatedTexture2D.cpp`
- `src/Graphics/TextureResource/AnimatedTexture2D.hpp`
- `src/Graphics/TextureResource/StoreTexture.hpp`
- `src/Graphics/TextureResource/Texture1D.cpp`
- `src/Graphics/TextureResource/Texture2D.cpp`
- `src/Graphics/TextureResource/Texture2D.hpp`
- `src/Graphics/Types.hpp`
- `src/Graphics/ViewMatrices2DUBO.cpp`
- `src/Graphics/ViewMatrices2DUBO.hpp`
- `src/Graphics/ViewMatrices3DUBO.cpp`
- `src/Graphics/ViewMatrices3DUBO.hpp`
- `src/Graphics/ViewMatricesCascadedUBO.cpp`
- `src/Graphics/ViewMatricesCascadedUBO.hpp`
- `src/Graphics/ViewMatricesInterface.hpp`
- `src/Input/Manager.cpp`
- `src/Input/Manager.hpp`
- `src/Input/PointerController.cpp`
- `src/Input/PointerController.hpp`
- `src/Physics/Manager.cpp`
- `src/Resources/Manager.cpp`
- `src/Resources/ResourceTrait.cpp`
- `src/Resources/ResourceTrait.hpp`
- `src/Saphir/AbstractShader.cpp`
- `src/Saphir/Generator/Abstract.cpp`
- `src/Saphir/Generator/SceneRendering.cpp`
- `src/Saphir/Keys.hpp`
- `src/Saphir/LightGenerator.cpp`
- `src/Saphir/LightGenerator.hpp`
- `src/Scenes/AbstractEntity.cpp`
- `src/Scenes/AbstractEntity.hpp`
- `src/Scenes/Component/AudioConsoleAdapters.cpp`
- `src/Scenes/Component/ConsoleAdapter.hpp`
- `src/Scenes/Component/DirectionalLight.cpp`
- `src/Scenes/Component/MultipleVisuals.hpp`
- `src/Scenes/Component/ParticlesEmitter.cpp`
- `src/Scenes/Component/ParticlesEmitter.hpp`
- `src/Scenes/Component/PointLight.cpp`
- `src/Scenes/Component/SkyFollowsSun.cpp`
- `src/Scenes/Component/SkyFollowsSun.hpp`
- `src/Scenes/Component/SoundEmitter.cpp`
- `src/Scenes/Component/SoundEmitter.hpp`
- `src/Scenes/Component/SpotLight.cpp`
- `src/Scenes/Loaders/FBXLoader.cpp`
- `src/Scenes/Loaders/GLTFLoader.cpp`
- `src/Scenes/Loaders/GLTFLoader.hpp`
- `src/Scenes/Loaders/USDLoader.cpp`
- `src/Scenes/Loaders/USDLoader.hpp`
- `src/Scenes/Manager.console.cpp`
- `src/Scenes/Scene.cpp`
- `src/Scenes/Scene.entities.cpp`
- `src/Scenes/Scene.hpp`
- `src/Scenes/SceneMetaData.cpp`
- `src/Scenes/Scene.rendering.cpp`
- `src/Scenes/SkyState.hpp`
- `src/Scenes/Toolkit.cpp`
- `src/Scenes/Toolkit.hpp`
- `src/Scenes/Viewers/ImageViewer.cpp`
- `src/SettingKeys.hpp`
- `src/Vulkan/DeferredDestructor.hpp`
- `src/Vulkan/Device.cpp`
- `src/Vulkan/Image.cpp`
- `src/Vulkan/Image.hpp`
- `src/Vulkan/SwapChain.cpp`
- `src/Window.cpp`
- `src/Window.hpp`
- `src/Window.linux.cpp`
- `src/Window.windows.cpp`
- `tools/template-guard/TemplateInstantiation.cpp`
- `tools/texture-bake/MaterialBake.cpp`
- `tools/texture-bake/TextureBake.cpp`
- `tools/texture-bake/TextureBake.hpp`

## ⚠️ Traps

- A forward declaration IWYU proposes for a type held BY VALUE in a member (`std::array< T, N >`) or used in a template
  body is wrong: keep the include with `// IWYU pragma: keep (<reason>)`.
- IWYU sends `std::size_t` to `<cstdint>` when that header is already needed: libstdc++ only; keep `<cstddef>`.
