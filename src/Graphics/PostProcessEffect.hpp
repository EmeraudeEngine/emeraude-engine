/*
 * src/Graphics/PostProcessEffect.hpp
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

/* Local inclusions for usages. */
#include "PostProcessor.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class TextureInterface;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief Abstract interface for multi-pass post-processing effects.
	 * @note Each effect manages its own intermediate render targets, render passes, and pipelines.
	 * Effects receive an input texture and produce an output texture, forming a chain.
	 */
	class PostProcessEffect
	{
		public:

			/**
			 * @brief Destructs the post-process effect.
			 */
			virtual ~PostProcessEffect () = default;

			/**
			 * @brief Creates GPU resources for this effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool create (Renderer & renderer, uint32_t width, uint32_t height) noexcept = 0;

			/**
			 * @brief Destroys GPU resources for this effect.
			 * @return void
			 */
			virtual void destroy () noexcept = 0;

			/**
			 * @brief Recreates GPU resources after a resize.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The new framebuffer width.
			 * @param height The new framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept = 0;

			/**
			 * @brief Executes the effect for the current frame.
			 * @note Called outside of any active render pass. The effect manages its own render passes.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The input color texture to process.
			 * @param inputDepth The input depth texture (may be nullptr if not available).
			 * @param inputNormals The input normals texture (may be nullptr if not available).
			 * @param constants The current post-processing push constants.
			 * @return const Vulkan::TextureInterface & The output texture to pass to the next effect.
			 */
			[[nodiscard]]
			virtual const Vulkan::TextureInterface & execute (
				const Vulkan::CommandBuffer & commandBuffer,
				const Vulkan::TextureInterface & inputColor,
				const Vulkan::TextureInterface * inputDepth,
				const Vulkan::TextureInterface * inputNormals,
				const PostProcessor::PushConstants & constants
			) noexcept = 0;

			/**
			 * @brief Returns whether this effect requires a depth buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresDepth () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires HDR input data.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresHDR () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a normals buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresNormals () const noexcept
			{
				return false;
			}

			/**
			 * @brief Enables or disables this effect.
			 * @param state The desired enabled state.
			 * @return void
			 */
			void
			enable (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether this effect is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled;
			}

		protected:

			/**
			 * @brief Constructs a post-process effect.
			 */
			PostProcessEffect () noexcept = default;

			bool m_enabled{true};
	};

	/** @brief A chain of post-process effects to be executed sequentially. */
	using PostProcessEffectChain = std::vector< std::shared_ptr< PostProcessEffect > >;
}
