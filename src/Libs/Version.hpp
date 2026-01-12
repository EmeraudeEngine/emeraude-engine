/*
 * src/Libs/Version.hpp
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
#include <cstdint>
#include <sstream>
#include <string>
#include <compare>
#include <optional>

namespace EmEn::Libs
{
	/**
	 * @brief Utility to create a parametric versioning.
	 */
	class Version final
	{
		public:

			/**
			 * @brief Constructs a default version.
			 */
			Version () noexcept = default;

			/**
			 * @brief Constructs a version.
			 * @param major The major number.
			 * @param minor The minor number.
			 * @param revision The revision number.
			 */
			constexpr Version (int major, int minor, int revision) noexcept
				: m_major{major},
				m_minor{minor},
				m_revision{revision}
			{

			}

			/**
			 * @brief Constructs a version from a number.
			 * @param bitmask An unsigned integer of 32 bits.
			 */
			explicit
			Version (uint32_t bitmask) noexcept
				: m_major{static_cast<int>((bitmask >> 22) & 0x3FF)}, // 0x3FF = mask 10 bits
				m_minor{static_cast<int>((bitmask >> 12) & 0x3FF)}, // 0x3FF = mask 10 bits
				m_revision{static_cast<int>(bitmask & 0xFFF)} // 0xFFF = mask 12 bits
			{

			}

			/**
			 * @brief Default three-way comparison operator (since C++20).
			 * @note Generates ==, !=, <, >, <=, >= automatically.
			 * @return bool
			 */
			auto operator<=> (const Version& other) const noexcept = default;

			/**
			 * @brief Parses a string to find the version numbers.
			 * @note This method return false on failure and let the version to 0.0.0
			 * @param string A reference to a string.
			 * @param separator The number delimiter. Default '.'.
			 * @return bool
			 */
			bool
			parseFromString (const std::string & string, char separator = '.') noexcept
			{
				std::istringstream stream{string};

				char sepA = 0;
				char sepB = 0;

				return
					stream >> m_major >> sepA >> m_minor >> sepB >> m_revision &&
					sepA == separator &&
					sepB == separator &&
					(stream >> std::ws).eof();
			}

			/**
			 * @brief Sets the version.
			 * @param major The major number.
			 * @param minor The minor number.
			 * @param revision The revision number.
			 * @return void
			 */
			void
			set (int major, int minor, int revision) noexcept
			{
				m_major = major;
				m_minor = minor;
				m_revision = revision;
			}

			/**
			 * @brief Sets the major number of the version.
			 * @param value
			 * @return void
			 */
			void
			setMajor (int value) noexcept
			{
				m_major = value;
			}

			/**
			 * @brief Sets the minor number of the version.
			 * @param value
			 * @return void
			 */
			void
			setMinor (int value) noexcept
			{
				m_minor = value;
			}

			/**
			 * @brief Sets the revision number of the version.
			 * @param value
			 * @return void
			 */
			void
			setRevision (int value) noexcept
			{
				m_revision = value;
			}

			/**
			 * @brief Returns the major number of the version.
			 * @return int
			 */
			[[nodiscard]]
			int
			major () const noexcept
			{
				return m_major;
			}

			/**
			 * @brief Returns the minor number of the version.
			 * @return int
			 */
			[[nodiscard]]
			int
			minor () const noexcept
			{
				return m_minor;
			}

			/**
			 * @brief Returns the revision number of the version.
			 * @return int
			 */
			[[nodiscard]]
			int
			revision () const noexcept
			{
				return m_revision;
			}

			/**
			 * @brief Creates a Version from a string, returning an empty optional on failure.
			 * @param string The string to parse (e.g., "1.2.3").
			 * @return std::optional< Version >
			 */
			[[nodiscard]]
			static
			std::optional< Version >
			FromString (const std::string& string, char separator = '.') noexcept
			{
				Version version;

				if ( version.parseFromString(string, separator))
				{
					return version;
				}

				return std::nullopt;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Version & obj);

			int m_major{0};
			int m_minor{0};
			int m_revision{0};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Version & obj)
	{
		return out << obj.major() << '.' << obj.minor() << '.' << obj.revision();
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Version & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
