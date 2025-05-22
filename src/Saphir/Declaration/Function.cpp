/*
 * src/Saphir/Declaration/Function.cpp
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

#include "Function.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace EmEn::Libs;
	using namespace Keys;

	Function::Function (Key name, Key returnType) noexcept
		: m_name(name),
		m_returnType(returnType)
	{

	}

	std::string
	Function::sourceCode () const noexcept
	{
		std::stringstream code{};

		code <<
			m_returnType << ' ' << m_name << " (" << ( m_parameters.empty() ? GLSL::Void : String::implode(m_parameters, ", ") ) << ")" "\n"
			"{" "\n" <<
			this->getCode() <<
			"}" "\n";

		return code.str();
	}

	void
	Function::addInParameter (Key type, Key name, bool isConstant) noexcept
	{
		std::stringstream code;

		if ( isConstant )
		{
			code << GLSL::Const << ' ';
		}

		code << GLSL::In << ' ' << type << ' ' << name;

		m_parameters.emplace_back(code.str());
	}

	void
	Function::addOutParameter (Key type, Key name) noexcept
	{
		m_parameters.emplace_back((std::stringstream{} << GLSL::Out << ' ' << type << ' ' << name).str());
	}

	void
	Function::addInOutParameter (Key type, Key name) noexcept
	{
		m_parameters.emplace_back((std::stringstream{} << GLSL::InOut << ' ' << type << ' ' << name).str());
	}

	std::string
	Function::callCode (const std::string & parameters) const noexcept
	{
		std::stringstream code;

		code << m_name << '(' << parameters << ')';

		return code.str();
	}

	std::string
	Function::callCode (const std::vector< std::string > & parameters) const noexcept
	{
		return this->callCode(String::implode(parameters, ", "));
	}

	std::string
	Function::callCode (const BlobTrait & parameters) const noexcept
	{
		return this->callCode(parameters.get());
	}
}
