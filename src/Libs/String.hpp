/*
 * src/Libs/String.hpp
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

#pragma once

/* STL inclusions. */
#include <cstring>
#include <cmath>
#include <array>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <limits>
#include <type_traits>

/* Local inclusions for usages. */
#include "Utility.hpp"

#if IS_WINDOWS
#undef min
#undef max
#endif

namespace EmEn::Libs::String
{
	static constexpr auto Empty = "";
	static constexpr auto WhiteCharsList = " \f\n\r\t\v";

	/** @brief Enumerates how to operate on a string. */
	enum class Side : uint8_t
	{
		Right,
		Left,
		Both
	};

	/**
	 * @brief Returns an incremental label.
	 * @tparam integer_t The type of integral number. Default uint32_t.
	 * @param label The base label.
	 * @param count A reference to a counter. This will be used next to the label then incremented.
	 * @return std::string
	 */
	template< typename integer_t = uint32_t >
	[[nodiscard]]
	std::string
	numericLabel (const std::string & label, integer_t count) noexcept requires(std::is_integral_v< integer_t >)
	{
		return label + std::to_string(count);
	}

	/**
	 * @brief Returns an incremental label.
	 * @tparam integer_t The type of integral number. Default uint32_t.
	 * @param label The base label.
	 * @param count A reference to a counter. This will be used next to the label then incremented.
	 * @return std::string
	 */
	template< typename integer_t = uint32_t >
	[[nodiscard]]
	std::string
	incrementalLabel (const std::string & label, integer_t & count) noexcept requires (std::is_integral_v< integer_t >)
	{
		return label + std::to_string(count++);
	}

	/**
	 * @brief Returns a string cleaned from white char at the beginning and/or the end.
	 * @param source A reference to the input string.
	 * @param list A list of characters to remove. Default the regular white characters.
	 * @param trimType From where removing chars off the string. Default Side::Both.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string trim (const std::string & source, const char * list = WhiteCharsList, Side trimType = Side::Both) noexcept;

	/**
	 * @brief Adds a padding to a string to meet the desired size.
	 * @TODO Allow the pad parameter to be a string instead of only a char.
	 * @param source A reference to the input string.
	 * @param size The final size of the string, if the input string is greater or equals in size, the function will return the string.
	 * @param pad The char to pad with.
	 * @param padType Where to add chars to the string. Default Side::Right.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string pad (const std::string & source, size_t size, char pad = ' ', Side padType = Side::Right) noexcept;

	/**
	 * @brief Splits a string into multiple others using a separator.
	 * @param source A reference to the input string.
	 * @param separator A character to use as a separator.
	 * @param keepEmpty Keep empty part after splitting. Default true.
	 * @param limit Limits the number of split. Default MAXIMUM.
	 * @return std::vector< std::string >
	 */
	[[nodiscard]]
	std::vector< std::string > explode (const std::string & source, char separator, bool keepEmpty = true, unsigned int limit = std::numeric_limits< unsigned int >::max()) noexcept;

	/**
	 * @brief Splits a string into multiple others using a separator.
	 * @param source A reference to the input string.
	 * @param separator A reference to a string to use as a separator.
	 * @param keepEmpty Keep empty part after splitting. Default true.
	 * @param limit Limits the number of split. Default MAXIMUM.
	 * @return std::vector< std::string >
	 */
	[[nodiscard]]
	std::vector< std::string > explode (const std::string & source, const std::string & separator, bool keepEmpty = true, unsigned int limit = std::numeric_limits< unsigned int >::max()) noexcept;

	/**
	 * @brief Joins a list of string to a single string.
	 * @param list A reference to a vector of string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string implode (const std::vector< std::string > & list) noexcept;

	/**
	 * @brief Joins a list of string to a single string using a delimiter between parts.
	 * @tparam delimiter_t The type of value to insert as delimiter.
	 * @param list A reference to a vector of string.
	 * @param delimiter A reference to a value to insert between each part.
	 * @return std::string
	 */
	template< typename delimiter_t >
	[[nodiscard]]
	std::string implode (const std::vector< std::string > & list, const delimiter_t & delimiter) noexcept
	{
		std::stringstream output;

		for ( const auto & item : list )
		{
			if ( item.empty() )
			{
				continue;
			}

			if ( output.tellp() == 0 )
			{
				output << item;
			}
			else
			{
				output << delimiter << item;
			}
		}

		return output.str();
	}

