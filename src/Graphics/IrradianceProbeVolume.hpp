/*
 * src/Graphics/IrradianceProbeVolume.hpp
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

#pragma once

/* Engine configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Vulkan
{
	class Device;
	class Image;
	class ImageView;
	class Sampler;
	class ShaderStorageBufferObject;
	class UniformBufferObject;
	class DescriptorPool;
	class DescriptorSetLayout;
	class DescriptorSet;
	class PipelineLayout;
	class ComputePipeline;
	class CommandBuffer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The irradiance probe volume — the engine's radiance cache (DDGI).
	 * @details A camera-centred, scrolling grid of probes. Every frame each probe traces a rotated
	 * spherical-Fibonacci set of rays against the TLAS (ray query in a compute pass), shades the
	 * hits with the scene's direct lighting (shadow-ray gated), the scene ambient and the volume's
	 * OWN irradiance at the hit (infinite bounces through feedback), turns misses into sky
	 * radiance, then blends the result into two octahedral atlases per probe — cosine-weighted
	 * radiance (E/π, the RTGI history convention) and distance moments for the Chebyshev
	 * visibility test — with temporal hysteresis. Consumers splice `EMEN_IRRADIANCE_PROBES_GLSL`
	 * (Effects/Shared/IrradianceProbesGLSL.hpp) and call `probeIrradiance(position, normal, viewDir)`
	 * at ANY world position, on screen or not — the light a reflection hit behind the camera
	 * receives from the rest of the room, which nothing else in the engine could tell it.
	 *
	 * References: Majercik, Guertin, Nowrouzezahrai, McGuire, "Dynamic Diffuse Global Illumination
	 * with Ray-Traced Irradiance Fields", JCGT 8(2), 2019 (https://jcgt.org/published/0008/02/01/);
	 * Majercik, Marrs, Spjut, McGuire, "Scaling Probe-Based Real-Time Dynamic Global Illumination
	 * for Production", JCGT 10(2), 2021 (https://jcgt.org/published/0010/02/01/) — the infinite
	 * scrolling volume follows NVIDIA's RTXGI SDK. Octahedral mapping: Cigolle et al., JCGT 3(2), 2014.
	 *
	 * @note ONE instance is owned by the renderer, next to the acceleration structure builder, and
	 * exists whenever the traced lane is resident. `Enabled` (settings) gates the per-frame update and
	 * the shader-side flag, not the allocation: a consumer's pipeline layout binds the set either way.
	 * @note The parameters are read once at initialization (settings under
	 * `Core/Graphics/RayTracing/IrradianceProbes/`); a change needs a relaunch.
	 */
	class EMEN_API IrradianceProbeVolume final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"IrradianceProbeVolume"};

			/** @brief Interior texels per probe side of the irradiance atlas (octahedral map). */
			static constexpr uint32_t IrradianceTexels{8};
			/** @brief Interior texels per probe side of the distance atlas (octahedral map). */
			static constexpr uint32_t DistanceTexels{16};
			/** @brief Upper bound of rays per probe (the update passes stage the ray set in shared memory). */
			static constexpr uint32_t MaxRaysPerProbe{512};

			/**
			 * @brief User-facing parameters, read from the settings at initialization.
			 */
			struct EMEN_API Parameters
			{
				uint32_t probeCountX{16};
				uint32_t probeCountY{8};
				uint32_t probeCountZ{16};
				uint32_t raysPerProbe{128};
				float probeSpacing{1.5F};
				/** @brief Where the camera sits in the volume's height (0 = bottom plane, 1 = top plane). */
				float cameraHeightFraction{0.35F};
				float hysteresis{0.97F};
				/** @brief Weight of the volume's own irradiance re-injected at a ray hit (0 = single bounce, 1 = full feedback). */
				float bounceFeedback{1.0F};
				float normalBias{0.1F};
				float viewBias{0.1F};
				/** @brief The IndirectDiffuse intensity (Core/Graphics/PostProcessing/IndirectDiffuse/Intensity), carried for the consumers that compose an image (RTR applies it; the probes' recursion and the RTGI feedback consume energy). */
				float indirectIntensity{0.8F};
				bool enabled{true};
			};

			/**
			 * @brief What one frame's update needs from the scene.
			 */
			struct EMEN_API FrameInputs
			{
				/** @brief The volume is centred on this position (the active camera). */
				Base::Math::Vector< 3, float > cameraPosition;
				/** @brief Scene ambient colour x effective illuminance — the raster's ambient term, added at every hit. */
				Base::PixelFactory::Color< float > ambient;
				/** @brief Luminance of the environment cubemap in nits (0 = no sky): a miss becomes this radiance. */
				float skyLuminance{0.0F};
				/** @brief Lights in the RT light SSBO. */
				uint32_t lightCount{0};
			};

			/**
			 * @brief Constructs the volume.
			 * @note Out of line, like the destructor: MSVC instantiates the members' destructors from an inline
			 * constructor (C2027 on the forward-declared Vulkan::ComputePipeline, found by the Windows build on
			 * 2026-09-22); GCC and Clang do not, so only a Windows build can catch it.
			 */
			IrradianceProbeVolume () noexcept;

			/**
			 * @brief Destructs the volume.
			 * @note Out of line: members are smart pointers to forward-declared types.
			 */
			~IrradianceProbeVolume () noexcept;

			/**
			 * @brief Reads the settings and creates every GPU resource and pipeline.
			 * @param renderer A reference to the graphics renderer (device, layouts, shader manager, RT set layout).
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize (Renderer & renderer) noexcept;

			/**
			 * @brief Releases every GPU resource (while the device still exists).
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Returns whether the volume can be bound by a consumer and updated.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usable () const noexcept
			{
				return m_tracePipeline != nullptr;
			}

			/**
			 * @brief Returns whether the volume is updated and consulted (the `Enabled` setting).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			enabled () const noexcept
			{
				return m_parameters.enabled;
			}

			/**
			 * @brief Returns the parameters in use.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Returns the number of probes.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			probeCount () const noexcept
			{
				return m_parameters.probeCountX * m_parameters.probeCountY * m_parameters.probeCountZ;
			}

			/**
			 * @brief Returns the descriptor set layout a consumer adds to its pipeline layout.
			 * @note Bindings 3 (parameters UBO), 4 (irradiance atlas) and 5 (distance atlas) are the
			 * consumer-facing ones — what `EMEN_IRRADIANCE_PROBES_GLSL` declares.
			 * @return const std::shared_ptr< Vulkan::DescriptorSetLayout > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::DescriptorSetLayout > &
			descriptorSetLayout () const noexcept
			{
				return m_descriptorSetLayout;
			}

			/**
			 * @brief Returns the descriptor set of a frame in flight, to bind at the consumer's set index.
			 * @param frameIndex The frame in flight index.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			const Vulkan::DescriptorSet * descriptorSet (uint32_t frameIndex) const noexcept;

			/**
			 * @brief Records this frame's update: scroll, parameters, trace, blend, borders.
			 * @note Record AFTER the TLAS build and the RT descriptor set update of the frame, BEFORE
			 * any consumer. The barriers toward the consumers (fragment and compute readers) are
			 * recorded here. When the volume is disabled only the parameters are written (the
			 * shader-side flag), so a bound consumer reads a valid, inert set.
			 * @param commandBuffer The frame's command buffer (recording).
			 * @param frameIndex The frame in flight index.
			 * @param rtDescriptorSet The renderer's RT set of the frame (TLAS, mesh, material and light SSBOs).
			 * @param bindlessDescriptorSet The bindless texture set (alpha test, albedo, environment cubemap).
			 * @param inputs The scene inputs of the frame.
			 * @return void
			 */
			void recordUpdate (const Vulkan::CommandBuffer & commandBuffer, uint32_t frameIndex, const Vulkan::DescriptorSet & rtDescriptorSet, const Vulkan::DescriptorSet & bindlessDescriptorSet, const FrameInputs & inputs) noexcept;

		private:

			/** @brief The std140 mirror of the GLSL `IrradianceProbeParams` block (EMEN_IRRADIANCE_PROBES_GLSL). */
			struct ParametersUBO
			{
				std::array< float, 4 > originSpacing;
				std::array< int32_t, 4 > probeCounts;
				std::array< int32_t, 4 > scrollReset;
				std::array< int32_t, 4 > resetPlanes;
				std::array< float, 4 > biasHysteresis;
				std::array< float, 4 > skyAmbient;
				std::array< float, 4 > ambientColor;
				std::array< float, 4 > rotation0;
				std::array< float, 4 > rotation1;
				std::array< float, 4 > rotation2;
			};

			static_assert(sizeof(ParametersUBO) == 160, "The parameters block must stay 10 vec4 wide (std140 mirror).");

			/** @brief Reads the settings into m_parameters, sanitizing what the shaders assume. */
			void readSettings (Renderer & renderer) noexcept;

			/** @brief Creates the two atlases, the ray buffer and the per-frame parameter buffers. */
			[[nodiscard]]
			bool createResources (Renderer & renderer) noexcept;

			/** @brief Creates the layout, the pool and the per-frame descriptor sets. */
			[[nodiscard]]
			bool createDescriptorSets (Renderer & renderer) noexcept;

			/** @brief Compiles the five compute pipelines. */
			[[nodiscard]]
			bool createPipelines (Renderer & renderer) noexcept;

			/** @brief Scrolls the volume to the camera cell and fills the frame's parameters block. */
			void updateParameters (uint32_t frameIndex, const FrameInputs & inputs) noexcept;

			/** @brief First use: UNDEFINED -> GENERAL and a clear of both atlases. */
			void recordAtlasInitialization (const Vulkan::CommandBuffer & commandBuffer) noexcept;

			std::shared_ptr< Vulkan::Device > m_device;
			Parameters m_parameters;
			std::shared_ptr< Vulkan::Image > m_irradianceImage;
			std::shared_ptr< Vulkan::ImageView > m_irradianceView;
			std::shared_ptr< Vulkan::Image > m_distanceImage;
			std::shared_ptr< Vulkan::ImageView > m_distanceView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::unique_ptr< Vulkan::ShaderStorageBufferObject > m_rayBuffer;
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_parameterBuffers;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::shared_ptr< Vulkan::PipelineLayout > m_tracePipelineLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_updatePipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_tracePipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_blendIrradiancePipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_blendDistancePipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_borderIrradiancePipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_borderDistancePipeline;
			std::mt19937 m_random{0x9E3779B9U};
			std::array< int32_t, 3 > m_cameraCell{0, 0, 0};
			std::array< int32_t, 3 > m_scroll{0, 0, 0};
			std::array< int32_t, 3 > m_resetPlanes{-1, -1, -1};
			uint32_t m_frameCounter{0};
			bool m_resetAll{true};
			bool m_hasCameraCell{false};
			bool m_atlasesInitialized{false};
	};
}
