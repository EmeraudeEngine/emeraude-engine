/*
 * src/Saphir/Declaration/UniformBlock.cpp
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

#include "UniformBlock.hpp"

/* STL inclusions. */
#include <ranges>
#include <sstream>

namespace EmEn::Saphir::Declaration
{
	using namespace Saphir::Keys;

	std::string
	UniformBlock::sourceCode () const noexcept
	{
		std::stringstream code;

		/* Check if structure declarations are requested. */
		const auto & structureDeclarations = this->structureDeclaration();

		if ( !structureDeclarations.empty() )
		{
			for ( const auto & declaration : structureDeclarations | std::views::values )
			{
				code << declaration.sourceCode();
			}
		}

		code << this->getLayoutQualifier() << GLSL::Uniform << ' ' << this->name() << "\n" "{" "\n";

		for ( const auto & [name, bufferBackedBlock] : this->members() )
		{
			/* NOTE: Wizardy for structures, which is not a regular buffer backed block member.
			 * Go to AbstractBufferBackedBlock::addArrayMember() for declaration. */
			if ( bufferBackedBlock.type() == VariableType::Structure )
			{
				code << '\t' << name << ' ' << bufferBackedBlock.name();

				if ( bufferBackedBlock.arraySize() > 0 )
				{
					code << '[' << bufferBackedBlock.arraySize() << ']';
				}

				code << ";" "\n";
			}
			else
			{
				code << '\t' << bufferBackedBlock.sourceCode();
			}
		}

		code << '}';

		if ( !this->instanceName().empty() )
		{
			code << ' ' << this->instanceName();

			if ( this->arraySize() > 0 )
			{
				code << '[' << this->arraySize() << ']';
			}
		}

		code << ";" "\n";

		return code.str();
	}
}
