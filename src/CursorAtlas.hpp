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
#include <cstdint>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Window.hpp"
#include "CoreTypes.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class ImageResource;
}

namespace EmEn
{
	/**
	 * @class CursorAtlas
	 * @brief Manages mouse cursor representations for the application using GLFW.
	 *
	 * The CursorAtlas serves as a centralized repository for both standard and custom
	 * cursor graphics. It provides lazy initialization for standard GLFW cursors and
	 * caching for custom cursors to avoid redundant resource allocation. The class
	 * ensures proper cleanup of all GLFW cursor resources upon destruction.
	 *
	 * Standard cursors are created on-demand and stored in a fixed-size array for fast
	 * access. Custom cursors are stored in an unordered map with heterogeneous lookup
	 * support to avoid unnecessary string allocations during lookups.
	 *
	 * @note This class is not thread-safe. All cursor operations should be performed
	 * on the main thread where GLFW operations are valid.
	 *
	 * @note Cursor operations on windowless mode windows are gracefully ignored,
	 * preventing errors while maintaining API consistency.
	 *
	 * @see Window, CursorType
	 * @version 0.8.35
	 */
	class CursorAtlas final
	{
		public:

			/**
			 * @brief Class identifier constant for logging and debugging.
			 * @version 0.8.35
			 */
			static constexpr auto ClassId{"CursorAtlas"};

			/**
			 * @brief The number of standard cursor types supported by GLFW.
			 *
			 * This constant defines the size of the internal array holding
			 * standard cursor pointers. It corresponds to the number of values
			 * in the CursorType enumeration.
			 *
			 * @version 0.8.35
			 */
			static constexpr size_t StandardCursorCount{6};

			/**
			 * @brief Constructs an empty cursor atlas.
			 *
			 * Creates a cursor atlas with no cursors initialized. Standard cursors
			 * are created lazily upon first use, and custom cursors are added on demand.
			 *
			 * @version 0.8.35
			 */
			CursorAtlas () noexcept = default;

			/**
			 * @brief Copy constructor deleted.
			 *
			 * CursorAtlas manages GLFW cursor resources that cannot be safely copied.
			 *
			 * @version 0.8.35
			 */
			CursorAtlas (const CursorAtlas &) = delete;

			/**
			 * @brief Move constructor deleted.
			 *
			 * CursorAtlas manages GLFW cursor resources that should not be moved.
			 *
			 * @version 0.8.35
			 */
			CursorAtlas (CursorAtlas &&) = delete;

			/**
			 * @brief Copy assignment operator deleted.
			 *
			 * CursorAtlas manages GLFW cursor resources that cannot be safely copied.
			 *
			 * @version 0.8.35
			 */
			CursorAtlas & operator= (const CursorAtlas &) = delete;

			/**
			 * @brief Move assignment operator deleted.
			 *
			 * CursorAtlas manages GLFW cursor resources that should not be moved.
			 *
			 * @version 0.8.35
			 */
			CursorAtlas & operator= (CursorAtlas &&) = delete;

			/**
			 * @brief Destroys the cursor atlas and releases all GLFW cursor resources.
			 *
			 * Automatically calls clear() to destroy all standard and custom cursors
			 * managed by this atlas, preventing resource leaks.
			 *
			 * @post All GLFW cursor handles managed by this atlas are destroyed.
			 * @version 0.8.35
			 */
			~CursorAtlas () noexcept;

			/**
			 * @brief Sets the cursor to a standard GLFW cursor type.
			 *
			 * Changes the active cursor for the specified window to one of the standard
			 * cursor shapes provided by GLFW. Standard cursors are lazily initialized on
			 * first use and cached for subsequent calls.
			 *
			 * If the cursor type index is out of range, the operation is silently ignored.
			 * Operations on windowless mode windows are also silently ignored.
			 *
			 * @param window Reference to the window where the cursor should be changed.
			 * @param cursorType The standard cursor type to display (Arrow, TextInput, etc.).
			 *
			 * @pre The window must be valid and initialized.
			 * @post If successful and not in windowless mode, the window displays the specified cursor.
			 *
			 * @see CursorType, Window
			 * @version 0.8.35
			 */
			void setCursor (Window & window, CursorType cursorType) noexcept;

