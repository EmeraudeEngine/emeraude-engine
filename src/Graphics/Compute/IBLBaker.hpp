/*
 * src/Graphics/Compute/IBLBaker.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for usages. */
#include "Mutex.hpp"

namespace EmEn::Vulkan
{
	class CommandBuffer;
	class CommandPool;
	class ComputePipeline;
	class DescriptorPool;
	class DescriptorSet;
	class DescriptorSetLayout;
	class Device;
	class PipelineLayout;
	class TextureInterface;
}

namespace EmEn::Saphir
{
	class ShaderManager;
}

namespace EmEn::Graphics
{
	class IBLTexture;
}

namespace EmEn::Graphics::Compute
{
	/**
	 * @brief GPU generator for the image-based lighting (IBL) assets.
	 * @note Bakes the split-sum BRDF LUT once at boot, and per-environment assets (diffuse
	 * irradiance cubemap + GGX-prefiltered specular cubemap) every time a scene adopts a new
	 * environment cubemap. The environment pipelines are cached across bakes.
	 * All work is submitted on the GRAPHICS queue: the produced images are sampled by
	 * fragment shaders on that same queue, which avoids a queue-family ownership transfer
	 * on an EXCLUSIVE image (graphics queues always support compute dispatches).
	 * @warning Convention: the baker works entirely in CUBEMAP space (identity face
	 * mapping, source sampled with cubemap-space directions) — which IS world space since the
	 * Y-up flip: the former world-to-cubemap `vec3(D.x, -D.y, D.z)` compensation is dead, see
	 * docs/caution-points.md. A StarMask direction is therefore a plain world direction.
	 */
	class EMEN_API IBLBaker final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GraphicsComputeIBLBaker"};

			/** @brief The cube-face convention of every compute pass that writes a cubemap through a 2D-array view: the
			 * standard face layout (+X, −X, +Y, −Y, +Z, −Z), st in [−1, 1], CUBEMAP space (= world space). One source
			 * for the bake and the dynamic sky's capture (Graphics::SkyEnvironment). */
			static constexpr auto FaceDirectionGLSL{R"GLSL(
/* Standard cube face layout (+X,-X,+Y,-Y,+Z,-Z), st in [-1,1]. CUBEMAP space. */
vec3 faceDirection (uint face, vec2 st)
{
	switch ( face )
	{
		case 0u: return vec3( 1.0, -st.y, -st.x);
		case 1u: return vec3(-1.0, -st.y,  st.x);
		case 2u: return vec3( st.x,  1.0,  st.y);
		case 3u: return vec3( st.x, -1.0, -st.y);
		case 4u: return vec3( st.x, -st.y,  1.0);
		default: return vec3(-st.x, -st.y, -1.0);
	}
}
)GLSL"};

