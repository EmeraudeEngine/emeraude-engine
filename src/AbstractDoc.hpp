/*
 * src/AbstractDoc.hpp
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

namespace EmEn
{
	/**
	 * @brief Base class for the help service documentation.
	 */
	class AbstractDoc
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDoc (const AbstractDoc & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDoc (AbstractDoc && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractDoc &
			 */
			AbstractDoc & operator= (const AbstractDoc & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractDoc &
			 */
			AbstractDoc & operator= (AbstractDoc && copy) noexcept = default;

			/**
			 * @brief The virtual destructor.
			 */
			virtual ~AbstractDoc () = default;

			/**
			 * @brief Returns the description.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			description () const noexcept
			{
				return m_description;
			}

		protected:

			/**
			 * @brief Constructs a base documentation.
			 * @param description A reference to a string [std::move].
			 */
			explicit AbstractDoc (std::string description) noexcept
				: m_description{std::move(description)}
			{

			}

		private:

			std::string m_description;
	};
}
