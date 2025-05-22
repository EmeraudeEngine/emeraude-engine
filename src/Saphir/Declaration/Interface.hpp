/*
 * src/Saphir/Declaration/Interface.hpp
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
#include <string>

/* Local inclusions for usages. */
#include "Saphir/Keys.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief The declaration interface.
	 * @note This is the base of all declaration in a shader generation code.
	 */
	class Interface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Interface (const Interface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Interface (Interface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Interface &
			 */
			Interface & operator= (const Interface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Interface &
			 */
			Interface & operator= (Interface && copy) noexcept = default;

			/**
			 * @brief Destructs the declaration interface.
			 */
			virtual ~Interface () = default;

			/**
			 * @brief Returns whether the declaration is valid for code generation.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isValid () const noexcept = 0;

			/**
			 * @brief Returns a unique name for this declaration.
			 * @return Key
			 */
			[[nodiscard]]
			virtual Key name () const noexcept = 0;

			/**
			 * @brief Returns the bytes consumed by the declaration data.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual uint32_t bytes () const noexcept = 0;

			/**
			 * @brief Returns the source code for this declaration to write it into the final shader.
			 * @return std::string
			 */
			[[nodiscard]]
			virtual std::string sourceCode () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a declaration interface.
			 */
			Interface () noexcept = default;
	};
}
