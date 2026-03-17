/*
 * src/Graphics/Effects/Framebuffer/Sharpen.hpp
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
	 * @brief Contrast Adaptive Sharpening (CAS) post-processing effect.
	 * @note Single-pass, LDR. Placed after ToneMapping to sharpen the final image.
	 * Uses a 5-tap cross pattern with adaptive weighting to sharpen low-contrast
	 * areas while avoiding ringing in high-contrast edges. Inspired by AMD FidelityFX CAS.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class Sharpen final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SharpenEffect"};

			/**
			 * @brief User-facing sharpening parameters.
			 */
			struct Parameters
			{
				float sharpness{0.5F};   /**< 0.0 = off, 0.5 = moderate, 1.0+ = strong */
			};

			/**
			 * @brief Constructs a sharpen effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			Sharpen (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a sharpen effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			Sharpen (Renderer & renderer, const Parameters & parameters) noexcept
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

			/**
			 * @brief Sets the sharpening parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current sharpening parameters.
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
			IntermediateRenderTarget m_outputTarget;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
	};
}
