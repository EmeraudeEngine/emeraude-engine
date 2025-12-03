/*
 * src/Libs/KVParser.hpp
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
#include <filesystem>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs
{
	/**
	 * @class KVVariable
	 * @brief Represents a single key-value variable with type conversion capabilities.
	 *
	 * KVVariable is a flexible container that stores a value as a string internally
	 * but provides convenient conversion methods to various primitive types (bool, int,
	 * float, double). Variables can be in an undefined state, which is useful for
	 * distinguishing between missing and explicitly set values in configuration files.
	 *
	 * Boolean conversion recognizes multiple string representations: "1", "true", "True",
	 * "TRUE", "on", "On", "ON" all evaluate to true; any other value evaluates to false.
	 *
	 * @note Numeric conversions use the String::toNumber utility and will return 0 for
	 * invalid string representations.
	 *
	 * @see KVSection, KVParser
	 * @version 0.8.38
	 */
	class KVVariable final
	{
		public:

			/**
			 * @brief Constructs an undefined KVVariable.
			 *
			 * Creates a variable in an undefined state with an empty internal value.
			 * Use isUndefined() to check if a variable was explicitly set.
			 */
			KVVariable () noexcept = default;

			/**
			 * @brief Constructs a KVVariable from a string value.
			 *
			 * @param value The string value to store.
			 */
			explicit
			KVVariable (std::string value) noexcept
				: m_value(std::move(value)),
				m_undefined(false)
			{

			}

			/**
			 * @brief Constructs a KVVariable from a boolean value.
			 *
			 * Stores the boolean as "1" (true) or "0" (false).
			 *
			 * @param value The boolean value to store.
			 */
			explicit
			KVVariable (bool value) noexcept
				: m_value(value ? "1" : "0"),
				m_undefined(false)
			{

			}

			/**
			 * @brief Constructs a KVVariable from an integer value.
			 *
			 * @param value The integer value to store as a string.
			 */
			explicit
			KVVariable (int value) noexcept
				: m_value(std::to_string(value)),
				m_undefined(false)
			{

			}

			/**
			 * @brief Constructs a KVVariable from a float value.
			 *
			 * @param value The float value to store as a string.
			 */
			explicit
			KVVariable (float value) noexcept
				: m_value(std::to_string(value)),
				m_undefined(false)
			{

			}

			/**
			 * @brief Constructs a KVVariable from a double value.
			 *
			 * @param value The double value to store as a string.
			 */
			explicit
			KVVariable (double value) noexcept
				: m_value(std::to_string(value)),
				m_undefined(false)
			{

			}

			/**
			 * @brief Checks if the variable is in an undefined state.
			 *
			 * @return true if the variable was default-constructed and never assigned a value,
			 *         false otherwise.
			 */
			[[nodiscard]]
			bool
			isUndefined () const noexcept
			{
				return m_undefined;
			}

			/**
			 * @brief Converts the variable's value to a boolean.
			 *
			 * Recognizes the following case-sensitive true values: "1", "true", "True",
			 * "TRUE", "on", "On", "ON". All other values (including empty string) return false.
			 *
			 * @return true if the value matches any recognized true representation, false otherwise.
			 */
			[[nodiscard]]
			bool
			asBoolean () const noexcept
			{
				return m_value == "1" ||
					m_value == "true" ||
					m_value == "True" ||
					m_value == "TRUE" ||
					m_value == "on" ||
					m_value == "On" ||
					m_value == "ON";
			}

			/**
			 * @brief Converts the variable's value to an integer.
			 *
			 * Uses String::toNumber for conversion. Invalid or non-numeric strings return 0.
			 *
			 * @return The integer representation of the stored value.
			 */
			[[nodiscard]]
			int
			asInteger () const noexcept
			{
				return String::toNumber< int >(m_value);
			}

			/**
			 * @brief Converts the variable's value to a float.
			 *
			 * Uses String::toNumber for conversion. Invalid or non-numeric strings return 0.0f.
			 *
			 * @return The float representation of the stored value.
			 */
			[[nodiscard]]
			float
			asFloat () const noexcept
			{
				return String::toNumber< float >(m_value);
			}

			/**
			 * @brief Converts the variable's value to a double.
			 *
			 * Uses String::toNumber for conversion. Invalid or non-numeric strings return 0.0.
			 *
			 * @return The double representation of the stored value.
			 */
			[[nodiscard]]
			double
			asDouble () const noexcept
			{
				return String::toNumber< double >(m_value);
			}

			/**
			 * @brief Returns the raw string value.
			 *
			 * @return Const reference to the internal string value.
			 */
			[[nodiscard]]
			const std::string &
			asString () const noexcept
			{
				return m_value;
			}

		private:

			std::string m_value;
			bool m_undefined{true};
	};

	/**
	 * @class KVSection
	 * @brief Represents a section in a key-value configuration file containing multiple variables.
	 *
	 * KVSection manages a collection of KVVariable objects indexed by string keys.
	 * In INI-style configuration files, sections appear as [SectionName] headers followed
	 * by key=value pairs. Each section maintains its own namespace of variable names.
	 *
	 * Variables are stored in a std::map, providing ordered iteration and efficient lookup.
	 * Attempting to retrieve a non-existent variable returns an undefined KVVariable rather
	 * than throwing an exception.
	 *
	 * @see KVVariable, KVParser
	 * @version 0.8.38
	 */
	class KVSection final
	{
		public:

			/**
			 * @brief Constructs an empty KVSection.
			 */
			KVSection () noexcept = default;

			/**
			 * @brief Adds or updates a variable in the section.
			 *
			 * If a variable with the same key already exists, it will be replaced.
			 *
			 * @param key The variable name/key.
			 * @param variable The KVVariable to store.
			 */
			void
			addVariable (std::string_view key, const KVVariable & variable) noexcept
			{
				m_variables[std::string{key}] = variable;
			}

			/**
			 * @brief Returns all variables in the section.
			 *
			 * Provides read-only access to the internal map of all variables.
			 *
			 * @return Const reference to the map of variable names to KVVariable objects.
			 */
			[[nodiscard]]
			const std::map< std::string, KVVariable > &
			variables () const noexcept
			{
				return m_variables;
			}

			/**
			 * @brief Retrieves a specific variable by key.
			 *
			 * Returns an undefined KVVariable if the key does not exist, rather than throwing
			 * an exception. Use KVVariable::isUndefined() to check if the variable was found.
			 *
			 * @param key The variable name to look up.
			 * @return The KVVariable associated with the key, or an undefined KVVariable if not found.
			 */
			[[nodiscard]]
			KVVariable
			variable (std::string_view key) const noexcept
			{
				if ( const auto variableIt = m_variables.find(std::string{key}); variableIt != m_variables.cend() )
				{
					return variableIt->second;
				}

				return {};
			}

			/**
			 * @brief Writes the section's variables to an output file stream.
			 *
			 * Outputs all variables in the format "key = value\n". Section headers are not
			 * written by this method; that responsibility belongs to KVParser::write().
			 *
			 * @param file The output file stream to write to.
			 */
			void write (std::ofstream & file) const noexcept;

		private:

			std::map< std::string, KVVariable > m_variables;
	};

	/**
	 * @class KVParser
	 * @brief Parses and manages INI-style key-value configuration files organized by sections.
	 *
	 * KVParser provides functionality to read and write configuration files in a simple
	 * INI-like format. Files are organized into sections (denoted by [SectionName]) with
	 * key=value pairs under each section. The parser automatically creates a default "main"
	 * section for any key-value pairs that appear before the first section header.
	 *
	 * Supported file format:
	 * - Section headers: [SectionName]
	 * - Variable definitions: key = value
	 * - Comments: Lines starting with '#'
	 * - Headers: Lines starting with '@' (ignored during parsing)
	 * - Blank lines and whitespace are handled gracefully
	 *
	 * The parser uses String::trim() to remove leading and trailing whitespace from keys
	 * and values, ensuring clean data storage.
	 *
	 * @code
	 * KVParser parser;
	 * if (parser.read("config.ini")) {
	 *     auto& section = parser.section("Graphics");
	 *     int width = section.variable("width").asInteger();
	 *     bool fullscreen = section.variable("fullscreen").asBoolean();
	 * }
	 * @endcode
	 *
	 * @note All methods are noexcept and use return values to indicate success/failure
	 * rather than throwing exceptions.
	 *
	 * @see KVSection, KVVariable
	 * @version 0.8.38
	 */
	class KVParser final
	{
		public:

			/**
			 * @brief Constructs an empty KVParser with no sections.
			 */
			KVParser () noexcept = default;

			/**
			 * @brief Returns a mutable reference to all sections.
			 *
			 * Provides direct access to the internal section map for iteration or modification.
			 *
			 * @return Reference to the map of section names to KVSection objects.
			 */
			[[nodiscard]]
			std::map< std::string, KVSection > &
			sections () noexcept
			{
				return m_sections;
			}

			/**
			 * @brief Returns a read-only reference to all sections.
			 *
			 * Provides const access to the internal section map for read-only iteration.
			 *
			 * @return Const reference to the map of section names to KVSection objects.
			 */
			[[nodiscard]]
			const std::map< std::string, KVSection > &
			sections () const noexcept
			{
				return m_sections;
			}

			/**
			 * @brief Retrieves or creates a section by name.
			 *
			 * If a section with the given name exists, returns a reference to it. Otherwise,
			 * creates a new empty section with that name and returns a reference to it.
			 * This method never fails and always returns a valid section reference.
			 *
			 * @param label The name of the section to retrieve or create.
			 * @return Reference to the requested KVSection.
			 */
			[[nodiscard]]
			KVSection & section (std::string_view label) noexcept;

			/**
			 * @brief Reads and parses a key-value configuration file.
			 *
			 * Parses the file at the specified path, populating the parser's sections and
			 * variables. The file is expected to be in INI-style format with [Section] headers
			 * and key=value pairs. Variables defined before the first section header are placed
			 * in a default "main" section.
			 *
			 * Supported line types:
			 * - [SectionName]: Section header
			 * - key = value: Variable definition
			 * - #comment: Comment line (ignored)
			 * - @header: Header line (ignored)
			 * - Blank lines: Ignored
			 *
			 * @param filepath Path to the configuration file to read.
			 * @return true if the file was successfully opened and parsed, false if the file
			 *         could not be opened.
			 *
			 * @note Parsing errors in individual lines are silently ignored; only file I/O
			 *       errors cause this method to return false.
			 */
			[[nodiscard]]
			bool read (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Writes all sections and variables to a configuration file.
			 *
			 * Creates or overwrites the file at the specified path with the current parser
			 * contents. Sections are written with [SectionName] headers followed by their
			 * key=value pairs, with a blank line separating each section.
			 *
			 * Output format:
			 * @code
			 * [SectionName]
			 * key1 = value1
			 * key2 = value2
			 *
			 * [AnotherSection]
			 * key3 = value3
			 * @endcode
			 *
			 * @param filepath Path to the configuration file to write.
			 * @return true if the file was successfully opened and written, false if the file
			 *         could not be created or opened for writing.
			 */
			[[nodiscard]]
			bool write (const std::filesystem::path & filepath) const noexcept;

		private:

			/**
			 * @enum LineType
			 * @brief Categorizes different line types encountered during file parsing.
			 * @version 0.8.38
			 */
			enum class LineType
			{
				None = 0,         ///< Empty or unrecognized line
				Headers = 1,      ///< Header line starting with '@'
				Comment = 2,      ///< Comment line starting with '#'
				SectionTitle = 3, ///< Section header line containing '['
				Definition = 4    ///< Variable definition line containing '='
			};

			/**
			 * @brief Extracts the section name from a section header line.
			 *
			 * Parses a line of the form "[SectionName]" and returns the text between the
			 * brackets. Returns an empty string if the line is not a valid section header.
			 *
			 * @param line The line to parse.
			 * @return The section name without brackets, or an empty string if invalid.
			 */
			static std::string parseSectionTitle (std::string_view line) noexcept;

			/**
			 * @brief Determines the type of a line based on its first significant character.
			 *
			 * Scans the line for the first occurrence of a special character ('@', '[', '#', '=')
			 * to determine the line's purpose. Used during parsing to route lines to appropriate
			 * handlers.
			 *
			 * @param line The line to analyze.
			 * @return The LineType classification of the line.
			 */
			static LineType getLineType (std::string_view line) noexcept;

			std::map< std::string, KVSection > m_sections;
	};
}
