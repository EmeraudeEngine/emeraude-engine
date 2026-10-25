/*
 * tools/template-guard/TemplateInstantiation.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

/* NOTE: The compile-time guard of the engine (owner decision, 2026-10-19), as emeraude-base's
 * src/Testing/test_TemplateInstantiation.cpp. A member of a class template is compiled only when something calls it, a
 * header only when a translation unit includes it: this translation unit
 *   1. includes EVERY header of the engine (an OS-suffixed one on its OS only), so each one compiles under the
 *      PARANOID warnings of GCC, clang and MSVC;
 *   2. explicitly instantiates every class template with the arguments the cascade uses, which compiles EVERY member
 *      not excluded by its own 'requires' clause.
 * It is the OBJECT target EmeraudeTemplateGuard, never linked: the library gains nothing from it. On macOS it is compiled
 * through TemplateInstantiation.mac.mm (Objective-C++ with ARC), because an engine header is Objective-C++ only.
 * A new header goes in the list, a new class template gets its instantiation(s). A specialization a .cpp instantiates
 * is declared 'extern template' in its header and NOT repeated here (an explicit instantiation definition may appear
 * once in a program; MSVC refuses the repeat, C4661).
 * See docs/static-analysis-ledger.md § The template guard. */

/* STL inclusions. */
#include <memory>

