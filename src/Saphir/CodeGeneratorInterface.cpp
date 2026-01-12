/*
 * src/Saphir/CodeGeneratorInterface.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "CodeGeneratorInterface.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Saphir
{
	void
	CodeGeneratorInterface::addComment (const std::string & comment, size_t depth) noexcept
	{
		std::stringstream commentStream;
		commentStream << std::string(depth, '\t') << "/* " << comment << " */\n";

		m_instructions.emplace_back(commentStream.str());
	}

	std::string
	CodeGeneratorInterface::getCode (const std::string & prependTopInstructions, const std::string & prependOutputInstructions) const noexcept
	{
		std::stringstream code{};

		if ( !prependTopInstructions.empty() || !m_topInstructions.empty() )
		{
			code <<
				"\t" "/* Preparation instructions */" "\n" <<

				prependTopInstructions;

			for (  const auto & line : m_topInstructions )
			{
				code << line;
			}

			code << '\n';
		}

		if ( !m_instructions.empty() )
		{
			code << "\t" "/* Main instructions */" "\n";

			for (  const auto & line : m_instructions )
			{
				code << line;
			}

			code << '\n';
		}

		if ( !prependOutputInstructions.empty() || !m_outputInstructions.empty() )
		{
			code <<
				"\t" "/* Output instructions */" "\n" <<

				prependOutputInstructions;

			for (  const auto & line : m_outputInstructions )
			{
				code << line;
			}
		}

		return code.str();
	}
}