	/**
	 * @brief Returns the string in upper case.
	 * @param source A reference to the input string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string toUpper (const std::string & source) noexcept;

	/**
	 * @brief Returns the string in lower case.
	 * @param source A reference to the input string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string toLower (const std::string & source) noexcept;

	/**
	 * @brief Returns the string with the first char in upper case.
	 * @param source A reference to the input string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string ucfirst (const std::string & source) noexcept;

	/**
	 * @brief Replaces a character by another one inside a string.
	 * @param search A character.
	 * @param replace A character.
	 * @param input A reference to the target string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string replace (char search, char replace, const std::string & input) noexcept;

	/**
	 * @brief Replaces a character by another one inside a string.
	 * @param search A character.
	 * @param replace A character.
	 * @param input A reference to the target string.
	 * @param limit The limit of search occurrence.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string replace (char search, char replace, const std::string & input, size_t limit) noexcept;

	/**
	 * @brief Replaces a piece of string by another one inside a string.
	 * @param search A reference to a string to seek the part to replace.
	 * @param replace A reference to a string to set the replacing part.
	 * @param input A reference to the target string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string replace (const std::string & search, const std::string & replace, const std::string & input) noexcept;

	/**
	 * @brief Replaces a piece of string by another one inside a string.
	 * @param search A reference to a string to seek the part to replace.
	 * @param replace A reference to a string to set the replacing part.
	 * @param input A reference to the target string.
	 * @param limit The limit of search occurrence.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string replace (const std::string & search, const std::string & replace, const std::string & input, size_t limit) noexcept;

	/**
	 * @brief Removes every occurrence of a character in a string and returns it.
	 * @param source A reference to the input string.
	 * @param character The character to remove.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string removeChars (const std::string & source, char character) noexcept;

	/**
	 * @brief Removes every occurrence from the character list in a string and returns it.
	 * @param source A reference to the input string.
	 * @param characters The character to remove.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string removeChars (const std::string & source, const std::string & characters) noexcept;

	/**
	 * @brief Removes the file extension in string representing a filepath.
	 * @param input A reference to a string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string removeFileExtension (const std::string & input) noexcept;

	/**
	 * @brief Extracts the filename from a filepath.
	 * @param input A reference to a string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string extractFilename (const std::string & input) noexcept;

	/**
	 * @brief Returns a string with only numbers present in the input string.
	 * @param input A reference to a string.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string extractNumbers (const std::string & input) noexcept;

	/**
	 * @brief Extracts a list of tag from a string.
	 * @param input A reference to a string.
	 * @param delimiters A reference to an array of two chars to set the tag delimiter. Default '{', '}'.
	 * @param removeDelimiters Do not include the delimiters with the extracted tags. Default false.
	 * @return std::vector< std::string >
	 */
	[[nodiscard]]
	std::vector< std::string > extractTags (const std::string & input, const std::array< char, 2 > & delimiters = {'{', '}'}, bool removeDelimiters = false) noexcept;

	/**
	 * @brief Returns the left part of match in source string.
	 * @param source A reference to the string.
	 * @param match A reference to a string for the matching part.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string left (const std::string & source, const std::string & match) noexcept;

	/**
	 * @brief Returns the right part of match in source string.
	 * @param source A reference to the string.
	 * @param match A reference to a string for the matching part.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string right (const std::string & source, const std::string & match) noexcept;

#if __cplusplus < 202002L /* C++20 feature */
	/**
	 * @brief Returns whether the string starts with a piece of another string.
	 * @param source A reference to the string.
	 * @param piece A reference to a string for the matching part.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	startsWith (const std::string & source, const std::string & piece) noexcept
	{
		return source.rfind(piece, 0) == 0;
	}

	/**
	 * @brief Returns whether the string ends with a piece of another string.
	 * @param source A reference to the string.
	 * @param piece A reference to a string for the matching part.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	endsWith (const std::string & source, const std::string & piece) noexcept
	{
		return source.find(piece, 0) == source.size() - piece.size();
	}
#endif

#if __cplusplus < 202302L /* C++23 feature */
	/**
	 * @brief Returns whether a piece of string is part of the source string.
	 * @param source A reference to the string.
	 * @param piece A reference to a string for the matching part.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	contains (const std::string & source, const std::string & piece) noexcept
	{
		return source.find(piece) != std::string::npos;
	}
#endif

	/**
	 * @brief Returns a multibyte character stored in a string from Unicode code.
	 * @param unicode The code of the character in the Unicode (BE).
	 * @return std::string
	 */
	[[nodiscard]]
	std::string unicodeToUTF8 (unsigned int unicode) noexcept;

