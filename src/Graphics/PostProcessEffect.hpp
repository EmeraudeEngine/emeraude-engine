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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief Abstract base class for all post-processing effects.
	 * @note Provides shared enable/disable state. Derived classes implement either
	 * multi-pass pipeline effects (IndirectPostProcessEffect) or single-pass
	 * fragment shader effects (DirectPostProcessEffect).
	 */
	class EMEN_API PostProcessEffect
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PostProcessEffect (const PostProcessEffect & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PostProcessEffect (PostProcessEffect && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PostProcessEffect &
			 */
			PostProcessEffect & operator= (const PostProcessEffect & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PostProcessEffect &
			 */
			PostProcessEffect & operator= (PostProcessEffect && copy) noexcept = default;

			/**
			 * @brief Destructs the post-process effect.
			 */
			virtual ~PostProcessEffect () = default;

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

			/**
			 * @brief Returns the effect display label, used by the GPU profiler scopes.
			 * @note Every concrete effect should override this with its ClassId; the
			 * default only exists so display-only helpers are not forced to.
			 * @return const char *
			 */
			[[nodiscard]]
			virtual
			const char *
			label () const noexcept
			{
				return "Effect";
			}

			/**
			 * @brief Returns whether this effect provides SCENE reflections (SSR, RTR).
			 * @note Drives the reflection cost ladder: while an enabled reflection provider is
			 * in the scene stack, the Renderer suspends the continuous reflection probes
			 * (render-to-cubemap) — paying a probe AND a traced reflection for the same
			 * surfaces would evaluate the same lobe twice.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			providesReflections () const noexcept
			{
				return false;
			}

		protected:

			/**
			 * @brief Constructs a post-process effect.
			 */
			PostProcessEffect () noexcept = default;

		private:

			bool m_enabled{true};
	};
}
