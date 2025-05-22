/*
 * src/Graphics/VertexBufferBinding.cpp
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

#include "VertexBufferBinding.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Graphics
{
	VertexBufferBinding::VertexBufferBinding (uint32_t binding, size_t elementCount, Topology topology, uint32_t bufferFlags) noexcept
		: FlagTrait(bufferFlags),
		m_binding(binding),
		m_elementCount(elementCount),
		m_topology(topology)
	{

	}

	bool
	VertexBufferBinding::operator== (const VertexBufferBinding & operand) const noexcept
	{
		if ( this != &operand )
		{
			if ( m_binding != operand.m_binding )
			{
				return false;
			}

			if ( m_elementCount != operand.m_elementCount )
			{
				return false;
			}

			if ( m_topology != operand.m_topology )
			{
				return false;
			}

			if ( this->flags() != operand.flags() )
			{
				return false;
			}
		}

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const VertexBufferBinding & obj)
	{
		return out << VertexBufferBinding::ClassId << ". "
			"Binding: " << obj.m_binding << ", "
			"Element count: " << obj.m_elementCount << ", "
			"Topology: " << to_string(obj.m_topology) << ", "
			"Per instance: " << ( obj.perInstance() ? "yes" : "no" ) << ", "
			"Request primitive restart: " << ( obj.requestPrimitiveRestart() ? "yes" : "no" ) << ", "
			"Is position absolute: " << ( obj.isPositionAbsolute() ? "yes" : "no" ) << ", "
			"Is dynamic vertex buffer: " << ( obj.isDynamicVertexBuffer() ? "yes" : "no" );
	}

	std::string
	to_string (const VertexBufferBinding & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
