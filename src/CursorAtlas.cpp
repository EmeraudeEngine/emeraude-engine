/*
 * src/CursorAtlas.cpp
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

#include "CursorAtlas.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Libs;
	
	void
	CursorAtlas::setCursor (Window & window, CursorType cursorType) noexcept
	{
		auto cursorIt = m_standardCursors.find(cursorType);

		if ( cursorIt == m_standardCursors.end() )
		{
			switch ( cursorType )
			{
				case CursorType::Arrow :
					cursorIt = m_standardCursors.emplace(cursorType, glfwCreateStandardCursor(GLFW_ARROW_CURSOR)).first;
					break;

				case CursorType::TextInput :
					cursorIt = m_standardCursors.emplace(cursorType, glfwCreateStandardCursor(GLFW_IBEAM_CURSOR)).first;
					break;

				case CursorType::Crosshair :
					cursorIt = m_standardCursors.emplace(cursorType, glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR)).first;
					break;

				case CursorType::Hand :
					cursorIt = m_standardCursors.emplace(cursorType, glfwCreateStandardCursor(GLFW_HAND_CURSOR)).first;
					break;

				case CursorType::HorizontalResize :
					cursorIt = m_standardCursors.emplace(cursorType, glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR)).first;
					break;

				case CursorType::VerticalResize :
					cursorIt = m_standardCursors.emplace(cursorType, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR)).first;
					break;
			}
		}

		glfwSetCursor(window.handle(), cursorIt->second);
	}

	void
	CursorAtlas::setCursor (Window & window, const std::string & label, const std::array< int, 2 > & size, unsigned char * data, const std::array< int, 2 > & hotSpot) noexcept
	{
		auto cursorIt = m_customCursors.find(label);

		if ( cursorIt == m_customCursors.end() )
		{
			const GLFWimage cursorImage{
				.width = size[0],
				.height = size[1],
				.pixels = data
			};

			cursorIt = m_customCursors.emplace(label, glfwCreateCursor(&cursorImage, hotSpot[0], hotSpot[1])).first;
		}

		glfwSetCursor(window.handle(), cursorIt->second);
	}

	void
	CursorAtlas::setCursor (Window & window, const std::string & label, PixelFactory::Pixmap< uint8_t > pixmap, const std::array< int, 2 > & hotSpot) noexcept
	{
		if ( pixmap.colorCount() != 4 )
		{
			TraceError{ClassId} << "A cursor needs a 4 channels image !";

			return;
		}

		auto cursorIt = m_customCursors.find(label);

		if ( cursorIt == m_customCursors.end() )
		{
			const GLFWimage cursorImage{
				.width = static_cast< int >(pixmap.width()),
				.height = static_cast< int >(pixmap.height()),
				.pixels = pixmap.pixelPointer(0)
			};

			cursorIt = m_customCursors.emplace(label, glfwCreateCursor(&cursorImage, hotSpot[0], hotSpot[1])).first;
		}

		glfwSetCursor(window.handle(), cursorIt->second);
	}

	void
	CursorAtlas::resetCursor (Window & window) noexcept
	{
		glfwSetCursor(window.handle(), nullptr);
	}

	void
	CursorAtlas::clear () noexcept
	{
		for ( const auto & cursor : m_standardCursors | std::views::values )
		{
			glfwDestroyCursor(cursor);
		}

		m_standardCursors.clear();

		for ( const auto & cursor : m_customCursors | std::views::values )
		{
			glfwDestroyCursor(cursor);
		}

		m_customCursors.clear();
	}
}
