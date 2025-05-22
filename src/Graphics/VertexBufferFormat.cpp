/*
 * src/Graphics/VertexBufferFormat.cpp
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

#include "VertexBufferFormat.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;

	bool
	VertexBufferFormat::operator== (const VertexBufferFormat & operand) const noexcept
	{
		if ( this == &operand )
		{
			return true;
		}

		if ( m_bindings.size() == operand.m_bindings.size() )
		{
			const auto limit = m_bindings.size();

			for ( size_t index = 0; index < limit; index++ )
			{
				if ( m_bindings[index].binding != operand.m_bindings[index].binding )
				{
					return false;
				}

				if ( m_bindings[index].stride != operand.m_bindings[index].stride )
				{
					return false;
				}

				if ( m_bindings[index].inputRate != operand.m_bindings[index].inputRate )
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}

		if ( m_attributes.size() == operand.m_attributes.size() )
		{
			const auto limit = m_attributes.size();

			for ( size_t index = 0; index < limit; index++ )
			{
				if ( m_attributes[index].location != operand.m_attributes[index].location )
				{
					return false;
				}

				if ( m_attributes[index].binding != operand.m_attributes[index].binding )
				{
					return false;
				}

				if ( m_attributes[index].format != operand.m_attributes[index].format )
				{
					return false;
				}

				if ( m_attributes[index].offset != operand.m_attributes[index].offset )
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}

		if ( m_bindingFormats != operand.m_bindingFormats )
		{
			return false;
		}

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const VertexBufferFormat & obj)
	{
		out << "Vertex buffer format @" << &obj << "\n";

		if ( obj.m_bindingFormats.empty() )
		{
			out << " - Binding formats : None !" "\n";
		}
		else
		{
			out << " - Binding formats :" "\n";

			for ( const auto & [index, binding] : obj.m_bindingFormats )
			{
				out << "\t" << "Binding index:" << index << " (" << binding << ")\n";
			}
		}

		if ( obj.m_bindings.empty() )
		{
			out << " - Binding descriptions : None !" "\n";
		}
		else
		{
			out << " - Binding descriptions :" "\n";

			for ( const auto & [binding, stride, inputRate] : obj.m_bindings )
			{
				out << "\t" <<
					"Binding point:" << binding << ", "
					"stride:" << stride << ", "
					"input rate: " << inputRate << "\n";
			}
		}

		if ( obj.m_attributes.empty() )
		{
			out << " - Attribute descriptions : None !" "\n";
		}
		else
		{
			out << " - Attribute descriptions :" "\n";

			for ( const auto & [location, binding, format, offset] : obj.m_attributes )
			{
				out << "\t" <<
					"Attribute location:" << location << " (" << to_cstring(static_cast< VertexAttributeType >(location)) << "), "
					"binding:" << binding << ", "
					"format:" << format << ", "
					"offset:" << offset << "\n";
			}
		}

		return out << "\n";
	}

	std::string
	to_string (const VertexBufferFormat & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
