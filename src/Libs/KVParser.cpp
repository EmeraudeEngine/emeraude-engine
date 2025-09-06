/*
 * src/Libs/KVParser.cpp
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

#include "KVParser.hpp"

namespace EmEn::Libs
{
	std::string
	KVParser::parseSectionTitle (const std::string & line) noexcept
	{
		const auto start = line.find_first_of('[');
		const auto length = line.find_last_of(']');

		if ( start != std::string::npos && length != std::string::npos )
		{
			return {line, start + 1, length - 1};
		}

		return {};
	}

	KVParser::LineType
	KVParser::getLineType (const std::string & line) noexcept
	{
		for ( const auto character : line )
		{
			switch ( character )
			{
				case '@' :
					return LineType::Headers;

				case '[' :
					return LineType::SectionTitle;

				case '#' :
					return LineType::Comment;

				case '=' :
					return LineType::Definition;

				default :
					/* We don't care about this char ... */
					break;
			}
		}

		return LineType::None;
	}

	KVSection &
	KVParser::section (const std::string & label) noexcept
	{
		if ( const auto sectionIt = m_sections.find(label); sectionIt != m_sections.cend() )
		{
			return sectionIt->second;
		}

		return m_sections[label];
	}

	bool
	KVParser::read (const std::string & filepath) noexcept
	{
		if ( std::ifstream file{filepath}; file.is_open() )
		{
			std::string line;

			/* This is the default section. */
			auto * currentSection = &this->section("main");

			/* Count the sections. */
			while ( std::getline(file, line) )
			{
				switch ( KVParser::getLineType(line) )
				{
					case LineType::SectionTitle :
						if ( auto sectionName = KVParser::parseSectionTitle(line); !sectionName.empty() )
						{
							currentSection = &this->section(sectionName);
						}
						break;

					case LineType::Definition :
						if ( auto equalSignPosition = line.find_first_of('='); equalSignPosition != std::string::npos )
						{
							auto key = String::trim(line.substr(0, equalSignPosition));
							auto value = String::trim(line.substr(equalSignPosition + 1));

							currentSection->addVariable(key, KVVariable{value});
						}
						break;

					case LineType::None :
					case LineType::Headers :
					case LineType::Comment :
						break;
				}
			}

			file.close();

			/* Indicates the parser state. */
			return true;
		}

		return false;
	}

	bool
	KVParser::write (const std::string & filepath) const noexcept
	{
		if ( std::ofstream file{filepath, std::ios::out | std::ios::trunc}; file.is_open() )
		{
			for ( const auto & [sectionName, section] : m_sections )
			{
				file << "[" << sectionName << "]" "\n";

				for ( const auto & [variableName, variable] : section.variables() )
				{
					file << variableName << " = " << variable.asString() << "\n";
				}

				file << "\n";
			}

			file.close();

			return true;
		}

		return false;
	}
}
