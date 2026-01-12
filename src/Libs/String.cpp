/*
 * src/Libs/String.cpp
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

#include "String.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>

namespace EmEn::Libs::String
{
	std::string
	trim (const std::string & source, const char * list, Side trimType) noexcept
	{
		switch ( trimType )
		{
			case Side::Right:
			{
				const auto end = source.find_last_not_of(list);

				if ( end == std::string::npos )
				{
					return {};
				}

				return {source, 0, end + 1};
			}

			case Side::Left:
			{
				const auto start = source.find_first_not_of(list);

				if ( start == std::string::npos )
				{
					return {};
				}

				return {source, start, source.size() - start};
			}

			case Side::Both:
			{
				const auto start = source.find_first_not_of(list);
				const auto end = source.find_last_not_of(list);

				if ( start == std::string::npos || end == std::string::npos )
				{
					return {};
				}

				return {source, start, end - start + 1};
			}
		}

		return source;
	}

	std::string
	pad (const std::string & source, size_t size, char pad, Side padType) noexcept
	{
		if ( source.size() >= size )
		{
			return source;
		}

		switch ( padType )
		{
			case Side::Right:
				return source + std::string(size - source.size(), pad);

			case Side::Left:
				return std::string(size-source.size(), pad) + source;

			case Side::Both:
			{
				const auto count = size - source.size();

				std::string copy{source};

				for ( size_t i = 0; i < count; i++ )
				{
					copy.insert((i % 2) != 0 ? copy.begin() : copy.end(), pad);
				}

				return copy;
			}

			default:
				return source;
		}
	}

	std::vector< std::string >
	explode (const std::string & source, char separator, bool keepEmpty, unsigned int limit) noexcept
	{
		std::vector< std::string > list{};

		/* Special case: empty string should return a vector with one empty string */
		if ( source.empty() )
		{
			return {""};
		}

		size_t startOffset = 0;
		size_t endOffset = 0;

		while ( (endOffset = source.find(separator, startOffset)) != std::string::npos )
		{
			const std::string chunk{source, startOffset, endOffset - startOffset};

			if ( keepEmpty || !chunk.empty() )
			{
				list.emplace_back(chunk);
			}

			startOffset = endOffset + 1;

			if ( list.size() >= limit )
			{
				break;
			}
		}

		/* Adds the end of the string if exists. */
		if ( startOffset != source.size() )
		{
			const std::string chunk{source, startOffset, source.size() - startOffset};

			if ( keepEmpty || !chunk.empty() )
			{
				list.emplace_back(chunk);
			}
		}

		return list;
	}

	std::vector< std::string >
	explode (const std::string & source, const std::string & separator, bool keepEmpty, unsigned int limit) noexcept
	{
		std::vector< std::string > list{};

		/* Special case: empty string should return a vector with one empty string */
		if ( source.empty() )
		{
			return {""};
		}

		size_t startOffset = 0;
		size_t endOffset = 0;

		while ( (endOffset = source.find(separator, startOffset)) != std::string::npos )
		{
			const std::string chunk{source, startOffset, endOffset - startOffset};

			if ( keepEmpty || !chunk.empty() )
			{
				list.emplace_back(chunk);
			}

			startOffset = endOffset + separator.size();

			if ( list.size() >= limit )
			{
				break;
			}
		}

		/* Adds the end of the string if exists. */
		if ( startOffset != source.size() )
		{
			const std::string chunk{source, startOffset, source.size() - startOffset};

			if ( keepEmpty || !chunk.empty() )
			{
				list.emplace_back(chunk);
			}
		}

		return list;
	}

	std::string
	implode (const std::vector< std::string > & list) noexcept
	{
		std::string output{};

		for ( const auto & chunk : list )
		{
			output.append(chunk);
		}

		return output;
	}

	std::string
	toUpper (const std::string & source) noexcept
	{
		std::string output{};

		const auto length = source.size();

		output.resize(length);

		for ( size_t i = 0; i < length; ++i )
		{
			output[i] = static_cast< char >(std::toupper(source[i]));
		}

		return output;
	}

	std::string
	toLower (const std::string & source) noexcept
	{
		std::string output{};

		const auto length = source.size();

		output.resize(length);

		for ( size_t i = 0; i < length; ++i )
		{
			output[i] = static_cast< char >(std::tolower(source[i]));
		}

		return output;
	}

	std::string
	ucfirst (const std::string & source) noexcept
	{
		std::string output{source};

		output[0] = static_cast< char >(std::toupper(output[0]));

		return output;
	}

	std::string
	replace (char search, char replace, const std::string & input, size_t limit) noexcept
	{
		std::string output{input};

		/* limit = 0 means replace all */
		if ( limit == 0 )
		{
			for ( auto & character : output )
			{
				if ( character == search )
				{
					character = replace;
				}
			}

			return output;
		}

		/* Replace up to limit occurrences */
		for ( auto & character : output )
		{
			if ( character != search )
			{
				continue;
			}

			character = replace;

			limit--;

			if ( limit == 0 )
			{
				break;
			}
		}

		return output;
	}

	std::string
	replace (const std::string & search, const std::string & replace, const std::string & input, size_t limit) noexcept
	{
		std::string output{input};

		size_t pos = 0;

		/* limit = 0 means replace all */
		if ( limit == 0 )
		{
			while ( ( pos = output.find(search, pos) ) != std::string::npos )
			{
				output.replace(pos, search.length(), replace);

				pos += replace.length();
			}

			return output;
		}

		/* Replace up to limit occurrences */
		while ( limit > 0 && ( pos = output.find(search, pos) ) != std::string::npos )
		{
			limit--;

			output.replace(pos, search.length(), replace);

			pos += replace.length();
		}

		return output;
	}

	std::string
	removeChars (const std::string & source, char character) noexcept
	{
		std::string output{};
		output.reserve(source.size());

		for ( auto inputCharacter : source )
		{
			if ( inputCharacter == character )
			{
				continue;
			}

			output += inputCharacter;
		}

		return output;
	}

	std::string
	removeChars (const std::string & source, const std::string & characters) noexcept
	{
		std::string output{};
		output.reserve(source.size());

		for ( auto input : source )
		{
#if __cplusplus >= 202002L /* C++20 feature */
			if ( std::ranges::any_of(characters, [input] (auto chr) {return input == chr;}) )
			{
				continue;
			}
#else
			if ( std::any_of(characters.cbegin(), characters.cend(), [input] (auto chr) {return input == chr;}) )
			{
				continue;
			}
#endif

			output += input;
		}

		return output;
	}

	std::string
	removeFileExtension (const std::string & input) noexcept
	{
		const auto position = input.find_last_of('.');

		if ( position != std::string::npos )
		{
			return input.substr(0, position);
		}

		return input;
	}

	std::string
	extractFilename (const std::string & input) noexcept
	{
		const auto position = input.find_last_of('/');

		if ( position != std::string::npos )
		{
			return input.substr(position + 1);
		}

		return input;
	}

	std::string
	extractNumbers (const std::string & input) noexcept
	{
		std::string output;
		output.reserve(input.size());

		bool needSpace = false;

		for ( const auto chr : input )
		{
			switch ( chr )
			{
				case '0' :
				case '1' :
				case '2' :
				case '3' :
				case '4' :
				case '5' :
				case '6' :
				case '7' :
				case '8' :
				case '9' :
				case '.' :
					output += chr;

					needSpace = true;
					break;

				default :
					if ( needSpace )
					{
						output += ' ';

						needSpace = false;
					}
					break;
			}
		}

		if ( output.back() == ' ' )
		{
			output.resize(output.size() - 1);
		}

		return output;
	}

	bool
	toBool (std::string value) noexcept
	{
		value = toLower(value);

		if ( value == "false" )
		{
			return false;
		}

		if ( value == "off" )
		{
			return false;
		}

		if ( value == "none" )
		{
			return false;
		}

		if ( value == "0" )
		{
			return false;
		}

		return true;
	}

	std::string
	left (const std::string & source, const std::string & match) noexcept
	{
		/* Empty match means everything is "to the left" */
		if ( match.empty() )
		{
			return source;
		}

		const auto pos = source.find(match);

		if ( pos == std::string::npos )
		{
			return source;
		}

		return source.substr(0, pos);
	}

	std::string
	right (const std::string & source, const std::string & match) noexcept
	{
		const auto pos = source.find(match);

		if ( pos == std::string::npos )
		{
			return {};
		}

		return source.substr(pos + match.size());
	}

	std::string
	unicodeToUTF8 (unsigned int unicode) noexcept
	{
		std::string utf8{};

		if ( unicode <= 0x7f )
		{
			utf8.append(1, static_cast< char >(unicode));
		}
		else if ( unicode <= 0x7ff )
		{
			utf8.append(1, static_cast< char >(0xc0 | ((unicode >> 6) & 0x1f)));
			utf8.append(1, static_cast< char >(0x80 | (unicode & 0x3f)));
		}
		else if ( unicode <= 0xffff )
		{
			utf8.append(1, static_cast< char >(0xe0 | ((unicode >> 12) & 0x0f)));
			utf8.append(1, static_cast< char >(0x80 | ((unicode >> 6) & 0x3f)));
			utf8.append(1, static_cast< char >(0x80 | (unicode & 0x3f)));
		}
		else
		{
			utf8.append(1, static_cast< char >(0xf0 | ((unicode >> 18) & 0x07)));
			utf8.append(1, static_cast< char >(0x80 | ((unicode >> 12) & 0x3f)));
			utf8.append(1, static_cast< char >(0x80 | ((unicode >> 6) & 0x3f)));
			utf8.append(1, static_cast< char >(0x80 | (unicode & 0x3f)));
		}

		return utf8;
	}

	std::vector< std::string >
	extractTags (const std::string & input, const std::array< char, 2 > & delimiters, bool removeDelimiters) noexcept
	{
		std::vector< std::string > list;

		size_t lastOffset = 0;

		while ( true )
		{
			const auto startPos = input.find_first_of(delimiters[0], lastOffset);

			if ( startPos == std::string::npos )
			{
				break;
			}

			lastOffset = input.find_first_of(delimiters[1], startPos);

			if ( lastOffset == std::string::npos )
			{
				break;
			}

			const auto offset = startPos + 1;

			if ( removeDelimiters )
			{
				list.emplace_back(input.substr(offset, lastOffset - offset));
			}
			else
			{
				list.emplace_back(input.substr(offset - 1, lastOffset - offset + 2));
			}
		}

		return list;
	}
}
