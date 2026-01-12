/*
 * src/Libs/TokenFormatter.cpp
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

#include "TokenFormatter.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cctype>
#include <cstring>

namespace EmEn::Libs
{
	TokenFormatter::TokenFormatter (std::string_view token) noexcept
	{
		/* Copy token to internal buffer (truncate if too long). */
		m_bufferLength = std::min(token.size(), MaxTokenLength);
		std::memcpy(m_buffer.data(), token.data(), m_bufferLength);

		m_detectedStyle = detectStyle(std::string_view{m_buffer.data(), m_bufferLength});
		parse();
	}

	CaseStyle
	TokenFormatter::detectedStyle () const noexcept
	{
		return m_detectedStyle;
	}

	const TokenFormatter::WordStorage &
	TokenFormatter::words () const noexcept
	{
		return m_words;
	}

	size_t
	TokenFormatter::wordCount () const noexcept
	{
		return m_wordCount;
	}

	bool
	TokenFormatter::empty () const noexcept
	{
		return m_wordCount == 0;
	}

	size_t
	TokenFormatter::totalWordLength () const noexcept
	{
		size_t total = 0;

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			total += m_words[i].size();
		}

		return total;
	}

	std::string
	TokenFormatter::toCamelCase () const noexcept
	{
		if ( m_wordCount == 0 )
		{
			return {};
		}

		std::string result;
		result.reserve(totalWordLength());

		appendLower(result, m_words[0]);

		for ( size_t i = 1; i < m_wordCount; ++i )
		{
			appendCapitalized(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toPascalCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength());

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			appendCapitalized(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toSnakeCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += '_';
			}

			appendLower(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toScreamingSnake () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += '_';
			}

			appendUpper(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toKebabCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += '-';
			}

			appendLower(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toTrainCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += '-';
			}

			appendUpper(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toFlatCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength());

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			appendLower(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toUpperFlatCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength());

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			appendUpper(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toLowerSpaced () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += ' ';
			}

			appendLower(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toUpperSpaced () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += ' ';
			}

			appendUpper(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::toTitleCase () const noexcept
	{
		std::string result;
		result.reserve(totalWordLength() + m_wordCount);

		for ( size_t i = 0; i < m_wordCount; ++i )
		{
			if ( i > 0 )
			{
				result += ' ';
			}

			appendCapitalized(result, m_words[i]);
		}

		return result;
	}

	std::string
	TokenFormatter::to (CaseStyle targetStyle) const noexcept
	{
		switch ( targetStyle )
		{
			case CaseStyle::CamelCase:
				return toCamelCase();

			case CaseStyle::PascalCase:
				return toPascalCase();

			case CaseStyle::SnakeCase:
				return toSnakeCase();

			case CaseStyle::ScreamingSnake:
				return toScreamingSnake();

			case CaseStyle::KebabCase:
				return toKebabCase();

			case CaseStyle::TrainCase:
				return toTrainCase();

			case CaseStyle::FlatCase:
				return toFlatCase();

			case CaseStyle::UpperFlatCase:
				return toUpperFlatCase();

			case CaseStyle::LowerSpaced:
				return toLowerSpaced();

			case CaseStyle::UpperSpaced:
				return toUpperSpaced();

			case CaseStyle::TitleCase:
				return toTitleCase();

			case CaseStyle::Unknown:
			default:
				/* Return concatenated words without formatting. */
				{
					std::string result;
					result.reserve(totalWordLength());

					for ( size_t i = 0; i < m_wordCount; ++i )
					{
						result += m_words[i];
					}

					return result;
				}
		}
	}

	/* Static methods implementation. */

	CaseStyle
	TokenFormatter::detect (std::string_view source) noexcept
	{
		return detectStyle(source);
	}

	std::string
	TokenFormatter::toCamelCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toCamelCase();
	}

	std::string
	TokenFormatter::toPascalCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toPascalCase();
	}

	std::string
	TokenFormatter::toSnakeCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toSnakeCase();
	}

	std::string
	TokenFormatter::toScreamingSnake (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toScreamingSnake();
	}

	std::string
	TokenFormatter::toKebabCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toKebabCase();
	}

	std::string
	TokenFormatter::toTrainCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toTrainCase();
	}

	std::string
	TokenFormatter::toFlatCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toFlatCase();
	}

	std::string
	TokenFormatter::toUpperFlatCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toUpperFlatCase();
	}

	std::string
	TokenFormatter::toLowerSpaced (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toLowerSpaced();
	}

	std::string
	TokenFormatter::toUpperSpaced (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toUpperSpaced();
	}

	std::string
	TokenFormatter::toTitleCase (std::string_view source) noexcept
	{
		return TokenFormatter{source}.toTitleCase();
	}

	std::string
	TokenFormatter::convert (std::string_view source, CaseStyle targetStyle) noexcept
	{
		return TokenFormatter{source}.to(targetStyle);
	}

	std::string_view
	TokenFormatter::styleName (CaseStyle style) noexcept
	{
		switch ( style )
		{
			case CaseStyle::CamelCase:
				return "camelCase";

			case CaseStyle::PascalCase:
				return "PascalCase";

			case CaseStyle::SnakeCase:
				return "snake_case";

			case CaseStyle::ScreamingSnake:
				return "SCREAMING_SNAKE_CASE";

			case CaseStyle::KebabCase:
				return "kebab-case";

			case CaseStyle::TrainCase:
				return "TRAIN-CASE";

			case CaseStyle::FlatCase:
				return "flatcase";

			case CaseStyle::UpperFlatCase:
				return "UPPERFLATCASE";

			case CaseStyle::LowerSpaced:
				return "lower spaced";

			case CaseStyle::UpperSpaced:
				return "UPPER SPACED";

			case CaseStyle::TitleCase:
				return "Title Case";

			case CaseStyle::Unknown:
			default:
				return "Unknown";
		}
	}

	/* Private methods implementation. */

	void
	TokenFormatter::parse () noexcept
	{
		if ( m_bufferLength == 0 )
		{
			return;
		}

		const char * bufferStart = m_buffer.data();
		const char * wordStart = nullptr;
		size_t wordLength = 0;

		for ( size_t i = 0; i < m_bufferLength; ++i )
		{
			const char chr = m_buffer[i];

			/* Skip separators and finalize current word. */
			if ( isSeparator(chr) )
			{
				if ( wordStart != nullptr && wordLength > 0 )
				{
					if ( m_wordCount < MaxWords )
					{
						m_words[m_wordCount++] = std::string_view{wordStart, wordLength};
					}

					wordStart = nullptr;
					wordLength = 0;
				}

				continue;
			}

			/* Handle case transitions for camelCase and PascalCase. */
			if ( std::isupper(static_cast< unsigned char >(chr)) != 0 )
			{
				if ( wordStart != nullptr && wordLength > 0 )
				{
					const char lastChar = *(wordStart + wordLength - 1);

					if ( std::islower(static_cast< unsigned char >(lastChar)) != 0 )
					{
						/* Lowercase to uppercase transition: finalize current word. */
						if ( m_wordCount < MaxWords )
						{
							m_words[m_wordCount++] = std::string_view{wordStart, wordLength};
						}

						wordStart = bufferStart + i;
						wordLength = 1;

						continue;
					}

					if ( std::isupper(static_cast< unsigned char >(lastChar)) != 0 )
					{
						/* Check if we're at the start of a new word in an acronym sequence. */
						/* e.g., "XMLParser" -> "XML" + "Parser" */
						if ( i + 1 < m_bufferLength &&
							std::islower(static_cast< unsigned char >(m_buffer[i + 1])) != 0 )
						{
							if ( m_wordCount < MaxWords )
							{
								m_words[m_wordCount++] = std::string_view{wordStart, wordLength};
							}

							wordStart = bufferStart + i;
							wordLength = 1;

							continue;
						}
					}
				}
			}

			/* Start a new word if not already in one. */
			if ( wordStart == nullptr )
			{
				wordStart = bufferStart + i;
				wordLength = 1;
			}
			else
			{
				++wordLength;
			}
		}

		/* Add the last word if not empty. */
		if ( wordStart != nullptr && wordLength > 0 && m_wordCount < MaxWords )
		{
			m_words[m_wordCount++] = std::string_view{wordStart, wordLength};
		}
	}

	CaseStyle
	TokenFormatter::detectStyle (std::string_view token) noexcept
	{
		if ( token.empty() )
		{
			return CaseStyle::Unknown;
		}

		bool hasUnderscore = false;
		bool hasHyphen = false;
		bool hasSpace = false;
		bool hasUppercase = false;
		bool hasLowercase = false;
		bool startsWithUpper = std::isupper(static_cast< unsigned char >(token[0])) != 0;

		for ( const char chr : token )
		{
			if ( chr == '_' )
			{
				hasUnderscore = true;
			}
			else if ( chr == '-' )
			{
				hasHyphen = true;
			}
			else if ( chr == ' ' )
			{
				hasSpace = true;
			}
			else if ( std::isupper(static_cast< unsigned char >(chr)) != 0 )
			{
				hasUppercase = true;
			}
			else if ( std::islower(static_cast< unsigned char >(chr)) != 0 )
			{
				hasLowercase = true;
			}
		}

		/* Check for separator-based styles first. */
		if ( hasUnderscore )
		{
			if ( hasUppercase && !hasLowercase )
			{
				return CaseStyle::ScreamingSnake;
			}

			return CaseStyle::SnakeCase;
		}

		if ( hasHyphen )
		{
			if ( hasUppercase && !hasLowercase )
			{
				return CaseStyle::TrainCase;
			}

			return CaseStyle::KebabCase;
		}

		if ( hasSpace )
		{
			if ( hasUppercase && !hasLowercase )
			{
				return CaseStyle::UpperSpaced;
			}

			if ( hasLowercase && !hasUppercase )
			{
				return CaseStyle::LowerSpaced;
			}

			/* Mixed case with spaces - likely Title Case. */
			return CaseStyle::TitleCase;
		}

		/* No separators - check for case-based styles. */
		if ( hasUppercase && hasLowercase )
		{
			if ( startsWithUpper )
			{
				return CaseStyle::PascalCase;
			}

			return CaseStyle::CamelCase;
		}

		if ( hasUppercase && !hasLowercase )
		{
			return CaseStyle::UpperFlatCase;
		}

		if ( hasLowercase && !hasUppercase )
		{
			return CaseStyle::FlatCase;
		}

		return CaseStyle::Unknown;
	}

	bool
	TokenFormatter::isSeparator (char chr) noexcept
	{
		return chr == '_' || chr == '-' || chr == ' ';
	}

	void
	TokenFormatter::appendCapitalized (std::string & result, std::string_view word) noexcept
	{
		if ( word.empty() )
		{
			return;
		}

		result += static_cast< char >(std::toupper(static_cast< unsigned char >(word[0])));

		for ( size_t i = 1; i < word.size(); ++i )
		{
			result += static_cast< char >(std::tolower(static_cast< unsigned char >(word[i])));
		}
	}

	void
	TokenFormatter::appendLower (std::string & result, std::string_view word) noexcept
	{
		for ( const char chr : word )
		{
			result += static_cast< char >(std::tolower(static_cast< unsigned char >(chr)));
		}
	}

	void
	TokenFormatter::appendUpper (std::string & result, std::string_view word) noexcept
	{
		for ( const char chr : word )
		{
			result += static_cast< char >(std::toupper(static_cast< unsigned char >(chr)));
		}
	}
}
