/*
 * src/Overlay/ImGUIScreen.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* Application configuration. */
#include "emeraude_config.hpp"

#ifdef IMGUI_ENABLED

/* STL inclusions. */
#include <array>
#include <string>
#include <functional>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
}

namespace EmEn::Overlay
{
	/**
	 * @brief The ImGUI screen specific class.
	 * @exception EmEn::Libs::NameableTrait A UI screen have a name.
	 */
	class ImGUIScreen final : public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImGUIScreen"};

			/**
			 * @brief Constructs an ImGUI screen.
			 * @param name A string [std::move].
			 * @param drawFunction A reference to a function.
			 */
			ImGUIScreen (const std::string & name, const std::function< void () > & drawFunction) noexcept
				: NameableTrait{std::move(name)},
				m_drawFunction{drawFunction}
			{

			}

			/**
			 * @brief Sets the UI screen visibility.
			 * @param state The state
			 * @return void
			 */
			void
			setVisibility (bool state) noexcept
			{
				m_flags[IsVisible] = state;
			}

			/**
			 * @brief Returns whether the UI screen is visible.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVisible () const noexcept
			{
				return m_flags[IsVisible];
			}

			/**
			 * @brief Renders the screen.
			 * @param commandBuffer A reference to a vulkan command buffer;
			 * @return void
			 */
			void render (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

		private:

			/* Flag names. */
			static constexpr auto IsVisible{0UL};

			std::function< void () > m_drawFunction;
			std::array< bool, 8 > m_flags{
				false/*IsVisible*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}

#endif
