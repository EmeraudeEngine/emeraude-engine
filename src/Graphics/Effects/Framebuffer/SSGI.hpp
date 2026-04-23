/*
 * src/Graphics/Effects/Framebuffer/SSGI.hpp
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


namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Screen-Space Global Illumination (SSGI) post-processing effect.
	 * @note One-bounce diffuse indirect lighting approximation using screen-space ray marching.
	 * For each pixel, casts cosine-weighted hemisphere rays through the depth buffer;
	 * on hit, samples the scene color at the hit point to produce indirect radiance
	 * (color bleeding). This is the screen-space fallback for RTGI when ray tracing
	 * hardware is not available.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class SSGI final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SSGIEffect"};

			/**
			 * @brief User-facing SSGI parameters.
			 */
			struct Parameters
			{
				float maxDistance{5.0F};
				float intensity{0.8F};
				float thickness{0.5F};
				uint32_t sampleCount{8};
				uint32_t stepCount{16};
				uint32_t blurRadius{4};
				float depthSigma{1.0F};
				float normalSigma{0.5F};
			};

			/**
			 * @brief Push constants for the SSGI trace pass.
			 */
			struct TracePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				float aspectRatio;
				float maxDistance;
				float thickness;
				uint32_t sampleCount;
				uint32_t stepCount;
			};

			/**
			 * @brief Push constants for the bilateral blur pass.
			 */
			struct BlurPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float directionX;
				float directionY;
				float depthSigma;
				float normalSigma;
				int32_t blurRadius;
				float padding;
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
			 * @brief Constructs a screen-space global illumination effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			SSGI (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a screen-space global illumination effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			SSGI (Renderer & renderer, const Parameters & parameters) noexcept
				: IndirectPostProcessEffect{renderer},
				m_parameters{parameters}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const Vulkan::TextureInterface * inputDepth, const Vulkan::TextureInterface * inputNormals, const Vulkan::TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept override;

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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the SSGI parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current SSGI parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			Parameters m_parameters;
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
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurHPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurVPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_applyPerFrame;
	};
}
