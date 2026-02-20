/*
 * src/Graphics/Effects/SSAO.hpp
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
#include "Graphics/PostProcessEffect.hpp"

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

namespace EmEn::Graphics::Effects
{
	/**
	 * @brief Screen-Space Ambient Occlusion (SSAO) post-processing effect.
	 * @note Computes ambient occlusion from the depth buffer using a hemisphere sampling approach,
	 * applies bilateral blur to reduce noise, then multiplies AO with the scene color.
	 * @extends EmEn::Graphics::PostProcessEffect This is a multi-pass post-process effect.
	 */
	class SSAO final : public PostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SSAOEffect"};

			/**
			 * @brief User-facing SSAO parameters.
			 */
			struct Parameters
			{
				float radius{0.5F};
				float intensity{1.5F};
				float bias{0.025F};
				uint32_t sampleCount{32};
			};

			/**
			 * @brief Push constants for the SSAO computation pass.
			 */
			struct SSAOPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float radius;
				float intensity;
				float bias;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				float aspectRatio;
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
			 * @brief Constructs an SSAO effect.
			 */
			SSAO () noexcept = default;

			/** @copydoc EmEn::Graphics::PostProcessEffect::create() */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::resize() */
			[[nodiscard]]
			bool resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (
				const Vulkan::CommandBuffer & commandBuffer,
				const Vulkan::TextureInterface & inputColor,
				const Vulkan::TextureInterface * inputDepth,
				const Vulkan::TextureInterface * inputNormals,
				const PostProcessor::PushConstants & constants
			) noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::requiresNormals() */
			[[nodiscard]]
			bool
			requiresNormals () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the SSAO parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current SSAO parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/* IRTs: AO computation (half-res), blur H (half-res), blur V (half-res), apply (full-res). */
			IntermediateRenderTarget m_aoTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			IntermediateRenderTarget m_outputTarget;

			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_aoPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_applyPipeline;

			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_aoLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_blurLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_applyLayout;

			/* Descriptor sets (fixed — never updated after creation). */
			std::unique_ptr< Vulkan::DescriptorSet > m_blurHDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_blurVDescSet;

			/* Per-frame-in-flight descriptor sets (updated every frame). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_aoPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_applyPerFrame;

			Renderer * m_renderer{nullptr};
			Parameters m_parameters;
	};
}
