/*
 * src/CursorAtlas.hpp
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

/* STL inclusions. */
#include <string>
#include <map>
#include <memory>
#include <array>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions for usages. */
#include "Window.hpp"
#include "Graphics/ImageResource.hpp"
#include "CoreTypes.hpp"

namespace EmEn
{
	/**
	 * @brief The cursor atlas holds all pointer representation for the application.
	 */
	class CursorAtlas final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CursorAtlas"};

			/** 
			 * @brief Constructs the cursor atlas.
			 */
			CursorAtlas () noexcept = default;

			/**
			 * @brief Changes the cursor representation on screen with a standard type.
			 * @param window A reference to the window where to change the cursor.
			 * @param cursorType The type of the cursor.
			 * @return void
			 */
			void setCursor (Window & window, CursorType cursorType) noexcept;

			/**
			 * @brief Changes the cursor representation on screen with a pixmap.
			 * @param window A reference to the window where to change the cursor.
			 * @param label A reference to a string.
			 * @param pixmap A reference to a pixmap.
			 * @param hotSpot A reference to an array of 2 integers. Default X:0, Y:0.
			 * @return void
			 */
			void setCursor (Window & window, const std::string & label, Libs::PixelFactory::Pixmap< uint8_t > pixmap, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept;

			/**
			 * @brief Changes the cursor representation on screen with a pixmap.
			 * @warning Raw mode to fit the GLFW basic needs. This is not the recommended version.
			 * @param window A reference to the window where to change the cursor.
			 * @param label A reference to a string.
			 * @param size A reference to an array of 2 integers.
			 * @param data A pointer to an uint8_t buffer. It must contain (width X height X 4) elements.
			 * @param hotSpot A reference to an array of 2 integers. Default X:0, Y:0.
			 * @return void
			 */
			void setCursor (Window & window, const std::string & label, const std::array< int, 2 > & size, unsigned char * data, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept;

			/**
			 * @brief Changes the cursor representation on screen with an image resource.
			 * @param window A reference to the window where to change the cursor.
			 * @param imageResource A reference to an image resource smart pointer.
			 * @param hotSpot A reference to an array of 2 integers. Default X:0, Y:0.
			 * @return void
			 */
			void
			setCursor (Window & window, const std::shared_ptr< Graphics::ImageResource > & imageResource, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept
			{
				if ( !imageResource->isLoaded() )
				{
					return;
				}

				this->setCursor(window, imageResource->name(), imageResource->data(), hotSpot);
			}

			/**
			 * @brief Reset back the cursor to default representation.
			 * @param window A reference to the window where to reset the cursor.
			 * @return void
			 */
			void resetCursor (Window & window) noexcept;

			/**
			 * @brief Removes all cursor from the atlas.
			 * @return void
			 */
			void clear () noexcept;

		private:

			std::map< CursorType, GLFWcursor * > m_standardCursors;
			std::map< std::string, GLFWcursor * > m_customCursors;
	};
}
