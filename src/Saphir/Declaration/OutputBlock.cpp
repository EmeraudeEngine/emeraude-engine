/*
 * src/Saphir/Declaration/OutputBlock.cpp
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

#include "OutputBlock.hpp"

/* STL inclusions. */
#include <limits>
#include <ranges>
#include <sstream>

namespace EmEn::Saphir::Declaration
{
	using namespace Keys;

	OutputBlock::OutputBlock (Key name, uint32_t location, Key instanceName, uint32_t arraySize) noexcept
		: AbstractShaderBlock(name, location, instanceName, arraySize)
	{

	}

	std::string
	OutputBlock::sourceCode () const noexcept
	{
		std::stringstream code;

		/* Check if structure are requested. */
		const auto & structures = this->structures();

		if ( !structures.empty() )
		{
			for ( const auto & structure: structures | std::views::values )
			{
				code << structure.sourceCode();
			}
		}

		code <<
			GLSL::Layout << " (" << GLSL::Location << " = " << this->location() << ") " <<
			GLSL::Out << ' ' << this->name() << "\n" "{" "\n";

		for ( const auto & shaderBlock: this->members() | std::views::values )
		{
			code << '\t' << shaderBlock.sourceCode();
		}

		code << '}';

		if ( !this->instanceName().empty() )
		{
			code << ' ' << this->instanceName();

			if ( this->arraySize() == std::numeric_limits< uint32_t >::max() )
			{
				code << "[]";
			}
			else if ( this->arraySize() > 0 )
			{
				code << '[' << this->arraySize() << ']';
			}
		}

		code << ";" "\n";

		return code.str();
	}
}
