/*
 * src/Graphics/Effects/Bloom.hpp
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
#include <array>
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
	 * @brief HDR Bloom post-processing effect using a multi-pass Dual Kawase approach.
	 * @note The algorithm performs a progressive downsample (13-tap) with brightness threshold,
	 * followed by an upsample (tent filter) with additive blending, and a final composite.
	 * @extends EmEn::Graphics::PostProcessEffect This is a multi-pass post-process effect.
	 */
	class Bloom final : public PostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BloomEffect"};

			/** @brief Number of mip levels in the downsample/upsample chain. */
			static constexpr int MipLevels = 5;

			/**
			 * @brief User-facing bloom parameters.
			 */
			struct Parameters
			{
				float threshold{1.0F};
				float softKnee{0.5F};
				float intensity{1.0F};
				float spread{1.0F};
			};

			/**
			 * @brief Push constants sent to all bloom shader passes.
			 */
			struct BloomPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float threshold;
				float softKnee;
				float intensity;
				float spread;
			};

			/**
			 * @brief Constructs a bloom effect.
			 */
			Bloom () noexcept = default;

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

			/** @copydoc EmEn::Graphics::PostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the bloom parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current bloom parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/**
			 * @brief Creates the shared descriptor set layout for single-input passes.
			 * @param renderer A reference to the graphics renderer.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getSingleInputLayout (Renderer & renderer) noexcept;

			/**
			 * @brief Creates the shared descriptor set layout for dual-input passes.
			 * @param renderer A reference to the graphics renderer.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDualInputLayout (Renderer & renderer) noexcept;

			/**
			 * @brief Creates all graphics pipelines for the bloom passes.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipelines (Renderer & renderer) noexcept;

			/**
			 * @brief Creates all descriptor sets for the bloom passes.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSets (Renderer & renderer) noexcept;

			/**
			 * @brief Records a single fullscreen pass into a target IRT.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param target A reference to the target intermediate render target.
			 * @param pipeline A reference to the graphics pipeline.
			 * @param pipelineLayout A reference to the pipeline layout.
			 * @param descriptorSet A reference to the descriptor set.
			 * @param pc A reference to the push constants.
			 * @return void
			 */
			void recordFullscreenPass (
				const Vulkan::CommandBuffer & commandBuffer,
				IntermediateRenderTarget & target,
				const Vulkan::GraphicsPipeline & pipeline,
				const Vulkan::PipelineLayout & pipelineLayout,
				const Vulkan::DescriptorSet & descriptorSet,
				const BloomPushConstants & pc
			) const noexcept;

			/* Intermediate render targets. */
			std::array< IntermediateRenderTarget, MipLevels > m_downTargets;
			std::array< IntermediateRenderTarget, MipLevels - 1 > m_upTargets;
			IntermediateRenderTarget m_outputTarget;

			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_downsamplePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_upsamplePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;

			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_downsampleLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_upsampleLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;

			/* Descriptor sets.
			 * NOTE: m_downDescSets[0] and m_compositeDescSet are updated every frame
			 * in execute(), so they need per-frame-in-flight copies to avoid updating
			 * a descriptor set still referenced by a pending command buffer. */
			std::array< std::unique_ptr< Vulkan::DescriptorSet >, MipLevels > m_downDescSets;
			std::array< std::unique_ptr< Vulkan::DescriptorSet >, MipLevels - 1 > m_upDescSets;
			std::unique_ptr< Vulkan::DescriptorSet > m_compositeDescSet;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_downFirstPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;

			/* Parameters. */
			Parameters m_parameters;

			Renderer * m_renderer{nullptr};
	};
}
