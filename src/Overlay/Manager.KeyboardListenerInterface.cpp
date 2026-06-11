/*
 * src/Overlay/Manager.KeyboardListenerInterface.cpp
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

#include "Manager.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

namespace EmEn::Overlay
{
	bool
	Manager::onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable keyboard key press event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningKeyboard() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onKeyPress(key, scancode, modifiers, repeat);
		}

		return std::ranges::any_of(m_screens, [key, scancode, modifiers, repeat] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningKeyboard() )
			{
				return false;
			}

			return screen->onKeyPress(key, scancode, modifiers, repeat);
		});
	}

	bool
	Manager::onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable keyboard key release event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningKeyboard() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onKeyRelease(key, scancode, modifiers);
		}

		return std::ranges::any_of(m_screens, [key, scancode, modifiers] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningKeyboard() )
			{
				return false;
			}

			return screen->onKeyRelease(key, scancode, modifiers);
		});
	}

	bool
	Manager::onCharacterType (uint32_t unicode) noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable keyboard character type event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningKeyboard() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onCharacterType(unicode);
		}

		return std::ranges::any_of(m_screens, [unicode] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningKeyboard() )
			{
				return false;
			}

			return screen->onCharacterType(unicode);
		});
	}
}
