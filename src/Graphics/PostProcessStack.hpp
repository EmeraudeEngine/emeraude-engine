/*
 * src/Graphics/PostProcessStack.hpp
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

/* Forward declarations. */
namespace EmEn::Graphics
{
	class IndirectPostProcessEffect;
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Ordered container of multi-pass post-process effects with GPU lifecycle management.
	 * @note Owned by a Scene to provide per-scene post-processing configuration.
	 */
	class PostProcessStack final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessStack"};

			PostProcessStack () noexcept = default;
			~PostProcessStack () noexcept;

			/* Non-copiable, movable. */
			PostProcessStack (const PostProcessStack &) = delete;
			PostProcessStack & operator= (const PostProcessStack &) = delete;
			PostProcessStack (PostProcessStack &&) noexcept = default;
			PostProcessStack & operator= (PostProcessStack &&) noexcept = default;

			/**
			 * @brief Appends an effect to the chain.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void addEffect (std::shared_ptr< IndirectPostProcessEffect > effect) noexcept;

			/**
			 * @brief Removes an effect from the chain.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void removeEffect (const std::shared_ptr< IndirectPostProcessEffect > & effect) noexcept;

			/**
			 * @brief Clears the entire effect chain.
			 * @return void
			 */
			void clearEffects () noexcept;

			/**
			 * @brief Returns the effect chain.
			 * @return const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			effects () const noexcept
			{
				return m_effects;
			}

			/**
			 * @brief Returns whether the stack has any effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasEffects () const noexcept
			{
				return !m_effects.empty();
			}

			/**
			 * @brief Creates GPU resources for all effects.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			bool createAll (Renderer & renderer, uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Destroys GPU resources for all effects.
			 * @return void
			 */
			void destroyAll () noexcept;

			/**
			 * @brief Recreates GPU resources after a resize.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The new framebuffer width.
			 * @param height The new framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			bool resizeAll (Renderer & renderer, uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires HDR input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresHDR () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires depth input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresDepth () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires normals input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresNormals () const noexcept;

		private:

			std::vector< std::shared_ptr< IndirectPostProcessEffect > > m_effects;
	};
}
