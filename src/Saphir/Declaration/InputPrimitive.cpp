/*
 * src/Saphir/Declaration/InputPrimitive.cpp
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

#include "InputPrimitive.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Saphir::Declaration
{
	using namespace Keys;

	InputPrimitive::InputPrimitive (InputPrimitiveType primitiveType) noexcept
		: m_primitiveType(primitiveType)
	{

	}

	Key
	InputPrimitive::name () const noexcept
	{
		switch ( m_primitiveType )
		{
			case InputPrimitiveType::Points :
				return GLSL::Primitive::Points;

			case InputPrimitiveType::Lines :
				return GLSL::Primitive::Lines;

			case InputPrimitiveType::LineAdjacency :
				return GLSL::Primitive::LineAdjacency;

			case InputPrimitiveType::Triangles :
				return GLSL::Primitive::Triangles;

			case InputPrimitiveType::TrianglesAdjacency :
				return GLSL::Primitive::TrianglesAdjacency;

			default :
				return nullptr;
		}
	}

	std::string
	InputPrimitive::sourceCode () const noexcept
	{
		std::stringstream code;

		code << GLSL::Layout << " (" << this->name() << ") " << GLSL::In << ";" "\n";

		return code.str();
	}
}
