/*
 * src/Graphics/Effects/Framebuffer/TAA.hpp
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

/* STL inclusions. */
#include <array>
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
	 * @brief Temporal Anti-Aliasing (TAA) post-processing effect.
	 * @note Single-pass HDR resolve, placed BEFORE DepthOfField/ToneMapping (the photographic
	 * effects receive a stabilized image). Accumulates the sub-pixel projection jitter
	 * (Halton (2,3), driven by requiresJitter()) into a full-resolution RGBA16F ping-pong
	 * history, reprojected through the velocity G-buffer with 3x3 depth-nearest dilation.
	 * History rectification uses variance clipping in YCoCg space; the HDR blend uses the
	 * Karis inverse-luminance weighting (anti-firefly, anti-flicker). The resolve output IS
	 * the new history image (zero-copy feedback).
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class EMEN_API TAA final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TAAEffect"};

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing TAA parameters.
			 * @note Overridden by the Core/Graphics/AntiAliasing/Temporal settings keys at create().
			 */
			struct EMEN_API Parameters
			{
				float alpha{0.1F}; /**< Blend weight of the current frame (0.1 = 90% history). */
				float varianceGamma{1.0F}; /**< Variance clipping AABB half-size, in standard deviations. */
				bool lumaWeighting{true}; /**< Karis inverse-luminance HDR blend weighting. */
			};

			/**
			 * @brief Constructs a temporal anti-aliasing effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			TAA (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresVelocity() */
			[[nodiscard]]
			bool
			requiresVelocity () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresJitter() */
			[[nodiscard]]
			bool
			requiresJitter () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the TAA parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current TAA parameters.
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
			/** @brief Full-resolution RGBA16F ping-pong: the resolve writes one while reading the other; the written image is both the effect output and the next frame's history. */
			std::array< IntermediateRenderTarget, 2 > m_historyTargets;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			uint32_t m_historyWriteIndex{0};
			/** @brief False until the first resolve after (re)creation: forces alpha to 1 so the uninitialized history is never read. */
			bool m_historyValid{false};
	};
}
