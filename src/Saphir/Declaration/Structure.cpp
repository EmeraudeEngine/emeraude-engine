/*
 * src/Saphir/Declaration/Structure.cpp
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

#include "Structure.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Libs/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace EmEn::Libs;
	using namespace Keys;

	Structure::Structure (Key name, Key instanceName) noexcept
		: m_name(name), m_instanceName(instanceName)
	{

	}

	bool
	Structure::isValid () const noexcept
	{
		if ( m_name == nullptr )
		{
			return false;
		}

		if ( m_members.empty() )
		{
			return false;
		}

		return true;
	}

	bool
	Structure::addMember (VariableType type, Key name) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ClassId} << "This structure has already a member named '" << name << "' !";

			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name)
		);

		return true;
	}

	std::string
	Structure::sourceCode () const noexcept
	{
		std::stringstream code{};

		code << GLSL::Struct << ' ' << m_name << "\n" "{" "\n";

		for ( const auto & variable : m_members )
		{
			code << '\t' << variable.second.sourceCode() << '\n';
		}

		code << '}';

		if ( !m_instanceName.empty() )
		{
			code << ' ' << m_instanceName;
		}

		code << ";" "\n";

		return code.str();
	}
}