	/**
	 * @brief Converts a string into a number.
	 * @tparam number_t The type of number.
	 * @warning This function is intended to be used within a number based template.
	 * If you know exactly the type of number you need, consider using the STL functions directly.
	 * @param stringValue A reference to a string.
	 * @return number_t
	 */
	template< typename number_t >
	[[nodiscard]]
	number_t
	toNumber (const std::string & stringValue) noexcept requires (std::is_arithmetic_v< number_t >)
	{
		if constexpr ( std::is_same_v< number_t, float > )
		{
			return std::stof(stringValue, nullptr);
		}

		if constexpr ( std::is_same_v< number_t, double > )
		{
			return std::stod(stringValue, nullptr);
		}

		if constexpr ( std::is_same_v< number_t, long double > )
		{
			return std::stold(stringValue, nullptr);
		}

		/* NOTE: Same as 'int8_t', 'int16_t' */
		/* WARNING: 'signed char' !== 'char' */
		if constexpr ( std::is_same_v< number_t, signed char > || std::is_same_v< number_t, char > || std::is_same_v< number_t, short int > )
		{
			const int integer = std::stoi(stringValue, nullptr, 10);

			if ( integer < std::numeric_limits< number_t >::min() )
			{
				return std::numeric_limits< number_t >::min();
			}

			if ( integer > std::numeric_limits< number_t >::max() )
			{
				return std::numeric_limits< number_t >::max();
			}

			return static_cast< number_t >(integer);
		}

		/* NOTE: Same as 'int32_t' */
		if constexpr ( std::is_same_v< number_t, int > )
		{
			return std::stoi(stringValue, nullptr, 10);
		}

		/* NOTE: Same as 'int64_t' */
		if constexpr ( std::is_same_v< number_t, long int > )
		{
			return std::stol(stringValue, nullptr, 10);
		}

		/* NOTE: Same as 'int128_t' */
		if constexpr ( std::is_same_v< number_t, long long int > )
		{
			return std::stoll(stringValue, nullptr, 10);
		}

		/* NOTE: Same as 'uint8_t', 'uint16_t', 'uint32_t' */
		if constexpr ( std::is_same_v< number_t, unsigned char > || std::is_same_v< number_t, unsigned short int > || std::is_same_v< number_t, unsigned int > )
		{
			const unsigned long int integer = std::stoul(stringValue, nullptr, 10);

			if ( integer > std::numeric_limits< number_t >::max() )
			{
				return std::numeric_limits< number_t >::max();
			}

			return static_cast< number_t >(integer);
		}

		/* NOTE: Same as 'uint64_t' */
		if constexpr ( std::is_same_v< number_t, unsigned long int > )
		{
			return std::stoul(stringValue, nullptr, 10);
		}

		/* NOTE: Same as 'uint128_t' */
		if constexpr ( std::is_same_v< number_t, unsigned long long int > )
		{
			return std::stoull(stringValue, nullptr, 10);
		}

		return 0;
	}

	/**
 	 * @brief Converts a floating point number into a boolean.
	 * @tparam number_t The type of floating point number. Default float.
	 * @param value The value to converts.
	 * @return bool
	 */
	template< typename number_t = float >
	[[nodiscard]]
	bool
	toBool (number_t value) noexcept requires (std::is_floating_point_v< number_t >)
	{
		return !Utility::isZero(value);
	}

	/**
	 * @brief Converts an integer into a boolean.
	 * @tparam integer_t The type of integral number. Default int.
	 * @param value The value to converts.
	 * @return bool
	 */
	template< typename integer_t = int >
	[[nodiscard]]
	bool
	toBool (integer_t value) noexcept requires (std::is_integral_v< integer_t >)
	{
		return value != 0;
	}

	/**
	 * @brief Converts a string into a boolean.
	 * @param value A reference to a string.
	 * @return bool
	 */
	[[nodiscard]]
	bool toBool (std::string value) noexcept;

