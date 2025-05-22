/*
 * src/Saphir/Declaration/OutputPrimitive.cpp
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

#include "OutputPrimitive.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Saphir::Declaration
{
	using namespace Keys;

	OutputPrimitive::OutputPrimitive (OutputPrimitiveType primitiveType, uint32_t maxVertices) noexcept
		: m_primitiveType(primitiveType), m_maxVertices(maxVertices)
	{

	}

	Key
	OutputPrimitive::name () const noexcept
	{
		switch ( m_primitiveType )
		{
			case OutputPrimitiveType::Points :
				return GLSL::Primitive::Points;

			case OutputPrimitiveType::LineStrip :
				return GLSL::Primitive::LineStrip;

			case OutputPrimitiveType::TriangleStrip :
				return GLSL::Primitive::TriangleStrip;

			default :
				return nullptr;
		}
	}

	std::string
	OutputPrimitive::sourceCode () const noexcept
	{
		std::stringstream code;

		code << GLSL::Layout << " (" << this->name() << ", " << GLSL::MaxVertices << " = " << m_maxVertices << ") " << GLSL::Out << ";" "\n";

		return code.str();
	}
}
