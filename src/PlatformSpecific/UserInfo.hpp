/*
 * src/PlatformSpecific/UserInfo.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <string>
#include <ostream>
#include <sstream>
#include <filesystem>

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief The user info class. This will gather information on the current user.
	 */
	class UserInfo final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"UserInfo"};

			/** 
			 * @brief Constructs a user info structure.
			 */
			UserInfo () noexcept;

			/**
			 * @brief Returns whether the current user information has been reached.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			informationFound () const noexcept
			{
				return m_informationFound;
			}

			/**
			 * @brief Returns the nice name of the current user.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns the name of the current user account.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			accountName () const noexcept
			{
				return m_accountName;
			}

			/**
			 * @brief Returns the home directory path of the current user.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			homePath () const noexcept
			{
				return m_homePath;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const UserInfo & obj);

			std::string m_name;
			std::string m_accountName;
			std::filesystem::path m_homePath;
			bool m_informationFound{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const UserInfo & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
