/*
 * src/Graphics/Effects/Framebuffer/DepthOfField.hpp
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
	 * @brief Depth of Field post-processing effect.
	 * @note Computes Circle of Confusion from depth, applies a separable blur weighted by CoC,
	 * then composites sharp and blurred regions.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class DepthOfField final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DepthOfFieldEffect"};

			/**
			 * @brief User-facing depth of field parameters.
			 */
			struct Parameters
			{
				float focusDistance{10.0F};
				float aperture{2.8F};
				float focalLength{50.0F};
				float cocScale{10.0F};
				bool autoFocus{true};
			};

			/**
			 * @brief Push constants for the CoC computation pass.
			 */
			struct CoCPushConstants
			{
				float nearPlane;
				float farPlane;
				float focusDistance;
				float aperture;
				float focalLength;
				float cocScale;
				float texelSizeX;
				float texelSizeY;
				uint32_t autoFocus;
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
			 * @brief Push constants for the composite pass.
			 */
			struct CompositePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float padding1;
				float padding2;
			};

			/**
			 * @brief Constructs a depth of field effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			DepthOfField (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a depth of field effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			DepthOfField (Renderer & renderer, const Parameters & parameters) noexcept
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

			/**
			 * @brief Sets the depth of field parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current depth of field parameters.
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
			/* IRTs: CoC (half-res R16), blur H (half-res), blur V (half-res), composite (full-res). */
			IntermediateRenderTarget m_cocTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_cocPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_cocLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_blurLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;
			/* Descriptor sets.
			 * NOTE: m_cocPerFrame and m_compositePerFrame are updated every frame
			 * in execute(), so they need per-frame-in-flight copies to avoid updating
			 * a descriptor set still referenced by a pending command buffer. */
			std::unique_ptr< Vulkan::DescriptorSet > m_blurHDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_blurVDescSet;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_cocPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
	};
}
