/*
 * src/Graphics/CombinePass.hpp
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
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

/* Local inclusions for inheritances. */
#include "IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "IntermediateRenderTarget.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The shared APPLY pass of the overlay effects (phase A of the pass-merging plan).
	 * @note Owned by the PostProcessor. Each contiguous group of overlay effects in the
	 * chain (reflections, AO, GI, contact shadows, volumetrics, flares) runs its internal
	 * passes through recordOverlayPasses(), then this pass applies ALL their results onto
	 * the chain color in ONE generated full-resolution fragment shader — reproducing the
	 * exact sequential math while paying a single full-res read/write instead of one
	 * apply/composite pass per effect.
	 * The generated shader (and its pipeline) is cached per group SIGNATURE (the ordered
	 * effect prefixes + context sampler needs); per-frame scalars travel through a small
	 * uniform buffer, textures are rebound every frame.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect Reuses the shared fullscreen pass
	 * infrastructure (pipeline creation, pass recording, descriptor helpers) — it is never
	 * inserted into a stack itself.
	 */
	class EMEN_API CombinePass final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CombinePass"};

			/** @brief One overlay effect's contribution, gathered by the PostProcessor. */
			using GroupEntry = CombineContribution;

			/**
			 * @brief Constructs the combine pass.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			CombinePass (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/**
			 * @brief Returns whether the pass targets are created.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_targets[0].isCreated();
			}

			/**
			 * @brief Records ONE combine pass applying a group of contributions onto the input color.
			 * @note Called outside any active render pass. The output alternates between two
			 * full-resolution targets (groupIndex parity), so consecutive groups chain safely.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The group's input color (the chain color when the group started).
			 * @param contributions The ordered contributions of the group's effects.
			 * @param context The per-frame chain context.
			 * @param groupIndex The group ordinal within the frame (selects the output target).
			 * @return const Vulkan::TextureInterface & The combined output, or inputColor on failure.
			 */
			[[nodiscard]]
			const Vulkan::TextureInterface & record (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const std::vector< GroupEntry > & contributions, const FrameContext & context, uint32_t groupIndex) noexcept;

		private:

			/** @brief GPU objects for one group signature (ordered prefixes + context needs). */
			struct Variant
			{
				std::shared_ptr< Vulkan::GraphicsPipeline > pipeline;
				std::shared_ptr< Vulkan::PipelineLayout > pipelineLayout;
				std::vector< std::unique_ptr< Vulkan::DescriptorSet > > descriptorSetsPerFrame;
				std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > uniformBuffersPerFrame;
				uint32_t samplerCount{0};
			};

			/**
			 * @brief Returns (creating on first use) the pipeline variant for a group signature.
			 * @param signature The group signature hash.
			 * @param contributions The ordered contributions (source of the generated GLSL).
			 * @return Variant * nullptr on creation failure.
			 */
			[[nodiscard]]
			Variant * getOrCreateVariant (size_t signature, const std::vector< GroupEntry > & contributions) noexcept;

			/**
			 * @brief Builds the combine fragment shader source for a group.
			 * @param contributions The ordered contributions.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string buildFragmentShaderSource (const std::vector< GroupEntry > & contributions) noexcept;

			/**
			 * @brief Computes the signature hash of a group.
			 * @param contributions The ordered contributions.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t computeSignature (const std::vector< GroupEntry > & contributions) noexcept;

			/* Two full-res HDR targets: consecutive groups ping-pong between them. */
			std::array< IntermediateRenderTarget, 2 > m_targets;
			std::unordered_map< size_t, Variant > m_variants;
	};
}
