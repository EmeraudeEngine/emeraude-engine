/*
 * src/Libs/Version.cpp
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

#include "Version.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>
#include <sstream>

/* Local inclusions. */
#include "String.hpp"

namespace EmEn::Libs
{
	Version::Version (const std::string & raw) noexcept
	{
		if ( !this->set(raw) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", " << raw << " is not a valid string for a version representation." << "\n";
		}
	}

	Version::Version (uint32_t bitmask) noexcept
		: m_major(static_cast< int >(bitmask >> 22)),
		m_minor(static_cast< int >(((bitmask << 10) >> 10) >> 12)),
		m_revision(static_cast< int >((bitmask << 20) >> 20))
	{

	}

	bool
	Version::operator== (const Version & operand) const noexcept
	{
		if ( this != &operand )
		{
			if ( operand.m_major != m_major )
			{
				return false;
			}

			if ( operand.m_minor != m_minor )
			{
				return false;
			}

			if ( operand.m_revision != m_revision )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Version::operator!= (const Version & operand) const noexcept
	{
		return !this->operator==(operand);
	}

	bool
	Version::operator> (const Version & operand) const noexcept
	{
		if ( m_major > operand.m_major )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor > operand.m_minor )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor == operand.m_minor && m_revision > operand.m_revision )
		{
			return true;
		}

		return false;
	}

	bool
	Version::operator>= (const Version & operand) const noexcept
	{
		if ( m_major > operand.m_major )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor > operand.m_minor )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor == operand.m_minor && m_revision >= operand.m_revision )
		{
			return true;
		}

		return false;
	}

	bool
	Version::operator< (const Version & operand) const noexcept
	{
		if ( m_major < operand.m_major )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor < operand.m_minor )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor == operand.m_minor && m_revision < operand.m_revision )
		{
			return true;
		}

		return false;
	}

	bool
	Version::operator<= (const Version & operand) const noexcept
	{
		if ( m_major < operand.m_major )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor < operand.m_minor )
		{
			return true;
		}

		if ( m_major == operand.m_major && m_minor == operand.m_minor && m_revision <= operand.m_revision )
		{
			return true;
		}

		return false;
	}

	bool
	Version::set (const std::string & raw) noexcept
	{
		const auto chunks = String::explode(raw, '.');

		if ( chunks.size() < 3 )
		{
			return false;
		}

		m_major = String::toNumber< int >(chunks[0]);
		m_minor = String::toNumber< int >(chunks[1]);
		m_revision = String::toNumber< int >(chunks[2]);

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const Version & obj)
	{
		return out << obj.m_major << '.' << obj.m_minor << '.' << obj.m_revision;
	}

	std::string
	to_string (const Version & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
