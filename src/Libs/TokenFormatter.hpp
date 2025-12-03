/*
 * src/Libs/TokenFormatter.hpp
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
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace EmEn::Libs
{
	/**
	 * @enum CaseStyle
	 * @brief Defines the supported string case styles for token formatting.
	 *
	 * This enumeration represents various naming conventions used in programming
	 * and text formatting, including separator-based styles (underscores, hyphens,
	 * spaces) and case-based styles (camel, pascal, flat).
	 *
	 * @version 0.8.38
	 */
	enum class CaseStyle : uint8_t
	{
		/** @brief Unable to determine the case style. */
		Unknown,
		/** @brief "camelCase": first word lowercase, subsequent words capitalized. */
		CamelCase,
		/** @brief "PascalCase": all words capitalized. */
		PascalCase,
		/** @brief "snake_case": words separated by underscores, all lowercase. */
		SnakeCase,
		/** @brief "SCREAMING_SNAKE_CASE": words separated by underscores, all uppercase. */
		ScreamingSnake,
		/** @brief "kebab-case": words separated by hyphens, all lowercase. */
		KebabCase,
		/** @brief "TRAIN-CASE": words separated by hyphens, all uppercase. */
		TrainCase,
		/** @brief "flatcase": all lowercase, no separators. */
		FlatCase,
		/** @brief "UPPERFLATCASE": all uppercase, no separators. */
		UpperFlatCase,
		/** @brief "lower spaced": words separated by spaces, all lowercase. */
		LowerSpaced,
		/** @brief "UPPER SPACED": words separated by spaces, all uppercase. */
		UpperSpaced,
		/** @brief "Title Case": words separated by spaces, each word capitalized. */
		TitleCase
	};

	/**
	 * @class TokenFormatter
	 * @brief Detects and converts between different string case styles.
	 *
	 * TokenFormatter provides utilities for parsing tokens written in various case styles
	 * (camelCase, snake_case, kebab-case, etc.) and converting them to other formats.
	 * It intelligently parses input tokens into individual words, detecting word boundaries
	 * based on separators (underscores, hyphens, spaces) or case transitions (camelCase).
	 *
	 * The class handles complex cases such as acronyms in PascalCase (e.g., "XMLParser"
	 * is correctly parsed as "XML" + "Parser") and supports up to 16 words with a maximum
	 * total token length of 128 characters.
	 *
	 * Both instance-based and static methods are provided for convenience. Instance methods
	 * allow parsing once and converting to multiple formats, while static methods provide
	 * one-shot conversions.
	 *
	 * @code
	 * // Instance-based usage
	 * TokenFormatter formatter("myVariableName");
	 * std::string snake = formatter.toSnakeCase();  // "my_variable_name"
	 * std::string kebab = formatter.toKebabCase();  // "my-variable-name"
	 *
	 * // Static usage
	 * std::string pascal = TokenFormatter::toPascalCase("user_id");  // "UserId"
	 * CaseStyle style = TokenFormatter::detect("API_KEY");  // CaseStyle::ScreamingSnake
	 * @endcode
	 *
	 * @note Tokens longer than MaxTokenLength (128) are truncated.
	 * @note The class supports up to MaxWords (16) words per token.
	 * @see CaseStyle
	 * @version 0.8.38
	 */
	class TokenFormatter final
	{
		public:

			/** @brief Maximum number of words that can be parsed from a token. */
			static constexpr size_t MaxWords{16};

			/** @brief Maximum length of a token in characters (tokens are truncated beyond this). */
			static constexpr size_t MaxTokenLength{128};

			/** @brief Storage type for parsed words as string views. */
			using WordStorage = std::array< std::string_view, MaxWords >;

			/**
			 * @brief Constructs a TokenFormatter and parses the input token into words.
			 *
			 * The constructor automatically detects the case style and parses the token
			 * into individual words by analyzing separators and case transitions. Tokens
			 * longer than MaxTokenLength are truncated.
			 *
			 * @param token The input string to parse and format.
			 */
			explicit TokenFormatter (std::string_view token) noexcept;

			/**
			 * @brief Returns the automatically detected case style of the input token.
			 *
			 * @return The detected CaseStyle, or CaseStyle::Unknown if style cannot be determined.
			 */
			[[nodiscard]]
			CaseStyle detectedStyle () const noexcept;

			/**
			 * @brief Returns the array of parsed words.
			 *
			 * @return Reference to the internal WordStorage array containing string views of each word.
			 * @note Only the first wordCount() elements contain valid data.
			 */
			[[nodiscard]]
			const WordStorage & words () const noexcept;

			/**
			 * @brief Returns the number of words parsed from the token.
			 *
			 * @return Number of valid words in the words() array (0 to MaxWords).
			 */
			[[nodiscard]]
			size_t wordCount () const noexcept;

			/**
			 * @brief Checks if the token contains no words.
			 *
			 * @return True if no words were parsed, false otherwise.
			 */
			[[nodiscard]]
			bool empty () const noexcept;

			/**
			 * @brief Calculates the total character length of all parsed words.
			 *
			 * This returns the sum of lengths of all words without separators.
			 *
			 * @return Total length of all words combined.
			 */
			[[nodiscard]]
			size_t totalWordLength () const noexcept;

			/**
			 * @brief Converts the parsed words to camelCase format.
			 *
			 * @return String in camelCase (e.g., "myVariableName").
			 */
			[[nodiscard]]
			std::string toCamelCase () const noexcept;

			/**
			 * @brief Converts the parsed words to PascalCase format.
			 *
			 * @return String in PascalCase (e.g., "MyVariableName").
			 */
			[[nodiscard]]
			std::string toPascalCase () const noexcept;

			/**
			 * @brief Converts the parsed words to snake_case format.
			 *
			 * @return String in snake_case (e.g., "my_variable_name").
			 */
			[[nodiscard]]
			std::string toSnakeCase () const noexcept;

			/**
			 * @brief Converts the parsed words to SCREAMING_SNAKE_CASE format.
			 *
			 * @return String in SCREAMING_SNAKE_CASE (e.g., "MY_VARIABLE_NAME").
			 */
			[[nodiscard]]
			std::string toScreamingSnake () const noexcept;

			/**
			 * @brief Converts the parsed words to kebab-case format.
			 *
			 * @return String in kebab-case (e.g., "my-variable-name").
			 */
			[[nodiscard]]
			std::string toKebabCase () const noexcept;

			/**
			 * @brief Converts the parsed words to TRAIN-CASE format.
			 *
			 * @return String in TRAIN-CASE (e.g., "MY-VARIABLE-NAME").
			 */
			[[nodiscard]]
			std::string toTrainCase () const noexcept;

			/**
			 * @brief Converts the parsed words to flatcase format.
			 *
			 * @return String in flatcase (e.g., "myvariablename").
			 */
			[[nodiscard]]
			std::string toFlatCase () const noexcept;

			/**
			 * @brief Converts the parsed words to UPPERFLATCASE format.
			 *
			 * @return String in UPPERFLATCASE (e.g., "MYVARIABLENAME").
			 */
			[[nodiscard]]
			std::string toUpperFlatCase () const noexcept;

			/**
			 * @brief Converts the parsed words to lower spaced format.
			 *
			 * @return String in lower spaced format (e.g., "my variable name").
			 */
			[[nodiscard]]
			std::string toLowerSpaced () const noexcept;

			/**
			 * @brief Converts the parsed words to UPPER SPACED format.
			 *
			 * @return String in UPPER SPACED format (e.g., "MY VARIABLE NAME").
			 */
			[[nodiscard]]
			std::string toUpperSpaced () const noexcept;

			/**
			 * @brief Converts the parsed words to Title Case format.
			 *
			 * @return String in Title Case (e.g., "My Variable Name").
			 */
			[[nodiscard]]
			std::string toTitleCase () const noexcept;

			/**
			 * @brief Converts the parsed words to the specified case style.
			 *
			 * This method provides a generic conversion interface using the CaseStyle enum.
			 * If CaseStyle::Unknown is provided, returns concatenated words without formatting.
			 *
			 * @param targetStyle The desired output case style.
			 * @return String converted to the target case style.
			 */
			[[nodiscard]]
			std::string to (CaseStyle targetStyle) const noexcept;

			/**
			 * @brief Detects the case style of the given source string.
			 *
			 * Analyzes the input string to determine its case style by examining
			 * separators (underscores, hyphens, spaces) and character casing patterns.
			 *
			 * @param source The string to analyze.
			 * @return The detected CaseStyle, or CaseStyle::Unknown if undeterminable.
			 */
			[[nodiscard]]
			static CaseStyle detect (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to camelCase format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in camelCase (e.g., "myVariableName").
			 */
			[[nodiscard]]
			static std::string toCamelCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to PascalCase format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in PascalCase (e.g., "MyVariableName").
			 */
			[[nodiscard]]
			static std::string toPascalCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to snake_case format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in snake_case (e.g., "my_variable_name").
			 */
			[[nodiscard]]
			static std::string toSnakeCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to SCREAMING_SNAKE_CASE format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in SCREAMING_SNAKE_CASE (e.g., "MY_VARIABLE_NAME").
			 */
			[[nodiscard]]
			static std::string toScreamingSnake (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to kebab-case format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in kebab-case (e.g., "my-variable-name").
			 */
			[[nodiscard]]
			static std::string toKebabCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to TRAIN-CASE format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in TRAIN-CASE (e.g., "MY-VARIABLE-NAME").
			 */
			[[nodiscard]]
			static std::string toTrainCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to flatcase format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in flatcase (e.g., "myvariablename").
			 */
			[[nodiscard]]
			static std::string toFlatCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to UPPERFLATCASE format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in UPPERFLATCASE (e.g., "MYVARIABLENAME").
			 */
			[[nodiscard]]
			static std::string toUpperFlatCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to lower spaced format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in lower spaced format (e.g., "my variable name").
			 */
			[[nodiscard]]
			static std::string toLowerSpaced (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to UPPER SPACED format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in UPPER SPACED format (e.g., "MY VARIABLE NAME").
			 */
			[[nodiscard]]
			static std::string toUpperSpaced (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to Title Case format (static convenience method).
			 *
			 * @param source The input string to convert.
			 * @return String in Title Case (e.g., "My Variable Name").
			 */
			[[nodiscard]]
			static std::string toTitleCase (std::string_view source) noexcept;

			/**
			 * @brief Converts a string to the specified case style (static convenience method).
			 *
			 * This static method creates a temporary TokenFormatter instance and performs
			 * the conversion in one call.
			 *
			 * @param source The input string to convert.
			 * @param targetStyle The desired output case style.
			 * @return String converted to the target case style.
			 */
			[[nodiscard]]
			static std::string convert (std::string_view source, CaseStyle targetStyle) noexcept;

			/**
			 * @brief Returns the human-readable name of a case style.
			 *
			 * Provides a string representation of the CaseStyle enum value for display
			 * or debugging purposes.
			 *
			 * @param style The CaseStyle to get the name of.
			 * @return String view containing the style name (e.g., "camelCase", "snake_case").
			 */
			[[nodiscard]]
			static std::string_view styleName (CaseStyle style) noexcept;

		private:

			/**
			 * @brief Parses the internal buffer into individual words.
			 *
			 * Analyzes the buffered token to identify word boundaries based on separators
			 * and case transitions, populating the m_words array with string views.
			 * Handles complex cases like acronyms (e.g., "XMLParser" -> "XML" + "Parser").
			 */
			void parse () noexcept;

			/**
			 * @brief Detects the case style of a token string.
			 *
			 * Internal implementation that analyzes separators and casing patterns to
			 * determine the CaseStyle of the input.
			 *
			 * @param token The token string to analyze.
			 * @return The detected CaseStyle.
			 */
			[[nodiscard]]
			static CaseStyle detectStyle (std::string_view token) noexcept;

			/**
			 * @brief Checks if a character is a word separator.
			 *
			 * @param chr The character to check.
			 * @return True if chr is underscore, hyphen, or space; false otherwise.
			 */
			[[nodiscard]]
			static bool isSeparator (char chr) noexcept;

			/**
			 * @brief Appends a word to result with the first character capitalized.
			 *
			 * @param result The string to append to.
			 * @param word The word to append with capitalization.
			 */
			static void appendCapitalized (std::string & result, std::string_view word) noexcept;

			/**
			 * @brief Appends a word to result in lowercase.
			 *
			 * @param result The string to append to.
			 * @param word The word to append in lowercase.
			 */
			static void appendLower (std::string & result, std::string_view word) noexcept;

			/**
			 * @brief Appends a word to result in uppercase.
			 *
			 * @param result The string to append to.
			 * @param word The word to append in uppercase.
			 */
			static void appendUpper (std::string & result, std::string_view word) noexcept;

			std::array< char, MaxTokenLength > m_buffer{}; /**< Internal copy of the token for stable string_view references. */
			size_t m_bufferLength{0};
			WordStorage m_words{}; /**< Views into m_buffer for each parsed word. */
			size_t m_wordCount{0};
			CaseStyle m_detectedStyle{CaseStyle::Unknown};
	};
}
