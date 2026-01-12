/*
 * src/Libs/SourceCodeParser.hpp
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

#pragma once

/* STL inclusions. */
#include <map>
#include <string>
#include <string_view>
#include <vector>

/* Local inclusions. */
#include "String.hpp"

namespace EmEn::Libs
{
	/**
	 * @class SourceCodeParser
	 * @brief Parses and annotates source code with contextual notices and formatting.
	 *
	 * The SourceCodeParser class provides functionality to parse source code files and add
	 * annotations at specific line and column positions. It supports optional line numbering,
	 * comment removal, and footer annotations. This utility is particularly useful for
	 * displaying error messages, warnings, or other contextual information alongside source code.
	 *
	 * Key features:
	 * - Parse source code and maintain line-by-line structure
	 * - Add annotations at specific line/column positions with visual indicators
	 * - Optional line number display with configurable width
	 * - Optional comment removal (C/C++ style) while preserving line mapping for annotations
	 * - Footer annotations for additional context
	 *
	 * Usage example:
	 * @code
	 * std::string code = "int main() {\n	return 0;\n}\n";
	 * SourceCodeParser parser(code, 5, false);
	 * parser.annotate(2, 11, "Missing semicolon");
	 * std::string annotated = parser.getParsedSourceCode();
	 * @endcode
	 *
	 * @note This class is designed for read-only analysis and formatting. It does not modify
	 *	   the original source code content.
	 * @see String::explode()
	 * @since 0.8.38
	 * @version 0.8.38
	 */
	class SourceCodeParser final
	{
		public:

			/**
			 * @brief Constructs a source code parser with specified formatting options.
			 *
			 * Initializes the parser by splitting the source code into lines and configuring
			 * line number display and comment removal options.
			 *
			 * @param sourceCode The source code to parse as a string reference. The content is
			 *				   split into individual lines for processing.
			 * @param showLineNumbers Controls line number display: 0 disables line numbers,
			 *						any value > 0 enables them with the specified width (minimum 5).
			 *						Values between 1-4 are automatically adjusted to 5.
			 * @param removeComments When true, removes all C/C++ style comments.
			 *					   from the source code while preserving line mapping for annotations.
			 *					   Empty lines resulting from comment removal are also removed.
			 *
			 * @note Line numbers are right-aligned with the specified width, followed by "| ".
			 * @note Comment removal maintains accurate line/column mapping for annotations.
			 * @version 0.8.38
			 */
			explicit
			SourceCodeParser (const std::string & sourceCode, uint32_t showLineNumbers, bool removeComments) noexcept
				: m_lines{String::explode(sourceCode, '\n')},
				m_showLineNumbers{showLineNumbers ? std::max(5u, showLineNumbers) : 0},
				m_removeComments{removeComments}
			{

			}

			/**
			 * @brief Adds an annotation at a specific line and column position.
			 *
			 * Places an annotation marker (^) under the specified column position on the
			 * specified line, followed by the notice text. Multiple annotations can be added
			 * to the same line at different columns. If line is 0, the annotation is added
			 * as a footer annotation instead.
			 *
			 * @param line The line number (1-based) where the annotation should appear.
			 *			 If 0 or less, the annotation is treated as a footer annotation.
			 * @param column The column position (0-based) where the annotation marker should point.
			 * @param notice The annotation text to display.
			 *
			 * @note Annotations appear below their corresponding line with tilde characters (~)
			 *	   leading to a caret (^) pointing at the column position.
			 * @note When line numbers are enabled, the annotation indentation accounts for the
			 *	   line number display width.
			 * @see annotate(std::string_view)
			 * @version 0.8.38
			 */
			void annotate (size_t line, size_t column, std::string_view notice) noexcept;

			/**
			 * @brief Adds a footer annotation to be displayed after all source code lines.
			 *
			 * Footer annotations appear at the end of the formatted output, after all source
			 * code lines and their inline annotations. Multiple footer annotations can be added
			 * and will be displayed in the order they were added.
			 *
			 * @param notice The footer annotation text to display.
			 *
			 * @note Footer annotations are preceded by a blank line for visual separation.
			 * @note Useful for general notes, summaries, or context that applies to the entire code.
			 * @see annotate(size_t, size_t, std::string_view)
			 * @version 0.8.38
			 */
			void
			annotate (std::string_view notice) noexcept
			{
				m_footAnnotations.emplace_back(notice);
			}

			/**
			 * @brief Generates and returns the formatted source code with all annotations.
			 *
			 * Processes the source code according to the configured options (line numbering,
			 * comment removal) and applies all added annotations. The output includes:
			 * - Optional line numbers with configurable width
			 * - Source code lines (with comments removed if enabled)
			 * - Inline annotations with visual markers pointing to specific columns
			 * - Footer annotations at the end
			 *
			 * @return A formatted string containing the annotated source code.
			 *
			 * @note When comments are removed, line mapping is preserved so annotations
			 *	   still point to the correct positions.
			 * @note Annotations use tilde characters (~) followed by a caret (^) to point
			 *	   at specific columns.
			 * @note Multiple annotations on the same line are displayed in column order.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			std::string getParsedSourceCode () const noexcept;

			/**
			 * @brief Static convenience method to parse and format source code in one call.
			 *
			 * Creates a temporary SourceCodeParser instance and immediately returns the formatted
			 * output. This method is useful for simple formatting tasks that don't require adding
			 * annotations.
			 *
			 * @param sourceCode The source code to parse and format.
			 * @param showLineNumbers Controls line number display: 0 disables line numbers,
			 *						any value > 0 enables them with the specified width (minimum 5).
			 *						Default is 5.
			 * @param removeComments When true, removes all C/C++ style comments from the source code.
			 *					   Default is false.
			 * @return A formatted string with line numbers (if enabled) and processed content.
			 *
			 * @note This method cannot add annotations. Use the constructor and instance methods
			 *	   if you need to annotate the source code.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			static
			std::string
			parse (const std::string & sourceCode, uint32_t showLineNumbers = 5, bool removeComments = false) noexcept
			{
				return SourceCodeParser{sourceCode, showLineNumbers, removeComments}.getParsedSourceCode();
			}

		private:

			std::vector< std::string > m_lines;
			std::map< size_t, std::multimap< size_t, std::string > > m_annotations{};
			std::vector< std::string > m_footAnnotations;
			const uint32_t m_showLineNumbers;
			const bool m_removeComments;
	};
}
