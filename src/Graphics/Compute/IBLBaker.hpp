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
#include <memory>

namespace EmEn::Vulkan
{
	class CommandBuffer;
	class CommandPool;
	class ComputePipeline;
	class DescriptorSetLayout;
	class Device;
	class PipelineLayout;
	class ShaderModule;
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
	 * mapping, source sampled with cubemap-space directions). Consumers keep applying the
	 * engine world-to-cubemap convention `vec3(D.x, -D.y, D.z)` — see docs/caution-points.md.
	 */
	class EMEN_API IBLBaker final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GraphicsComputeIBLBaker"};

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
			bool generateBRDFLut (IBLTexture & lut) const noexcept;

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
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool bakeEnvironment (const Vulkan::TextureInterface & source, IBLTexture & irradiance, IBLTexture & prefiltered) noexcept;

		private:

			/**
			 * @brief Creates (once) the cached environment-bake pipelines and command objects.
			 * @return bool True when everything is ready.
			 */
			[[nodiscard]]
			bool ensureEnvironmentPipelines () noexcept;

#ifdef EMERAUDE_DEBUG_IBL_FACES
			/**
			 * @brief Debug: reads back every mip of a baked texture and writes tonemapped
			 * PNG faces to /tmp (same pattern as EMERAUDE_DEBUG_HDR_FACES).
			 * @param texture A reference to the baked texture.
			 * @param label The file name label.
			 * @return void
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
	};
}
