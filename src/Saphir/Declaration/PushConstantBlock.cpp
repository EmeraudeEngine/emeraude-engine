/*
 * src/Saphir/Declaration/PushConstantBlock.cpp
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

#include "PushConstantBlock.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include <ranges>


#include "Libs/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace EmEn::Libs;
	using namespace Keys;

	PushConstantBlock::PushConstantBlock (Key name, Key instanceName) noexcept
		: BlockInterface(name, instanceName)
	{

	}

	bool
	PushConstantBlock::isValid () const noexcept
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

	uint32_t
	PushConstantBlock::bytes () const noexcept
	{
		uint32_t size = 0;

		for ( const auto & structure : this->structures() | std::views::values )
		{
			size += structure.bytes();
		}

		for ( const auto & pushConstant : this->members() | std::views::values )
		{
			size +=  pushConstant .bytes();
		}

		/* FIXME: Check alignment. */
		if ( this->arraySize() > 1U )
		{
			size *= this->arraySize();
		}

		return size;
	}

	bool
	PushConstantBlock::addMember (VariableType type, Key name) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ClassId} << "This push constant block has already a member named '" << name << "' !";

			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, 0)
		);

		return true;
	}

	bool
	PushConstantBlock::addArrayMember (VariableType type, Key name, uint32_t arraySize) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ClassId} << "This push constant block has already a member named '" << name << "' !";

			return false;
		}

		if ( arraySize == 0 )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, arraySize)
		);

		return true;
	}

	std::string
	PushConstantBlock::sourceCode () const noexcept
	{
		std::stringstream code;

		/* Default Std430 */
		code << GLSL::Layout << " (" << GLSL::PushConstant << ") " << GLSL::Uniform << ' ' << this->name() << "\n" "{" "\n";

		for ( const auto & pushConstant : m_members | std::views::values )
		{
			code << '\t' <<  pushConstant.sourceCode();
		}

		code << '}';

		if ( !this->instanceName().empty() )
		{
			code << ' ' << this->instanceName();
		}

		code << ";" "\n";

		return code.str();
	}
}