/* Local inclusions: every header of the engine. */
// IWYU pragma: begin_keep
#include "Animations/AnimatableInterface.hpp"
#include "Animations/AnimationClipResource.hpp"
#include "Animations/AnimationInterface.hpp"
#include "Animations/ConstantValue.hpp"
#include "Animations/FlameFlicker.hpp"
#include "Animations/KeyFrame.hpp"
#include "Animations/LampFlicker.hpp"
#include "Animations/PlaybackWrap.hpp"
#include "Animations/RandomValue.hpp"
#include "Animations/Sequence.hpp"
#include "Animations/SkeletalAnimator.hpp"
#include "Animations/SkeletonResource.hpp"
#include "Arguments.hpp"
#include "Audio/AbstractObject.hpp"
#include "Audio/Ambience.hpp"
#include "Audio/AmbienceChannel.hpp"
#include "Audio/AmbienceSound.hpp"
#include "Audio/Buffer.hpp"
#include "Audio/EffectSlot.hpp"
#include "Audio/Effects/Abstract.hpp"
#include "Audio/Effects/AutoWah.hpp"
#include "Audio/Effects/Chorus.hpp"
#include "Audio/Effects/Compressor.hpp"
#include "Audio/Effects/Distortion.hpp"
#include "Audio/Effects/EAXReverb.hpp"
#include "Audio/Effects/Echo.hpp"
#include "Audio/Effects/Equalizer.hpp"
#include "Audio/Effects/Flanger.hpp"
#include "Audio/Effects/FrequencyShifter.hpp"
#include "Audio/Effects/PitchShifter.hpp"
#include "Audio/Effects/Reverb.hpp"
#include "Audio/Effects/RingModulator.hpp"
#include "Audio/Effects/VocalMorpher.hpp"
#include "Audio/ExternalInput.hpp"
#include "Audio/Filters/Abstract.hpp"
#include "Audio/Filters/BandPass.hpp"
#include "Audio/Filters/HighPass.hpp"
#include "Audio/Filters/LowPass.hpp"
#include "Audio/HardwareOutput.hpp"
#include "Audio/Listener.hpp"
#include "Audio/Manager.hpp"
#include "Audio/MusicResource.hpp"
#include "Audio/OpenALExtensions.hpp"
#include "Audio/OpenALHandles.hpp"
#include "Audio/PlayableInterface.hpp"
#include "Audio/PlaylistResource.hpp"
#include "Audio/Recorder.hpp"
#include "Audio/SoundResource.hpp"
#include "Audio/SoundfontResource.hpp"
#include "Audio/Source.hpp"
#include "Audio/Speaker.hpp"
#include "Audio/TrackMixer.hpp"
#include "Audio/Types.hpp"
#include "Audio/Utility.hpp"
#include "Console/Argument.hpp"
#include "Console/Command.hpp"
#include "Console/CommandResult.hpp"
#include "Console/CommandSignature.hpp"
#include "Console/ControllableTrait.hpp"
#include "Console/Controller.hpp"
#include "Console/Expression.hpp"
#include "Console/JSONNumber.hpp"
#include "Console/MCP/Protocol.hpp"
#include "Console/MCP/Server.hpp"
#include "Console/Output.hpp"
#include "Console/Parameter.hpp"
#include "Console/RemoteListener.hpp"
#include "Console/RemoteProtocol.hpp"
#include "Console/TypedBinding.hpp"
#include "Console/Types.hpp"
#include "Constants.hpp"
#include "Core.hpp"
#include "CoreTypes.hpp"
#include "CursorAtlas.hpp"
#include "FaultInjection.hpp"
#include "FileSystem.hpp"
#include "Graphics/AlphaCoverage.hpp"
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/CelestialBody.hpp"
#include "Graphics/CloudShadowMap.hpp"
#include "Graphics/CloudShapeResource.hpp"
#include "Graphics/CombinePass.hpp"
#include "Graphics/CompressedImageResource.hpp"
#include "Graphics/Compute/IBLBaker.hpp"
#include "Graphics/Compute/ProbeConvolver.hpp"
#include "Graphics/Compute/XRayAnalyzer.hpp"
#include "Graphics/CubemapMovieResource.hpp"
#include "Graphics/CubemapResource.hpp"
#include "Graphics/DeferredLightResolve.hpp"
#include "Graphics/DenoisePass.hpp"
#include "Graphics/DirectPostProcessEffect.hpp"
#include "Graphics/DummyColorProjectionTexture.hpp"
#include "Graphics/DummyShadowTexture.hpp"
#include "Graphics/EffectSlot.hpp"
#include "Graphics/Effects/Atmosphere/AerialPerspective.hpp"
#include "Graphics/Effects/Atmosphere/AtmosphericFog.hpp"
#include "Graphics/Effects/Atmosphere/VolumetricClouds.hpp"
#include "Graphics/Effects/Atmosphere/VolumetricLight.hpp"
#include "Graphics/Effects/Atmosphere/VolumetricScattering.hpp"
#include "Graphics/Effects/Camera/DepthOfField.hpp"
#include "Graphics/Effects/Camera/LensFlare.hpp"
#include "Graphics/Effects/Camera/MotionBlur.hpp"
#include "Graphics/Effects/Camera/ToneMapping.hpp"
#include "Graphics/Effects/Camera/VeilingGlare.hpp"
#include "Graphics/Effects/Lighting/RTAO.hpp"
#include "Graphics/Effects/Lighting/RTContactShadows.hpp"
#include "Graphics/Effects/Lighting/RTGI.hpp"
#include "Graphics/Effects/Lighting/RTR.hpp"
#include "Graphics/Effects/Lighting/SSAO.hpp"
#include "Graphics/Effects/Lighting/SSContactShadows.hpp"
#include "Graphics/Effects/Lighting/SSGI.hpp"
#include "Graphics/Effects/Lighting/SSR.hpp"
#include "Graphics/Effects/Resolve/FXAA.hpp"
#include "Graphics/Effects/Resolve/FXAASharpen.hpp"
#include "Graphics/Effects/Resolve/Sharpen.hpp"
#include "Graphics/Effects/Resolve/TAA.hpp"
#include "Graphics/Effects/Shared/AtmosphereGLSL.hpp"
#include "Graphics/Effects/Shared/CSMSamplingGLSL.hpp"
#include "Graphics/Effects/Shared/CloudVolumeGLSL.hpp"
#include "Graphics/Effects/Shared/IrradianceProbesGLSL.hpp"
#include "Graphics/Effects/Shared/LightFalloffGLSL.hpp"
#include "Graphics/Effects/Shared/LineLightGLSL.hpp"
#include "Graphics/Effects/Shared/MarchDitherGLSL.hpp"
#include "Graphics/Effects/Shared/RTAlphaTestGLSL.hpp"
#include "Graphics/Effects/Shared/StarMaskGLSL.hpp"
#include "Graphics/Effects/Shared/ViewReconstructionGLSL.hpp"
#include "Graphics/Effects/Style/BlackAndWhite.hpp"
#include "Graphics/Effects/Style/CRTPhosphor.hpp"
#include "Graphics/Effects/Style/ChromaBleeding.hpp"
#include "Graphics/Effects/Style/ChromaticAberration.hpp"
#include "Graphics/Effects/Style/ColorGrading.hpp"
#include "Graphics/Effects/Style/Dithering.hpp"
#include "Graphics/Effects/Style/DustAndHair.hpp"
#include "Graphics/Effects/Style/FilmGrain.hpp"
#include "Graphics/Effects/Style/Flicker.hpp"
#include "Graphics/Effects/Style/FrameMasking.hpp"
#include "Graphics/Effects/Style/PhosphorBloom.hpp"
#include "Graphics/Effects/Style/Pixelation.hpp"
#include "Graphics/Effects/Style/Retro.hpp"
#include "Graphics/Effects/Style/Scanlines.hpp"
#include "Graphics/Effects/Style/SignalGhosting.hpp"
#include "Graphics/Effects/Style/VHSAberration.hpp"
#include "Graphics/Effects/Style/VerticalJitter.hpp"
#include "Graphics/Effects/Style/Vignetting.hpp"
#include "Graphics/ExternalInput.hpp"
#include "Graphics/FontResource.hpp"
#include "Graphics/FrameCapture.hpp"
#include "Graphics/FrameDiagnostics.hpp"
#include "Graphics/FramebufferPrecisions.hpp"
#include "Graphics/Frustum.hpp"
#include "Graphics/GIDenoiser.hpp"
#include "Graphics/GPUTextureSet.hpp"
#include "Graphics/Geometry/CDLODTerrainResource.hpp"
#include "Graphics/Geometry/DisplacedGridResource.hpp"
#include "Graphics/Geometry/GenerationParameters.hpp"
#include "Graphics/Geometry/Geometries.hpp"
#include "Graphics/Geometry/HeightfieldDetailSurfaceResource.hpp"
#include "Graphics/Geometry/HeightfieldSurface.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Geometry/MeshShadingSurface.hpp"
#include "Graphics/Geometry/OceanSurface.hpp"
#include "Graphics/Geometry/OceanSurfaceResource.hpp"
#include "Graphics/Geometry/PulledVertexResource.hpp"
#include "Graphics/Geometry/RawGeometryOptions.hpp"
#include "Graphics/Geometry/RawIndexedVertexResource.hpp"
#include "Graphics/Geometry/RawVertexResource.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Geometry/SubGeometry.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"
#include "Graphics/Geometry/VertexResource.hpp"
#include "Graphics/GrabPass.hpp"
#include "Graphics/IBLTexture.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/ImposterAtlas.hpp"
#include "Graphics/IndirectPostProcessEffect.hpp"
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Graphics/IrradianceProbeVolume.hpp"
#include "Graphics/KTX2Decoder.hpp"
#include "Graphics/KTX2Writer.hpp"
#include "Graphics/LTCTables.hpp"
#include "Graphics/LTCTexture.hpp"
#include "Graphics/MDI/BatchBuilder.hpp"
#include "Graphics/MDI/PerDrawData.hpp"
#include "Graphics/Material/BeamResource.hpp"
#include "Graphics/Material/Component/Color.hpp"
#include "Graphics/Material/Component/Interface.hpp"
#include "Graphics/Material/Component/Texture.hpp"
#include "Graphics/Material/Component/Value.hpp"
#include "Graphics/Material/GPURTMaterialData.hpp"
#include "Graphics/Material/Helpers.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Material/Materials.hpp"
#include "Graphics/Material/PathResource.hpp"
#include "Graphics/Material/SkyAtmosphereResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/MipChain.hpp"
#include "Graphics/MoonSurfacePhotometry.hpp"
#include "Graphics/MovieResource.hpp"
#include "Graphics/OceanWaves.hpp"
#include "Graphics/OverflowCensus.hpp"
#include "Graphics/PathDebugOverlay.hpp"
#include "Graphics/Photometry.hpp"
#include "Graphics/PostProcessEffect.hpp"
#include "Graphics/PostProcessStack.hpp"
#include "Graphics/PostProcessor.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Recorder.hpp"
#include "Graphics/RenderDocCapture.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/RenderTarget/AbstractFilter.hpp"
#include "Graphics/RenderTarget/ImposterBake.hpp"
#include "Graphics/RenderTarget/ShadowMap.hpp"
#include "Graphics/RenderTarget/Texture.hpp"
#include "Graphics/RenderTarget/View.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Renderable/BasicSeaResource.hpp"
#include "Graphics/Renderable/ColorBackgroundResource.hpp"
#include "Graphics/Renderable/DynamicSkyResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/OceanResource.hpp"
#include "Graphics/Renderable/ProgramCacheKey.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Graphics/Renderable/SpriteResource.hpp"
#include "Graphics/Renderable/TerrainResource.hpp"
#include "Graphics/Renderable/Types.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/RenderableInstance/Multiple.hpp"
#include "Graphics/RenderableInstance/PathPoints.hpp"
#include "Graphics/RenderableInstance/RenderContext.hpp"
#include "Graphics/RenderableInstance/RenderStateTracker.hpp"
#include "Graphics/RenderableInstance/Unique.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RendererFrameScope.hpp"
#include "Graphics/SceneRenderTarget.hpp"
#include "Graphics/SelectionDepthTarget.hpp"
#include "Graphics/SelectionOutline.hpp"
#include "Graphics/SharedUBOManager.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Graphics/SkinnedGeometryProcessor.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/SkyEnvironment.hpp"
#include "Graphics/StarCatalogueResource.hpp"
#include "Graphics/StorageTexture2D.hpp"
#include "Graphics/StorageTexture3D.hpp"
#include "Graphics/TextureCache.hpp"
#include "Graphics/TextureCompressor.hpp"
#include "Graphics/TexturePacker.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/TextureResource/AnimatedTextureCubemap.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture1D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/TextureResource/Texture3D.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Graphics/Types.hpp"
#include "Graphics/VertexBufferBinding.hpp"
#include "Graphics/VertexBufferFormat.hpp"
#include "Graphics/VertexBufferFormatManager.hpp"
#include "Graphics/VideoColorConversion.hpp"
#include "Graphics/VideoFrameConverter.hpp"
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"
#include "Graphics/ViewMatricesCascadedUBO.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Graphics/VolumetricImageResource.hpp"
#include "Help/AbstractDoc.hpp"
#include "Help/ArgumentDoc.hpp"
#include "Help/Lexicon.hpp"
#include "Help/ShortcutDoc.hpp"
#include "Identification.hpp"
#include "Input/ControllerInterface.hpp"
#include "Input/GamepadController.hpp"
#include "Input/JoystickController.hpp"
#include "Input/KeyboardController.hpp"
#include "Input/KeyboardListenerInterface.hpp"
#include "Input/Manager.hpp"
#include "Input/PointerController.hpp"
#include "Input/PointerListenerInterface.hpp"
#include "Input/Types.hpp"
#include "Net/APIClient.hpp"
#include "Net/APIRequestItem.hpp"
#include "Net/DownloadItem.hpp"
#include "Net/Manager.hpp"
#include "Net/NetworkInterfaces.hpp"
#include "Net/SerialPort.hpp"
#include "Net/TCPClient.hpp"
#include "Net/TCPServer.hpp"
#include "Net/Types.hpp"
#include "Net/UDPClient.hpp"
#include "Net/WiFiScanner.hpp"
#include "Notifier.hpp"
#include "Overlay/Framebuffer.hpp"
#include "Overlay/FramebufferProperties.hpp"
#include "Overlay/ImGUIScreen.hpp"
#include "Overlay/Manager.hpp"
#include "Overlay/Surface.hpp"
#include "Overlay/UIScreen.hpp"
#include "Physics/BodyInertia.hpp"
#include "Physics/BodyPhysicalProperties.hpp"
#include "Physics/BoxCollisionModel.hpp"
#include "Physics/CapsuleCollisionModel.hpp"
#include "Physics/CharacterController.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "Physics/EnvironmentPhysicalProperties.hpp"
#include "Physics/Manager.hpp"
#include "Physics/MovableTrait.hpp"
#include "Physics/NarrowPhase.hpp"
#include "Physics/Particle.hpp"
#include "Physics/Physics.hpp"
#include "Physics/PointCollisionModel.hpp"
#include "Physics/SoftStepSolver.hpp"
#include "Physics/SphereCollisionModel.hpp"
#include "Physics/SurfacePhysicalProperties.hpp"
#include "Physics/TriangleMeshCollisionModel.hpp"
#include "Physics/Vehicle.hpp"
#include "PlatformManager.hpp"
#include "PlatformSpecific/Desktop/Commands.hpp"
#include "PlatformSpecific/Desktop/Dialog/Abstract.hpp"
#include "PlatformSpecific/Desktop/Dialog/CustomMessage.hpp"
#include "PlatformSpecific/Desktop/Dialog/Message.hpp"
#include "PlatformSpecific/Desktop/Dialog/OpenFile.hpp"
#include "PlatformSpecific/Desktop/Dialog/SaveFile.hpp"
#include "PlatformSpecific/Desktop/Dialog/TextInput.hpp"
#include "PlatformSpecific/Desktop/Dialog/Types.hpp"
#include "PlatformSpecific/Desktop/Notification.hpp"
#include "PlatformSpecific/Helpers.hpp"
#include "PlatformSpecific/StorageInfo.hpp"
#include "PlatformSpecific/SystemInfo.hpp"
#include "PlatformSpecific/Types.hpp"
#include "PlatformSpecific/UserInfo.hpp"
#include "PlatformSpecific/VideoCaptureDevice.hpp"
#include "PrimaryServices.hpp"
#include "Resources/BaseInformation.hpp"
#include "Resources/Container.hpp"
#include "Resources/LoadingRequest.hpp"
#include "Resources/Manager.hpp"
#include "Resources/PeerStore.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Resources/SharingServer.hpp"
#include "Resources/Types.hpp"
#include "Saphir/AbstractShader.hpp"
#include "Saphir/AbstractVertexStage.hpp"
#include "Saphir/BeamGLSL.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/CodeGeneratorInterface.hpp"
#include "Saphir/ComputeShader.hpp"
#include "Saphir/Declaration/AbstractBufferBackedBlock.hpp"
#include "Saphir/Declaration/AbstractShaderBlock.hpp"
#include "Saphir/Declaration/BlockInterface.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Declaration/InputAttribute.hpp"
#include "Saphir/Declaration/InputBlock.hpp"
#include "Saphir/Declaration/InputPrimitive.hpp"
#include "Saphir/Declaration/Interface.hpp"
#include "Saphir/Declaration/Member/BufferBackedBlock.hpp"
#include "Saphir/Declaration/Member/Interface.hpp"
#include "Saphir/Declaration/Member/PushConstant.hpp"
#include "Saphir/Declaration/Member/ShaderBlock.hpp"
#include "Saphir/Declaration/Member/Structure.hpp"
#include "Saphir/Declaration/OutputBlock.hpp"
#include "Saphir/Declaration/OutputFragment.hpp"
#include "Saphir/Declaration/OutputPrimitive.hpp"
#include "Saphir/Declaration/PushConstantBlock.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/ShaderStorageBlock.hpp"
#include "Saphir/Declaration/SpecializationConstant.hpp"
#include "Saphir/Declaration/StageInput.hpp"
#include "Saphir/Declaration/StageOutput.hpp"
#include "Saphir/Declaration/Structure.hpp"
#include "Saphir/Declaration/TexelBuffer.hpp"
#include "Saphir/Declaration/Types.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Generator/Abstract.hpp"
#include "Saphir/Generator/GizmoRendering.hpp"
#include "Saphir/Generator/HeightfieldSurfaceHelper.hpp"
#include "Saphir/Generator/MeshShadingSurfaceHelper.hpp"
#include "Saphir/Generator/OceanSurfaceHelper.hpp"
#include "Saphir/Generator/OverlayRendering.hpp"
#include "Saphir/Generator/PostProcessing.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/Generator/ShadowCasting.hpp"
#include "Saphir/Generator/SkinningLayoutHelper.hpp"
#include "Saphir/Generator/TBNSpaceRendering.hpp"
#include "Saphir/GeometryShader.hpp"
#include "Saphir/ImposterGLSL.hpp"
#include "Saphir/Keys.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Saphir/MeshShader.hpp"
#include "Saphir/PathGLSL.hpp"
#include "Saphir/Program.hpp"
#include "Saphir/SetIndexes.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/TaskShader.hpp"
#include "Saphir/TesselationControlShader.hpp"
#include "Saphir/TesselationEvaluationShader.hpp"
#include "Saphir/Types.hpp"
#include "Saphir/VertexShader.hpp"
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Scenes/AVConsole/Types.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Scenes/BindlessTextureSet.hpp"
#include "Scenes/CloudSet.hpp"
#include "Scenes/Component/Abstract.hpp"
#include "Scenes/Component/AbstractLightEmitter.hpp"
#include "Scenes/Component/AbstractModifier.hpp"
#include "Scenes/Component/Beam.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/CelestialClock.hpp"
#include "Scenes/Component/CharacterController.hpp"
#include "Scenes/Component/CloudVolume.hpp"
#include "Scenes/Component/ConsoleAdapter.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Component/DirectionalPushModifier.hpp"
#include "Scenes/Component/LineLight.hpp"
#include "Scenes/Component/Microphone.hpp"
#include "Scenes/Component/MultipleVisuals.hpp"
#include "Scenes/Component/NodeAnimation.hpp"
#include "Scenes/Component/ParticlesEmitter.hpp"
#include "Scenes/Component/Path.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SkyFollowsSun.hpp"
#include "Scenes/Component/SoundEmitter.hpp"
#include "Scenes/Component/SphericalPushModifier.hpp"
#include "Scenes/Component/SpotLight.hpp"
#include "Scenes/Component/Vehicle.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Scenes/Component/Weight.hpp"
#include "Scenes/CubicInfluenceArea.hpp"
#include "Scenes/Debug/Compass.hpp"
#include "Scenes/DefinitionResource.hpp"
#include "Scenes/DirectionalShadowOptions.hpp"
#include "Scenes/Editor/Gizmo/Abstract.hpp"
#include "Scenes/Editor/Gizmo/Rotate.hpp"
#include "Scenes/Editor/Gizmo/Scale.hpp"
#include "Scenes/Editor/Gizmo/Translate.hpp"
#include "Scenes/Editor/Manager.hpp"
#include "Scenes/EffectsToolkit/CameraPresets.hpp"
#include "Scenes/EffectsToolkit/FX.hpp"
#include "Scenes/EffectsToolkit/StylePresets.hpp"
#include "Scenes/GPUMeshMetaData.hpp"
#include "Scenes/GroundLevelInterface.hpp"
#include "Scenes/GroundTriangles.hpp"
#include "Scenes/InfluenceAreaInterface.hpp"
#include "Scenes/InstanceCluster.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/Loaders/FBXLoader.hpp"
#include "Scenes/Loaders/FBXParser.hpp"
#include "Scenes/Loaders/GLTFLoader.hpp"
#include "Scenes/Loaders/GLTFParser.hpp"
#include "Scenes/Loaders/GeometryDeduplicator.hpp"
#include "Scenes/Loaders/Interface.hpp"
#include "Scenes/Loaders/LoaderOptions.hpp"
#include "Scenes/Loaders/SceneData.hpp"
#include "Scenes/Loaders/USDLoader.hpp"
#include "Scenes/Loaders/USDParser.hpp"
#include "Scenes/Loaders/USDZArchive.hpp"
#include "Scenes/Loaders/WADLoader.hpp"
#include "Scenes/Loaders/WADParser.hpp"
#include "Scenes/LocatableInterface.hpp"
#include "Scenes/Manager.hpp"
#include "Scenes/Node.hpp"
#include "Scenes/NodeController.hpp"
#include "Scenes/NodeCrawler.hpp"
#include "Scenes/OctreeSector.hpp"
#include "Scenes/OctreeSectorCrawler.hpp"
#include "Scenes/OrbitController.hpp"
#include "Scenes/ParticipatingMedium.hpp"
#include "Scenes/PhysicsRecorder.hpp"
#include "Scenes/RenderBatch.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/SceneDataConsumer.hpp"
#include "Scenes/SceneInstanceTransforms.hpp"
#include "Scenes/SceneMetaData.hpp"
#include "Scenes/SeaLevelInterface.hpp"
#include "Scenes/SkyState.hpp"
#include "Scenes/SphericalInfluenceArea.hpp"
#include "Scenes/StaticEntity.hpp"
#include "Scenes/Toolkit.hpp"
#include "Scenes/Viewers/ImageViewer.hpp"
#include "Scenes/Viewers/ModelViewer.hpp"
#include "ServiceInterface.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "SettingsKeyRestoration.hpp"
#include "SystemNotification.hpp"
#include "Tool/GeometryDataPrinter.hpp"
#include "Tool/ShowVulkanInformation.hpp"
#include "Tool/ToolInterface.hpp"
#include "Tracer.hpp"
#include "TracerLogger.hpp"
#include "User.hpp"
#include "Vulkan/AbstractDeviceDependentObject.hpp"
#include "Vulkan/AbstractObject.hpp"
#include "Vulkan/AccelerationStructure.hpp"
#include "Vulkan/AccelerationStructureBuilder.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/BufferTransferOperation.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DebugMessenger.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DeviceMemory.hpp"
#include "Vulkan/DeviceQueueConfiguration.hpp"
#include "Vulkan/DeviceRequirements.hpp"
#include "Vulkan/ExternalImageDescriptor.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GPUProfiler.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageTransferOperation.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/IndexBufferObject.hpp"
#include "Vulkan/IndirectBuffer.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/Loader.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/PendingSubmissions.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/PhysicalDeviceProbe.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/PortabilitySubset.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/RenderSubPass.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Sync/BufferMemoryBarrier.hpp"
#include "Vulkan/Sync/Event.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Vulkan/Sync/MemoryBarrier.hpp"
#include "Vulkan/Sync/Semaphore.hpp"
#include "Vulkan/TextureInterface.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/Types.hpp"
#include "Vulkan/UniformBufferObject.hpp"
#include "Vulkan/Utility.hpp"
#include "Vulkan/VertexBufferObject.hpp"
#include "Vulkan/VideoEncoderH265.hpp"
#include "Window.hpp"
#include "emeraude_export.hpp"
#ifdef __APPLE__
#include "PlatformSpecific/StringConversion.mac.hpp"
#endif
#ifdef _WIN32
#include "PlatformSpecific/COM.windows.hpp"
#endif
// IWYU pragma: end_keep