			/**
			 * @brief Sets the cursor to a custom cursor from a pixmap.
			 *
			 * Creates or retrieves a custom cursor from the provided pixmap and sets it as
			 * the active cursor for the specified window. The cursor is cached using the
			 * label as a key, avoiding redundant cursor creation for repeated calls with
			 * the same label.
			 *
			 * The pixmap must be RGBA format (4 channels). If the pixmap has a different
			 * channel count, an error is logged and the operation is aborted.
			 *
			 * @param window Reference to the window where the cursor should be changed.
			 * @param label Unique identifier for this custom cursor. Used for caching.
			 * @param pixmap The image data for the cursor in RGBA format (4 channels).
			 * @param hotSpot The cursor hotspot position [x, y] in pixels. Default is [0, 0] (top-left).
			 *
			 * @pre The window must be valid and initialized.
			 * @pre The pixmap must have exactly 4 color channels (RGBA).
			 * @post If successful, the custom cursor is cached and displayed on the window.
			 *
			 * @warning If the pixmap does not have 4 channels, an error is logged and the operation fails.
			 *
			 * @see Libs::PixelFactory::Pixmap, Window
			 * @version 0.8.35
			 */
			void setCursor (Window & window, std::string_view label, Libs::PixelFactory::Pixmap< uint8_t > pixmap, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept;

			/**
			 * @brief Sets the cursor to a custom cursor from raw pixel data.
			 *
			 * Low-level interface for creating custom cursors directly from raw RGBA pixel data.
			 * This method is provided for compatibility with GLFW's basic API but is not the
			 * recommended approach. Prefer using the pixmap or ImageResource overloads.
			 *
			 * The pixel data must be in RGBA format with size[0] × size[1] × 4 bytes. The cursor
			 * is cached using the label as a key.
			 *
			 * @param window Reference to the window where the cursor should be changed.
			 * @param label Unique identifier for this custom cursor. Used for caching.
			 * @param size Cursor dimensions [width, height] in pixels.
			 * @param data Pointer to the raw RGBA pixel data buffer. Must contain (width × height × 4) bytes.
			 * @param hotSpot The cursor hotspot position [x, y] in pixels. Default is [0, 0] (top-left).
			 *
			 * @pre The window must be valid and initialized.
			 * @pre The data pointer must be valid and contain sufficient RGBA data.
			 * @post If successful, the custom cursor is cached and displayed on the window.
			 *
			 * @warning This is a raw, low-level API. Prefer using the pixmap or ImageResource overloads.
			 * @warning The caller is responsible for ensuring the data buffer is valid and properly sized.
			 *
			 * @see Window
			 * @version 0.8.35
			 */
			void setCursor (Window & window, std::string_view label, const std::array< int, 2 > & size, unsigned char * data, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept;

			/**
			 * @brief Sets the cursor to a custom cursor from an image resource.
			 *
			 * Convenience method that sets a custom cursor using a loaded ImageResource.
			 * The image resource must be fully loaded before calling this method. If the
			 * resource is not loaded, the operation is silently ignored.
			 *
			 * This method internally calls the pixmap overload, caching the cursor using
			 * the image resource's name as the label.
			 *
			 * @param window Reference to the window where the cursor should be changed.
			 * @param imageResource Shared pointer to a loaded image resource containing cursor data.
			 * @param hotSpot The cursor hotspot position [x, y] in pixels. Default is [0, 0] (top-left).
			 *
			 * @pre The window must be valid and initialized.
			 * @pre The imageResource must be loaded (isLoaded() returns true).
			 * @post If successful, the custom cursor is cached and displayed on the window.
			 *
			 * @see Graphics::ImageResource, Window
			 * @version 0.8.35
			 */
			void setCursor (Window & window, const std::shared_ptr< Graphics::ImageResource > & imageResource, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept;

			/**
			 * @brief Resets the cursor to the default system cursor.
			 *
			 * Restores the window's cursor to the default system cursor by passing nullptr
			 * to GLFW. This operation is silently ignored for windowless mode windows.
			 *
			 * @param window Reference to the window where the cursor should be reset.
			 *
			 * @pre The window must be valid and initialized.
			 * @post If not in windowless mode, the window displays the default system cursor.
			 *
			 * @see Window
			 * @version 0.8.35
			 */
			void resetCursor (Window & window) noexcept;

			/**
			 * @brief Removes all cursors from the atlas and releases GLFW resources.
			 *
			 * Destroys all standard and custom GLFW cursor handles managed by this atlas
			 * and clears the internal storage. After this call, all previously set cursors
			 * are invalidated and must be recreated if needed.
			 *
			 * This method is automatically called by the destructor.
			 *
			 * @post All GLFW cursor handles are destroyed and internal storage is cleared.
			 * @post All cursor pointers in the internal arrays become nullptr.
			 *
			 * @version 0.8.35
			 */
			void clear () noexcept;

		private:

			/**
			 * @brief Transparent hash functor for heterogeneous lookup with string_view.
			 *
			 * This helper struct enables efficient heterogeneous lookup in the custom
			 * cursors map, allowing searches using string_view without allocating a
			 * temporary std::string. This optimization avoids unnecessary memory
			 * allocations when checking if a cursor already exists.
			 *
			 * @version 0.8.35
			 */
			struct StringHash
			{
				/**
				 * @brief Type tag enabling heterogeneous lookup.
				 * @version 0.8.35
				 */
				using is_transparent = void;

				/**
				 * @brief Computes the hash of a string_view.
				 * @param sv The string_view to hash.
				 * @return size_t The computed hash value for use in unordered containers.
				 * @version 0.8.35
				 */
				[[nodiscard]]
				size_t
				operator() (std::string_view sv) const noexcept
				{
					return std::hash< std::string_view >{}(sv);
				}
			};

			/**
			 * @brief Array holding lazily-initialized standard GLFW cursors.
			 *
			 * Each element corresponds to a CursorType enum value. Cursors are
			 * created on first use and remain valid until clear() or destruction.
			 *
			 * @version 0.8.35
			 */
			std::array< GLFWcursor *, StandardCursorCount > m_standardCursors{};

			/**
			 * @brief Map holding custom cursors indexed by their label.
			 *
			 * Uses heterogeneous lookup via StringHash to avoid string allocations
			 * during lookups. Keys are std::string for ownership, but lookups can
			 * be performed using string_view.
			 *
			 * @version 0.8.35
			 */
			std::unordered_map< std::string, GLFWcursor *, StringHash, std::equal_to<> > m_customCursors;
	};
}
