/*
 * src/Saphir/Declaration/AbstractShaderBlock.cpp
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

#include "AbstractShaderBlock.hpp"

/* Local inclusions. */
#include "Libs/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace EmEn::Libs;

	const char * ShaderBlockTracerTag = "ShaderBlock";

	AbstractShaderBlock::AbstractShaderBlock (Key name, uint32_t location, Key instanceName, uint32_t arraySize) noexcept
		: BlockInterface(name, instanceName, arraySize),
		m_location(location)
	{

	}

	bool
	AbstractShaderBlock::isValid () const noexcept
	{
		if ( this->name() == nullptr )
		{
			return false;
		}

		if ( this->instanceName().empty() )
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
	AbstractShaderBlock::addMember (VariableType type, Key name, Key interpolation) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ShaderBlockTracerTag} << "This shader block has already a member named '" << name << "' !";

			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, interpolation, 0)
		);

		return true;
	}

	bool
	AbstractShaderBlock::addMember (const Structure & structure, Key interpolation) noexcept
	{
		const auto name = structure.instanceName().c_str();

		if ( Utility::contains(m_members, name) )
		{
			TraceError{ShaderBlockTracerTag} << "This shader block has already a member named '" << name << "' !";

			return false;
		}

		if ( !this->addStructure(structure.name(), structure) )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(VariableType::Structure, name, interpolation, 0)
		);

		return true;
	}

	bool
	AbstractShaderBlock::addArrayMember (VariableType type, Key name, uint32_t arraySize, Key interpolation) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ShaderBlockTracerTag} << "This shader block has already a member named '" << name << "' !";

			return false;
		}

		if ( arraySize == 0 )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, interpolation, arraySize)
		);

		return true;
	}

	bool
	AbstractShaderBlock::addArrayMember (const Structure & structure, uint32_t arraySize, Key interpolation) noexcept
	{
		const auto name = structure.instanceName().c_str();

		if ( Utility::contains(m_members, name) )
		{
			TraceError{ShaderBlockTracerTag} << "This shader block has already a member named '" << name << "' !";

			return false;
		}

		if ( arraySize == 0 )
		{
			return false;
		}

		if ( !this->addStructure(structure.name(), structure) )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(VariableType::Structure, name, interpolation, arraySize)
		);

		return true;
	}
}
