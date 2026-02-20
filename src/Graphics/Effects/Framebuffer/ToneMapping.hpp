/*
 * src/Graphics/Effects/Framebuffer/ToneMapping.hpp
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
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorSet;
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
	 * @brief Tone mapping post-processing effect that converts HDR to LDR.
	 * @note Supports multiple tone mapping operators: ACES Filmic, Reinhard, and Uncharted 2.
	 * Includes optional auto-exposure (eye adaptation) via luminance measurement and
	 * temporal exponential moving average.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class ToneMapping final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ToneMappingEffect"};

			/**
			 * @brief Available tone mapping operators.
			 */
			enum class Operator : uint32_t
			{
				ACESFilmic = 0,
				Reinhard = 1,
				Uncharted2 = 2
			};

			/**
			 * @brief User-facing tone mapping parameters.
			 */
			struct Parameters
			{
				Operator tonemapOperator{Operator::ACESFilmic};
				float exposure{1.0F};
				float gamma{2.2F};
				bool autoExposureEnabled{true};
				float keyValue{0.18F};
				float adaptSpeedUp{1.5F};
				float adaptSpeedDown{3.0F};
				float minExposure{0.1F};
				float maxExposure{10.0F};
			};

			/**
			 * @brief Push constants sent to the tone mapping shader (no auto-exposure).
			 */
			struct ToneMappingPushConstants
			{
				float exposure;
				float gamma;
				uint32_t tonemapOperator;
				float padding;
			};

			/**
			 * @brief Constructs a tone mapping effect.
			 */
			ToneMapping () noexcept = default;

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
				const PostProcessor::PushConstants & constants
			) noexcept override;

			/**
			 * @brief Sets the tone mapping parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current tone mapping parameters.
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
			 * @brief Creates all graphics pipelines.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipelines (Renderer & renderer) noexcept;

			/**
			 * @brief Creates all descriptor sets.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSets (Renderer & renderer) noexcept;

			Renderer * m_renderer{nullptr};
			Parameters m_parameters;

			/* Standard tone mapping output (LDR). */
			IntermediateRenderTarget m_outputTarget;

			/* Standard tone mapping pipeline (no auto-exposure). */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;

			/* Auto-exposure: luminance downsample chain (half-res -> 1x1, R16G16B16A16_SFLOAT). */
			std::vector< std::unique_ptr< IntermediateRenderTarget > > m_lumTargets;

			/* Auto-exposure: adaptation ping-pong (two 1x1 R16G16B16A16_SFLOAT targets). */
			std::array< IntermediateRenderTarget, 2 > m_adaptTargets;
			uint32_t m_currentAdaptIndex{0};

			/* Auto-exposure: pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_lumExtractPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_lumDownsamplePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_adaptPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_autoExposurePipeline;

			/* Auto-exposure: pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_adaptPipelineLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_autoExpPipelineLayout;

			/* Auto-exposure: descriptor sets.
			 * m_lumDownDescSets are fixed (internal chain).
			 * Per-frame sets are updated each frame to handle external input and ping-pong. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_lumDownDescSets;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_adaptPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_autoExpDescPerFrame;

			/* Auto-exposure: time tracking for deltaTime computation. */
			float m_previousTime{0.0F};
			bool m_firstFrame{true};
	};
}
