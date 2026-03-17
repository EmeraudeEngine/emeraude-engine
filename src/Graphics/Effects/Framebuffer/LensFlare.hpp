/*
 * src/Graphics/Effects/Framebuffer/LensFlare.hpp
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
	 * @brief Light-aware lens flare post-processing effect.
	 * @note Projects a dominant light source to screen-space, then generates
	 * ghost copies along the light-to-center axis with chromatic distortion,
	 * adds a halo ring around the light position, and composites additively
	 * with the scene.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class LensFlare final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LensFlareEffect"};

			/**
			 * @brief User-facing lens flare parameters.
			 */
			struct Parameters
			{
				float threshold{0.8F};
				float softKnee{0.5F};
				int32_t ghostCount{4};
				float ghostSpacing{0.3F};
				float haloRadius{0.6F};
				float haloThickness{0.1F};
				float chromaticDistortion{0.02F};
				float intensity{1.0F};
			};

			/**
			 * @brief Push constants for the threshold pass.
			 */
			struct ThresholdPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float threshold;
				float softKnee;
			};

			/**
			 * @brief Push constants for the ghost + halo pass.
			 */
			struct GhostHaloPushConstants
			{
				float lightScreenX;
				float lightScreenY;
				float ghostSpacing;
				float haloRadius;
				float haloThickness;
				float chromaticDistortion;
				float intensity;
				int32_t ghostCount;
			};

			/**
			 * @brief Push constants for the composite pass.
			 */
			struct CompositePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float lightOnScreen;
				float padding;
			};

			/**
			 * @brief Constructs a lens flare effect.
			 */
			LensFlare () noexcept = default;

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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the dominant light emission direction.
			 * @param x The X component of the emission direction.
			 * @param y The Y component of the emission direction.
			 * @param z The Z component of the emission direction.
			 * @return void
			 */
			void
			setLightDirection (float x, float y, float z) noexcept
			{
				m_lightDirX = x;
				m_lightDirY = y;
				m_lightDirZ = z;
			}

			/**
			 * @brief Sets the lens flare parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current lens flare parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/* Intermediate render targets. */
			IntermediateRenderTarget m_thresholdTarget;
			IntermediateRenderTarget m_ghostHaloTarget;
			IntermediateRenderTarget m_outputTarget;

			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_thresholdPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_ghostHaloPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;

			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_thresholdLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_ghostHaloLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;

			/* Descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_thresholdPerFrame;
			std::unique_ptr< Vulkan::DescriptorSet > m_ghostHaloDescSet;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;

			/* Light direction (emission direction, will be negated for projection). */
			float m_lightDirX{0.0F};
			float m_lightDirY{-1.0F};
			float m_lightDirZ{0.0F};

			/* Parameters. */
			Parameters m_parameters;
			Renderer * m_renderer{nullptr};
	};
}
