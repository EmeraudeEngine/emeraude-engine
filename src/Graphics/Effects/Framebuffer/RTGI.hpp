/*
 * src/Graphics/Effects/Framebuffer/RTGI.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorSet;
		class DescriptorSetLayout;
		class GraphicsPipeline;
		class PipelineLayout;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Ray-Traced Global Illumination (RTGI) post-processing effect.
	 * @note One-bounce diffuse indirect lighting using GL_EXT_ray_query.
	 * For each pixel, casts hemisphere rays against the TLAS; on hit, samples
	 * the surface albedo and computes direct lighting at the hit point to produce
	 * indirect radiance (color bleeding). RT-only, no screen-space fallback.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class RTGI final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RTGIEffect"};

			/**
			 * @brief User-facing RTGI parameters.
			 */
			struct Parameters
			{
				float maxDistance{5.0F};
				float intensity{1.5F};
				float bias{0.02F};
				uint32_t sampleCount{4};
			};

			/**
			 * @brief Push constants for the RTGI trace pass.
			 */
			struct TracePushConstants
			{
				float invViewProj[16];
				float invViewCol0[3];
				float viewPosX;
				float invViewCol1[3];
				float viewPosY;
				float invViewCol2[3];
				float viewPosZ;
				float maxDistance;
				float intensity;
				float bias;
				uint32_t sampleCount;
			};

			/**
			 * @brief Push constants for the blur pass.
			 */
			struct BlurPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float directionX;
				float directionY;
			};

			/**
			 * @brief Push constants for the apply pass.
			 */
			struct ApplyPushConstants
			{
				float intensity;
				float padding1;
				float padding2;
				float padding3;
			};

			/**
			 * @brief Constructs an RTGI effect.
			 */
			RTGI () noexcept = default;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (
				const Vulkan::CommandBuffer & commandBuffer,
				const Vulkan::TextureInterface & inputColor,
				const Vulkan::TextureInterface * inputDepth,
				const Vulkan::TextureInterface * inputNormals,
				const Vulkan::TextureInterface * inputMaterialProperties,
				const PostProcessor::PushConstants & constants
			) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresNormals() */
			[[nodiscard]]
			bool
			requiresNormals () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties() */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresRayTracing() */
			[[nodiscard]]
			bool
			requiresRayTracing () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the RTGI parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current RTGI parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/* IRTs: trace (half-res), blur H (half-res), blur V (half-res), apply (full-res). */
			IntermediateRenderTarget m_traceTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			IntermediateRenderTarget m_outputTarget;

			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tracePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_applyPipeline;

			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_traceLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_blurLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_applyLayout;

			/* Descriptor sets (fixed -- never updated after creation). */
			std::unique_ptr< Vulkan::DescriptorSet > m_blurHDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_blurVDescSet;

			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_applyPerFrame;

			Renderer * m_renderer{nullptr};
			Parameters m_parameters;
	};
}