	/**
	 * @brief Concatenates two C-strings.
	 * @param strA A C-String
	 * @param strB A C-String
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	concat (const char * strA, const char * strB) noexcept
	{
		return std::string{strA} + std::string{strB};
	}

	/**
	 * @brief Concatenates something to a string.
	 * @tparam data_t The type of data.
	 * @param str A C-String
	 * @param append A value to appends
	 * @return std::string
	 */
	template< typename data_t >
	[[nodiscard]]
	std::string
	concat (const char * str, data_t append) noexcept
	{
		return std::string{str} + std::to_string(append);
	}

	/**
	 * @brief Concatenates something to a string.
	 * @tparam data_t The type of data.
	 * @param str A reference to a string.
	 * @param append A value to appends
	 * @return std::string
	 */
	template< typename data_t >
	[[nodiscard]]
	std::string
	concat (const std::string & str, data_t append) noexcept
	{
		return str.append(std::to_string(append));
	}

	/**
	 * @brief Appends something to a string.
	 * @tparam data_t The type of data.
	 * @param str A C-String
	 * @param append A value to appends
	 * @return std::string
	 */
	template< typename data_t >
	[[nodiscard]]
	std::string
	append (const char * str, data_t append) noexcept
	{
		return std::string(str) + std::to_string(append);
	}

	/**
	 * @brief Appends something to a string.
	 * @tparam data_t The type of data.
	 * @param str A reference to a string.
	 * @param append A value to appends
	 * @return std::string
	 */
	template< typename data_t >
	[[nodiscard]]
	std::string &
	append (std::string & str, data_t append) noexcept
	{
		//return str += std::to_string(append);
		return str.append(std::to_string(append));
	}

	/**
	 * @brief Converts a type to a string.
	 * @tparam data_t The type of data.
	 * @param value The value to convert.
	 * @return std::string
	 */
	template< typename data_t >
	[[nodiscard]]
	std::string
	to_string (data_t value) noexcept
	{
		return std::to_string(value);
	}

	/**
	 * @brief Converts a pointer to a string.
	 * @param value A pointer.
	 * @return std::string
	 */
	template<>
	[[maybe_unused]]
	[[nodiscard]]
	inline
	std::string
	to_string< void * > (void * value) noexcept
	{
		return '@' + std::to_string(reinterpret_cast< std::intptr_t >(value));
	}

	/**
	 * @brief Converts a const pointer to a string.
	 * @param value A pointer.
	 * @return std::string
	 */
	template<>
	[[maybe_unused]]
	[[nodiscard]]
	inline
	std::string
	to_string< const void * > (const void * value) noexcept
	{
		return '@' + std::to_string(reinterpret_cast< std::intptr_t >(value));
	}

	/**
	 * @brief Converts a boolean to a string.
	 * @param value A boolean.
	 * @return std::string
	 */
	template<>
	[[maybe_unused]]
	[[nodiscard]]
	inline
	std::string
	to_string (bool value) noexcept
	{
		return value ? "true" : "false";
	}

	/**
	 * @brief Converts a thread id to a string.
	 * @param value A thread id.
	 * @return std::string
	 */
	template<>
	[[maybe_unused]]
	[[nodiscard]]
	inline
	std::string
	to_string< std::thread::id > (std::thread::id value) noexcept
	{
		std::stringstream output;

		output << value;

		return output.str();
	}

	/**
	 * @brief Serializes a vector to a string.
	 * @tparam data_t The type of data in the vector.
	 * @param source A reference to a vector.
	 * @return std::string
	 */
	template< typename data_t >
	[[nodiscard]]
	std::string
	serializeVector (const std::vector< data_t > & source)
	{
		return {reinterpret_cast< const char * >(source.data()), source.size() * sizeof(data_t)};
	}

	/**
	 * @brief Deserializes a string to a vector.
	 * @tparam data_t The type of data in the vector.
	 * @param source A reference to a string
	 * @return std::vector< data_t >
	 */
	template< typename data_t >
	[[nodiscard]]
	std::vector< data_t >
	deserializeVector (const std::string & source)
	{
		std::vector< data_t > recoveredData;
		recoveredData.resize(source.size() / sizeof(data_t));

		std::memcpy(recoveredData.data(), source.data(), source.size());

		return recoveredData;
	}
}
