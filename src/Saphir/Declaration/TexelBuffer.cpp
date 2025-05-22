/*
 * src/Saphir/Declaration/TexelBuffer.cpp
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

#include "TexelBuffer.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Saphir::Declaration
{
	using namespace Keys;

	TexelBuffer::TexelBuffer (uint32_t set, uint32_t binding, Key type, Key name) noexcept
		: m_type(type),
		m_name(name),
		m_set(set),
		m_binding(binding)
	{

	}

	bool
	TexelBuffer::isValid () const noexcept
	{
		if ( m_type == nullptr )
		{
			return false;
		}

		if ( m_name == nullptr )
		{
			return false;
		}

		return true;
	}

	std::string
	TexelBuffer::sourceCode () const noexcept
	{
		std::stringstream code;

		code <<
			GLSL::Layout << " (" << GLSL::Set << " = " << m_set << ", " << GLSL::Binding << " = " << m_binding << ") " <<
			GLSL::Uniform << ' ' << m_type << ' ' << m_name << ";" "\n";

		return code.str();
	}
}
