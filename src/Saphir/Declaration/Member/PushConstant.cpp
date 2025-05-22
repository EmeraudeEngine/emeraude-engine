/*
 * src/Saphir/Declaration/Member/PushConstant.cpp
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

#include "PushConstant.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Saphir::Declaration::Member
{
	PushConstant::PushConstant (VariableType type, Key name, uint32_t arraySize) noexcept
		: m_type(type),
		m_name(name),
		m_arraySize(arraySize)
	{

	}

	uint32_t
	PushConstant::bytes () const noexcept
	{
		/* FIXME : Check alignment with array size. */
		if ( m_arraySize > 1 )
		{
			return size_bytes(m_type) * m_arraySize;
		}

		return size_bytes(m_type);
	}

	std::string
	PushConstant::sourceCode () const noexcept
	{
		std::stringstream code;

		code << to_cstring(m_type) << ' ' << m_name;

		if ( m_arraySize > 0 )
		{
			code << '[' << m_arraySize << ']';
		}

		code << ";" "\n";

		return code.str();
	}
}
