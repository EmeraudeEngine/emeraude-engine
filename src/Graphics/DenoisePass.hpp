/*
 * src/Graphics/DenoisePass.hpp
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
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

/* Local inclusions for inheritances. */
#include "IndirectPostProcessEffect.hpp"

namespace EmEn::Vulkan
{
	class Framebuffer;
	class RenderPass;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The shared separable-blur pass of the overlay effects (phase E of the
	 * pass-merging plan).
	 * @note Owned by the PostProcessor. The overlay effects whose working chain is
	 * "trace → blur H → blur V" (reflections, AO, GI, contact shadows) delegate the blur
	 * pair: the whole combine group runs it as TWO multi-render-target passes — one
	 * horizontal, one vertical — instead of two passes per effect. Each effect keeps its
	 * EXACT kernel through the GLSL snippet of its DenoiseContribution; the H/V targets
	 * stay effect-owned (mixed formats allowed, same extent required across the group).
	 * The generated shader and its pipeline are cached per group SIGNATURE; the
	 * framebuffers are rebuilt when the attachment set changes (effect resize).
	 * @extends EmEn::Graphics::IndirectPostProcessEffect Reuses the shared fullscreen
	 * pass infrastructure — it is never inserted into a stack itself.
	 */
	class EMEN_API DenoisePass final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DenoisePass"};

			/** @brief One overlay effect's denoise contribution, gathered by the PostProcessor. */
			using GroupEntry = DenoiseContribution;

			/**
			 * @brief Constructs the denoise pass.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			DenoisePass (Renderer & renderer) noexcept;

			/**
			 * @brief Destructs the denoise pass.
			 */
			~DenoisePass () override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool
			create (uint32_t /*width*/, uint32_t /*height*/) noexcept override
			{
				/* The pass owns no target: everything is per-signature, created lazily. */
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/**
			 * @brief Records the TWO shared blur passes (H then V) for a group of contributions.
			 * @note Called outside any active render pass, between the group's pre-denoise
			 * (trace) and post-denoise (temporal) passes. On failure the group's blur targets
			 * are left untouched (traced error) — the combine then reads stale/undefined data,
			 * which is a programming error, not a runtime condition.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param contributions The ordered contributions of the group's effects.
			 * @param context The per-frame chain context.
			 * @return bool
			 */
			[[nodiscard]]
			bool record (const Vulkan::CommandBuffer & commandBuffer, const std::vector< GroupEntry > & contributions, const FrameContext & context) noexcept;

		private:

			/** @brief GPU objects for one group signature. */
			struct Variant
			{
				std::shared_ptr< Vulkan::RenderPass > renderPass;
				std::shared_ptr< Vulkan::GraphicsPipeline > pipeline;
				std::shared_ptr< Vulkan::PipelineLayout > pipelineLayout;
				std::vector< std::unique_ptr< Vulkan::DescriptorSet > > descriptorSetsHPerFrame;
				std::vector< std::unique_ptr< Vulkan::DescriptorSet > > descriptorSetsVPerFrame;
				std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > uniformBuffersPerFrame;
				/* Framebuffers rebuilt when the attachment image views change (resize). */
				std::unique_ptr< Vulkan::Framebuffer > framebufferH;
				std::unique_ptr< Vulkan::Framebuffer > framebufferV;
				std::vector< VkImageView > attachmentViewsH;
				std::vector< VkImageView > attachmentViewsV;
				uint32_t samplerCount{0};
				uint32_t width{0};
				uint32_t height{0};
			};

			/** @brief Push constants of the generated blur shader (pass direction). */
			struct DirectionPushConstants
			{
				float directionX;
				float directionY;
				float padding0;
				float padding1;
			};

			[[nodiscard]]
			Variant * getOrCreateVariant (size_t signature, const std::vector< GroupEntry > & contributions) noexcept;

			[[nodiscard]]
			bool updateFramebuffers (Variant & variant, const std::vector< GroupEntry > & contributions) noexcept;

			[[nodiscard]]
			static std::string buildFragmentShaderSource (const std::vector< GroupEntry > & contributions) noexcept;

			[[nodiscard]]
			static size_t computeSignature (const std::vector< GroupEntry > & contributions) noexcept;

			std::unordered_map< size_t, Variant > m_variants;
	};
}
