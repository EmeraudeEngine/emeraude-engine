/*
 * src/Overlay/ImGUIScreen.hpp
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
#include "emeraude_config.hpp"

#ifdef IMGUI_ENABLED

/* STL inclusions. */
#include <array>
#include <string>
#include <functional>

/* Local inclusions for inheritances. */
#include "NameableTrait.hpp"

namespace EmEn::Overlay
{
	/**
	 * @brief The ImGUI screen specific class.
	 * @exception EmEn::Base::NameableTrait A UI screen have a name.
	 */
	class ImGUIScreen final : public Base::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImGUIScreen"};

			/**
			 * @brief Constructs an ImGUI screen.
			 * @param name A string [std::move].
			 * @param drawFunction A reference to a function.
			 */
			ImGUIScreen (std::string name, const std::function< void () > & drawFunction) noexcept
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
				m_isVisible = state;
			}

			/**
			 * @brief Returns whether the UI screen is visible.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVisible () const noexcept
			{
				return m_isVisible;
			}

			/**
			 * @brief Emits the ImGUI widgets of this screen.
			 * @note Must be called between ImGui::NewFrame() and ImGui::Render(). The
			 * Overlay::Manager drives a single NewFrame()/Render() cycle per frame for
			 * every visible ImGUI screen (ImGUI uses a single global context).
			 * @return void
			 */
			void
			draw () const noexcept
			{
				m_drawFunction();
			}

		private:

			std::function< void () > m_drawFunction;
			bool m_isVisible{false};
	};
}

#endif