/* Local inclusions for the instantiations below. */
#include "Logging/Severity.hpp"

/* Every class template, with the arguments the cascade uses. */
template class EmEn::Graphics::RenderTarget::ShadowMap< EmEn::Graphics::ViewMatrices2DUBO >;
template class EmEn::Graphics::RenderTarget::ShadowMap< EmEn::Graphics::ViewMatrices3DUBO >;
template class EmEn::Graphics::RenderTarget::ShadowMap< EmEn::Graphics::ViewMatricesCascadedUBO >;
template class EmEn::Graphics::RenderTarget::Texture< EmEn::Graphics::ViewMatrices2DUBO >;
template class EmEn::Graphics::RenderTarget::Texture< EmEn::Graphics::ViewMatrices3DUBO >;
template class EmEn::Graphics::RenderTarget::View< EmEn::Graphics::ViewMatrices2DUBO >;
template class EmEn::Graphics::RenderTarget::View< EmEn::Graphics::ViewMatrices3DUBO >;
template class EmEn::Scenes::NodeCrawler< EmEn::Scenes::Node >;
template class EmEn::Scenes::NodeCrawler< const EmEn::Scenes::Node >;
template class EmEn::Scenes::OctreeSector< EmEn::Scenes::AbstractEntity, true >;
template class EmEn::Scenes::OctreeSector< EmEn::Scenes::AbstractEntity, false >;
template class EmEn::Scenes::OctreeSectorCrawler< EmEn::Scenes::OctreeSector< EmEn::Scenes::AbstractEntity, true > >;
template class EmEn::Scenes::OctreeSectorCrawler< EmEn::Scenes::OctreeSector< EmEn::Scenes::AbstractEntity, false > >;
template class EmEn::Resources::Container< EmEn::Graphics::TextureResource::AnimatedTextureCubemap >;
template class EmEn::Resources::Container< EmEn::Animations::AnimationClipResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::BasicGroundResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::BasicSeaResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Material::BeamResource >;
template class EmEn::Resources::Container< EmEn::Graphics::CloudShapeResource >;
template class EmEn::Resources::Container< EmEn::Graphics::CompressedImageResource >;
template class EmEn::Resources::Container< EmEn::Graphics::CubemapMovieResource >;
template class EmEn::Resources::Container< EmEn::Graphics::CubemapResource >;
template class EmEn::Resources::Container< EmEn::Scenes::DefinitionResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::DynamicSkyResource >;
template class EmEn::Resources::Container< EmEn::Graphics::FontResource >;
template class EmEn::Resources::Container< EmEn::Graphics::ImageResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Geometry::IndexedVertexResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::MeshResource >;
template class EmEn::Resources::Container< EmEn::Graphics::MovieResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::MultiLayerMeshResource >;
template class EmEn::Resources::Container< EmEn::Audio::MusicResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Material::PathResource >;
template class EmEn::Resources::Container< EmEn::Audio::PlaylistResource >;
template class EmEn::Resources::Container< EmEn::Graphics::StarCatalogueResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Geometry::PulledVertexResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Geometry::RawIndexedVertexResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Geometry::RawVertexResource >;
template class EmEn::Resources::Container< EmEn::Animations::SkeletonResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::SkyBoxResource >;
template class EmEn::Resources::Container< EmEn::Audio::SoundResource >;
template class EmEn::Resources::Container< EmEn::Audio::SoundfontResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::SpriteResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Material::StandardResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Renderable::TerrainResource >;
template class EmEn::Resources::Container< EmEn::Graphics::TextureResource::Texture2D >;
template class EmEn::Resources::Container< EmEn::Graphics::TextureResource::Texture3D >;
template class EmEn::Resources::Container< EmEn::Graphics::TextureResource::TextureCubemap >;
template class EmEn::Resources::Container< EmEn::Graphics::VolumetricImageResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Geometry::VertexGridResource >;
template class EmEn::Resources::Container< EmEn::Graphics::Geometry::VertexResource >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::Beam >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::Camera >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::CloudVolume >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::DirectionalLight >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::DirectionalPushModifier >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::LineLight >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::MultipleVisuals >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::NodeAnimation >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::ParticlesEmitter >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::Path >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::PointLight >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::SkyFollowsSun >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::SoundEmitter >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::SphericalPushModifier >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::SpotLight >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::CelestialClock >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::Visual >;
template class EmEn::Scenes::Component::ConsoleAdapter< EmEn::Scenes::Component::Weight >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::Node, EmEn::Scenes::Component::Camera >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::Node, EmEn::Scenes::Component::CelestialClock >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::StaticEntity, EmEn::Scenes::Component::Visual >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::Node, EmEn::Scenes::Component::Visual >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::Node, EmEn::Scenes::Component::DirectionalLight >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::Node, EmEn::Scenes::Component::SpotLight >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::StaticEntity, EmEn::Scenes::Component::Camera >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::StaticEntity, EmEn::Scenes::Component::DirectionalLight >;
template class EmEn::Scenes::BuiltEntity< EmEn::Scenes::StaticEntity, EmEn::Scenes::Component::SpotLight >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::Beam >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::Camera >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::CloudVolume >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::DirectionalLight >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::DirectionalPushModifier >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::LineLight >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::MultipleVisuals >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::NodeAnimation >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::ParticlesEmitter >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::Path >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::PointLight >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::SkyFollowsSun >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::SoundEmitter >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::SphericalPushModifier >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::SpotLight >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::CelestialClock >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::Visual >;
template class EmEn::Scenes::ComponentBuilder< EmEn::Scenes::Component::Weight >;
template class EmEn::T_TraceHelperBase< EmEn::TraceDebug, EmEn::Severity::Debug >;
template class EmEn::T_TraceHelperBase< EmEn::TraceInfo, EmEn::Severity::Info >;
template class EmEn::T_TraceHelperBase< EmEn::TraceSuccess, EmEn::Severity::Success >;
template class EmEn::T_TraceHelperBase< EmEn::TraceWarning, EmEn::Severity::Warning >;
template class EmEn::T_TraceHelperBase< EmEn::TraceError, EmEn::Severity::Error >;