			/**
			 * @brief Constructs an IBL baker.
			 * @note Out-of-line (with the destructor): the cached Vulkan members are
			 * forward-declared, their lifetime code must not instantiate here.
			 * @param device The Vulkan device.
			 * @param shaderManager The shader compilation manager.
			 */
			IBLBaker (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept;

			/** @brief Non-copyable. */
			IBLBaker (const IBLBaker &) = delete;

			IBLBaker & operator= (const IBLBaker &) = delete;

			IBLBaker (IBLBaker &&) = delete;

			IBLBaker & operator= (IBLBaker &&) = delete;

			/**
			 * @brief Destructs the IBL baker.
			 * @note Out-of-line: the cached Vulkan objects are forward-declared here.
			 */
			~IBLBaker ();

			/**
			 * @brief Bakes the split-sum BRDF LUT into the texture (blocking, one-shot).
			 * @note The texture image must be created (IBLTexture::create()) with the BRDFLut
			 * role. On success the image is left in SHADER_READ_ONLY_OPTIMAL layout, ready to
			 * be published to the bindless table.
			 * @param lut A reference to the destination texture.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool generateBRDFLut (const IBLTexture & lut) const;

			/**
			 * @brief A celestial body to keep OUT of an environment bake.
			 * @note A body declared IN the source texture (a sky manifest star with "InTexture")
			 * already lights the scene through the analytic directional light derived from it —
			 * WITH shadows. Left in the bake, the same body would light every surface a second
			 * time, unshadowed, from the texture: on Kloppenheim 05 the in-texture sun is 80 % of
			 * the illuminance the whole sky pours on the ground. The bake reads the sky at the rim
			 * of the cone instead (see the GLSL `maskStar()`); the visible skybox keeps the body.
			 * A default-constructed mask is disabled.
			 */
			struct EMEN_API StarMask
			{
				/** @brief Direction toward the body, cubemap (= world) space, normalized. */
				std::array< float, 3 > direction{0.0F, 0.0F, 0.0F};

				/** @brief Half-angle of the masked cone, in radians. 0 = no mask. */
				float halfAngleRadians{0.0F};

				[[nodiscard]]
				bool
				enabled () const noexcept
				{
					return halfAngleRadians > 0.0F;
				}

				bool operator== (const StarMask & other) const noexcept = default;
			};

			/**
			 * @brief Bakes the per-environment IBL assets from a source environment cubemap
			 * (blocking: submits on the graphics queue and waits for completion — a few
			 * hundred microseconds of GPU work, acceptable at the sky-change rate).
			 * @note The source must be a created cubemap texture carrying its full mip chain
			 * (the filtered importance sampling reads source mips by solid-angle ratio). On
			 * success both destination images are left in SHADER_READ_ONLY_OPTIMAL layout.
			 * @param source A reference to the source environment cubemap texture.
			 * @param irradiance A reference to the destination texture (IrradianceCubemap role).
			 * @param prefiltered A reference to the destination texture (PrefilteredCubemap role).
			 * @param starMask The in-texture celestial body to keep out of both bakes (a default-constructed mask keeps everything).
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool bakeEnvironment (const Vulkan::TextureInterface & source, IBLTexture & irradiance, IBLTexture & prefiltered, const StarMask & starMask) noexcept;

			/**
			 * @brief The descriptor sets binding ONE source cubemap to ONE (irradiance, prefiltered) pair: one set per
			 * dispatch (each prefilter mip, then the irradiance).
			 * @note Created once by createEnvironmentBindings() and reused by every bake of that pairing: the blocking
			 * bake builds a transient one, the amortised dynamic-sky bake (Graphics::SkyEnvironment) keeps one per ring
			 * set and records with it frame after frame. The sets free themselves into their pool (FREE_DESCRIPTOR_SET).
			 */
			class EMEN_API EnvironmentBindings final
			{
				public:

					/**
					 * @brief Constructs the bindings.
					 * @param pool The pool the sets were allocated from.
					 * @param sets The sets, the prefilter mips first, the irradiance last.
					 */
					EnvironmentBindings (std::shared_ptr< Vulkan::DescriptorPool > pool, std::vector< std::unique_ptr< Vulkan::DescriptorSet > > sets) noexcept;

					EnvironmentBindings (const EnvironmentBindings &) = delete;

					EnvironmentBindings & operator= (const EnvironmentBindings &) = delete;

					EnvironmentBindings (EnvironmentBindings &&) = delete;

					EnvironmentBindings & operator= (EnvironmentBindings &&) = delete;

					/**
					 * @brief Destructs the bindings (the sets first, then the pool).
					 */
					~EnvironmentBindings ();

					/**
					 * @brief Returns the set of a prefilter mip.
					 * @pre mipLevel < prefilterMipCount().
					 * @param mipLevel The prefiltered mip level.
					 * @return const Vulkan::DescriptorSet &
					 */
					[[nodiscard]]
					const Vulkan::DescriptorSet & prefilter (uint32_t mipLevel) const noexcept;

					/**
					 * @brief Returns the set of the irradiance dispatch.
					 * @return const Vulkan::DescriptorSet &
					 */
					[[nodiscard]]
					const Vulkan::DescriptorSet & irradiance () const noexcept;

					/**
					 * @brief Returns the number of prefilter mips the bindings cover.
					 * @return uint32_t
					 */
					[[nodiscard]]
					uint32_t
					prefilterMipCount () const noexcept
					{
						return m_sets.empty() ? 0U : static_cast< uint32_t >(m_sets.size() - 1);
					}

				private:

					std::shared_ptr< Vulkan::DescriptorPool > m_pool;
					std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_sets;
			};

			/**
			 * @brief Creates the descriptor sets binding a source cubemap to an (irradiance, prefiltered) pair.
			 * @note Ensures the environment pipelines first. Cold path: validates everything, refuses with a trace.
			 * @param source A reference to the source environment cubemap (created, with its full mip chain).
			 * @param irradiance A reference to the destination (IrradianceCubemap role, created).
			 * @param prefiltered A reference to the destination (PrefilteredCubemap role, created).
			 * @return std::unique_ptr< EnvironmentBindings > Null on failure.
			 */
			[[nodiscard]]
			std::unique_ptr< EnvironmentBindings > createEnvironmentBindings (const Vulkan::TextureInterface & source, const IBLTexture & irradiance, const IBLTexture & prefiltered) noexcept;

			/**
			 * @brief Records the GGX prefilter of ONE mip into a command buffer: the mip is discarded and opened for the
			 * compute write, written, then left in SHADER_READ_ONLY_OPTIMAL for the fragment and compute readers.
			 * @note The source must be in SHADER_READ_ONLY_OPTIMAL. The first barrier waits for every earlier read of the
			 * mip on the queue, in any stage (a write-after-read execution dependency), so a ring set can be re-baked in the
			 * frame command buffer; the last one makes it visible to every later stage (the materials, the compute and the
			 * ray-tracing effects). Roughness mip / (mips − 1), as the materials sample it.
			 * @pre The bindings come from createEnvironmentBindings() with this prefiltered texture; mipLevel <
			 * prefiltered.mipLevels().
			 * @param commandBuffer A reference to the recording command buffer.
			 * @param bindings A reference to the bindings.
			 * @param mipLevel The mip to write.
			 * @param prefiltered A reference to the destination.
			 * @param sourceSize The source's face size at mip 0, in texels.
			 * @param starMask The celestial body to keep out (default: none).
			 */
			void recordPrefilterMip (const Vulkan::CommandBuffer & commandBuffer, const EnvironmentBindings & bindings, uint32_t mipLevel, const IBLTexture & prefiltered, uint32_t sourceSize, const StarMask & starMask) const noexcept;

			/**
			 * @brief An image barrier on mip levels of a cubemap (all six faces): named fields, never swapped.
			 */
			struct CubeBarrier
			{
				/** @brief The layout before. */
				VkImageLayout oldLayout{VK_IMAGE_LAYOUT_UNDEFINED};
				/** @brief The layout after. */
				VkImageLayout newLayout{VK_IMAGE_LAYOUT_UNDEFINED};
				/** @brief The accesses to make available. */
				VkAccessFlags srcAccess{0};
				/** @brief The accesses to make visible. */
				VkAccessFlags dstAccess{0};
				/** @brief The stages to wait for. */
				VkPipelineStageFlags srcStage{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
				/** @brief The stages that wait. */
				VkPipelineStageFlags dstStage{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
				/** @brief The first mip level. */
				uint32_t baseMip{0};
				/** @brief The number of mip levels. */
				uint32_t mipCount{1};
			};

			/**
			 * @brief Records an image barrier on mip levels of a cubemap (all six faces).
			 * @note Public: the dynamic sky's capture (Graphics::SkyEnvironment) moves its cubemap's mips with it too.
			 * @param commandBuffer A reference to the recording command buffer.
			 * @param image The image.
			 * @param barrier The barrier.
			 */
			static void recordCubeBarrier (const Vulkan::CommandBuffer & commandBuffer, VkImage image, const CubeBarrier & barrier) noexcept;

			/**
			 * @brief Records the irradiance convolution into a command buffer, with the same barriers as
			 * recordPrefilterMip().
			 * @pre The bindings come from createEnvironmentBindings() with this irradiance texture.
			 * @param commandBuffer A reference to the recording command buffer.
			 * @param bindings A reference to the bindings.
			 * @param irradiance A reference to the destination.
			 * @param sourceSize The source's face size at mip 0, in texels.
			 * @param starMask The celestial body to keep out (default: none).
			 */
			void recordIrradiance (const Vulkan::CommandBuffer & commandBuffer, const EnvironmentBindings & bindings, const IBLTexture & irradiance, uint32_t sourceSize, const StarMask & starMask) const noexcept;

			/**
			 * @brief Push constant block of the environment pipelines (prefilter, irradiance).
			 * @note Public: the probe convolver (Compute::ProbeConvolver) records the SAME
			 * prefilter pipeline into the frame command buffer and must push this block.
			 */
			struct EMEN_API EnvironmentPushConstants
			{
				uint32_t sourceSize;
				uint32_t destSize;
				uint32_t sampleCount;
				float roughness;
				/* GLSL `vec4 starMask` at offset 16: direction toward the masked body (xyz) and
				 * the cone half-angle in radians (w, <= 0 = no mask — so a zero-initialized block,
				 * which is what the probe convolver pushes, masks nothing). */
				float starMaskX;
				float starMaskY;
				float starMaskZ;
				float starMaskHalfAngle;
			};

			/**
			 * @brief Creates (once) the cached environment-bake pipelines and command objects.
			 * @note Public: also called by Compute::ProbeConvolver before borrowing the
			 * prefilter pipeline (see the getters below). Thread-safe: the logic thread (a picture sky's bake) and the
			 * render thread (a probe, the dynamic sky) both reach it; once created, the pipelines are immutable.
			 * @return bool True when everything is ready.
			 */
			[[nodiscard]]
			bool ensureEnvironmentPipelines ();

			/**
			 * @brief Returns the environment descriptor set layout (source cube + dest faces), or nullptr.
			 * @return const std::shared_ptr< Vulkan::DescriptorSetLayout > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::DescriptorSetLayout > &
			environmentDSLayout () const noexcept
			{
				return m_environmentDSLayout;
			}

			/**
			 * @brief Returns the environment pipeline layout, or nullptr.
			 * @return const std::shared_ptr< Vulkan::PipelineLayout > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::PipelineLayout > &
			environmentPipelineLayout () const noexcept
			{
				return m_environmentPipelineLayout;
			}

			/**
			 * @brief Returns the GGX prefilter compute pipeline, or nullptr.
			 * @return const Vulkan::ComputePipeline *
			 */
			[[nodiscard]]
			const Vulkan::ComputePipeline *
			prefilterPipeline () const noexcept
			{
				return m_prefilterPipeline.get();
			}

		private:

#ifdef EMERAUDE_DEBUG_IBL_FACES
			/**
			 * @brief Debug: reads back every mip of a baked texture and writes tonemapped
			 * PNG faces to /tmp (same pattern as EMERAUDE_DEBUG_HDR_FACES).
			 * @param texture A reference to the baked texture.
			 * @param label The file name label.
			 */
			void dumpTextureFaces (const IBLTexture & texture, const char * label) const noexcept;
#endif

			std::shared_ptr< Vulkan::Device > m_device;
			Saphir::ShaderManager * m_shaderManager;

			/* Cached across environment bakes. */
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_environmentDSLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_environmentPipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_prefilterPipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_irradiancePipeline;
			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::unique_ptr< Vulkan::CommandBuffer > m_commandBuffer;
			/** @brief Guards the creation of the environment pipelines (ensureEnvironmentPipelines()). */
			mutable Base::Mutex m_pipelinesAccess;
	};
}
