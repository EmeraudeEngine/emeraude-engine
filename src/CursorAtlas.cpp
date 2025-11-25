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

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Graphics/ImageResource.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Libs;

	namespace
	{
		/**
		 * @brief Converts a CursorType enum to the corresponding GLFW cursor shape constant.
		 * @param cursorType The cursor type to convert.
		 * @return The GLFW cursor shape constant.
		 */
		[[nodiscard]]
		constexpr int
		toGLFWCursorShape (CursorType cursorType) noexcept
		{
			switch ( cursorType )
			{
				case CursorType::Arrow :
					return GLFW_ARROW_CURSOR;

				case CursorType::TextInput :
					return GLFW_IBEAM_CURSOR;

				case CursorType::Crosshair :
					return GLFW_CROSSHAIR_CURSOR;

				case CursorType::Hand :
					return GLFW_HAND_CURSOR;

				case CursorType::HorizontalResize :
					return GLFW_HRESIZE_CURSOR;

				case CursorType::VerticalResize :
					return GLFW_VRESIZE_CURSOR;
			}

			return GLFW_ARROW_CURSOR;
		}
	}

	CursorAtlas::~CursorAtlas () noexcept
	{
		this->clear();
	}

	void
	CursorAtlas::setCursor (Window & window, CursorType cursorType) noexcept
	{
		const auto index = static_cast< size_t >(cursorType);

		if ( index >= StandardCursorCount )
		{
			return;
		}

		if ( m_standardCursors[index] == nullptr )
		{
			m_standardCursors[index] = glfwCreateStandardCursor(toGLFWCursorShape(cursorType));
		}

		if ( !window.isWindowLessMode() )
		{
			glfwSetCursor(window.handle(), m_standardCursors[index]);
		}
	}

	void
	CursorAtlas::setCursor (Window & window, std::string_view label, const std::array< int, 2 > & size, unsigned char * data, const std::array< int, 2 > & hotSpot) noexcept
	{
		/* Heterogeneous lookup: no allocation if cursor already exists. */
		auto cursorIt = m_customCursors.find(label);

		if ( cursorIt == m_customCursors.end() )
		{
			const GLFWimage cursorImage{
				.width = size[0],
				.height = size[1],
				.pixels = data
			};

			/* Only allocate std::string when inserting a new cursor. */
			cursorIt = m_customCursors.emplace(std::string{label}, glfwCreateCursor(&cursorImage, hotSpot[0], hotSpot[1])).first;
		}

		if ( !window.isWindowLessMode() )
		{
			glfwSetCursor(window.handle(), cursorIt->second);
		}
	}

	void
	CursorAtlas::setCursor (Window & window, std::string_view label, PixelFactory::Pixmap< uint8_t > pixmap, const std::array< int, 2 > & hotSpot) noexcept
	{
		if ( pixmap.colorCount() != 4 )
		{
			TraceError{ClassId} << "A cursor needs a 4 channels image !";

			return;
		}

		/* Heterogeneous lookup: no allocation if cursor already exists. */
		auto cursorIt = m_customCursors.find(label);

		if ( cursorIt == m_customCursors.end() )
		{
			const GLFWimage cursorImage{
				.width = static_cast< int >(pixmap.width()),
				.height = static_cast< int >(pixmap.height()),
				.pixels = pixmap.pixelPointer(0)
			};

			/* Only allocate std::string when inserting a new cursor. */
			cursorIt = m_customCursors.emplace(std::string{label}, glfwCreateCursor(&cursorImage, hotSpot[0], hotSpot[1])).first;
		}

		if ( !window.isWindowLessMode() )
		{
			glfwSetCursor(window.handle(), cursorIt->second);
		}
	}

	void
	CursorAtlas::setCursor (Window & window, const std::shared_ptr< Graphics::ImageResource > & imageResource, const std::array< int, 2 > & hotSpot) noexcept
	{
		if ( !imageResource->isLoaded() )
		{
			return;
		}

		this->setCursor(window, imageResource->name(), imageResource->data(), hotSpot);
	}

	void
	CursorAtlas::resetCursor (Window & window) noexcept
	{
		if ( window.isWindowLessMode() )
		{
			return;
		}

		glfwSetCursor(window.handle(), nullptr);
	}

	void
	CursorAtlas::clear () noexcept
	{
		for ( auto & cursor : m_standardCursors )
		{
			if ( cursor != nullptr )
			{
				glfwDestroyCursor(cursor);
				cursor = nullptr;
			}
		}

		for ( const auto & cursor : m_customCursors | std::views::values )
		{
			glfwDestroyCursor(cursor);
		}

		m_customCursors.clear();
	}
}
