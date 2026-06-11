/*
* src/Overlay/Manager.PointerListenerInterface.cpp
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
	Manager::onPointerMove (float positionX, float positionY) noexcept
	{
		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable pointer move event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onPointerMove(positionX, positionY);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onPointerMove(positionX, positionY);
		});
	}

	bool
	Manager::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable pointer button press event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onButtonPress(positionX, positionY, buttonNumber, modifiers);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onButtonPress(positionX, positionY, buttonNumber, modifiers);
		});
	}

	bool
	Manager::onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable pointer button release event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onButtonRelease(positionX, positionY, buttonNumber, modifiers);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onButtonRelease(positionX, positionY, buttonNumber, modifiers);
		});
	}

	bool
	Manager::onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers) noexcept
	{
		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			Tracer::debug(ClassId, "Received a dispatchable mouse wheel event!");
		}

		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onMouseWheel(positionX, positionY, xOffset, yOffset, modifiers);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onMouseWheel(positionX, positionY, xOffset, yOffset, modifiers);
		});
	}
}
