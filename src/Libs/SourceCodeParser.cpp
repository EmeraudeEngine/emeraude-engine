/*
 * src/Libs/SourceCodeParser.cpp
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

#include "SourceCodeParser.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <sstream>

/* Conditional std::format support (not available on macOS SDK < 13.3). */
#if !IS_MACOS
#include <format>
#endif

namespace EmEn::Libs
{
	void
	SourceCodeParser::annotate (size_t line, size_t column, std::string_view notice) noexcept
	{
		if ( line < 1 )
		{
			this->annotate(notice);
		}
		else
		{
			m_annotations.emplace(line, std::multimap< size_t, std::string >{}).first->second.emplace(column, std::string{notice});
		}
	}

	std::string
	SourceCodeParser::getParsedSourceCode () const noexcept
	{
		std::ostringstream outputSource{};

		/* Build a mapping from original line numbers to output line numbers when removing comments.
		 * This ensures annotations point to the correct lines in the output. */
		std::map< size_t, size_t > lineMapping; /* original line -> output line */
		std::vector< std::pair< size_t, std::string > > outputLines; /* (original line number, content) */

		bool insideBlockComment = false;

		for ( size_t i = 0; i < m_lines.size(); ++i )
		{
			const auto originalLineNumber = i + 1;
			const auto & line = m_lines[i];

			if ( m_removeComments )
			{
				std::string processedLine;
				bool keepLine = true;
				size_t pos = 0;

				while ( pos < line.size() )
				{
					if ( insideBlockComment )
					{
						/* Look for end of block comment. */
						if ( const auto endPos = line.find("*/", pos); endPos != std::string::npos )
						{
							insideBlockComment = false;
							pos = endPos + 2;
						}
						else
						{
							/* Still inside block comment, skip rest of line. */
							break;
						}
					}
					else
					{
						/* Check for line comment. */
						const auto lineCommentPos = line.find("//", pos);
						/* Check for block comment start. */
						const auto blockCommentPos = line.find("/*", pos);

						if ( lineCommentPos != std::string::npos && (blockCommentPos == std::string::npos || lineCommentPos < blockCommentPos) )
						{
							/* Line comment found first - take everything before it and stop. */
							processedLine += line.substr(pos, lineCommentPos - pos);
							break;
						}

						if ( blockCommentPos != std::string::npos )
						{
							/* Block comment found - take everything before it. */
							processedLine += line.substr(pos, blockCommentPos - pos);
							pos = blockCommentPos + 2;
							insideBlockComment = true;

							/* Check if block comment ends on same line. */
							if ( const auto endPos = line.find("*/", pos); endPos != std::string::npos )
							{
								insideBlockComment = false;
								pos = endPos + 2;
							}
						}
						else
						{
							/* No comments found - take the rest of the line. */
							processedLine += line.substr(pos);

							break;
						}
					}
				}

				/* Trim trailing whitespace. */
				while ( !processedLine.empty() && (processedLine.back() == ' ' || processedLine.back() == '\t') )
				{
					processedLine.pop_back();
				}

				/* Skip empty lines resulting from comment removal. */
				if ( processedLine.empty() && !insideBlockComment )
				{
					/* Check if line was only whitespace/comments. */
					bool wasOnlyWhitespace = true;

					for ( char c : line )
					{
						if ( c != ' ' && c != '\t' )
						{
							wasOnlyWhitespace = false;
							break;
						}
					}

					if ( !wasOnlyWhitespace )
					{
						keepLine = false;
					}
				}

				if ( keepLine || !processedLine.empty() )
				{
					lineMapping[originalLineNumber] = outputLines.size() + 1;
					outputLines.emplace_back(originalLineNumber, processedLine);
				}
			}
			else
			{
				/* No comment removal - direct mapping. */
				lineMapping[originalLineNumber] = outputLines.size() + 1;
				outputLines.emplace_back(originalLineNumber, line);
			}
		}

		/* Output the processed lines. */
		for ( size_t i = 0; i < outputLines.size(); ++i )
		{
			const auto outputLineNumber = i + 1;
			const auto originalLineNumber = outputLines[i].first;
			const auto & line = outputLines[i].second;

			/* Print the line of code with formatting. */
			if ( m_showLineNumbers > 0 )
			{
#if IS_MACOS
				/* Fallback for macOS < 13.3: use ostringstream formatting. */
				outputSource.width(m_showLineNumbers);
				outputSource.fill(' ');
				outputSource << outputLineNumber << "| " << line << '\n';
#else
				/* Use std::format on Linux/Windows. */
				outputSource << std::format("{:>{}}| {}\n", outputLineNumber, m_showLineNumbers, line);
#endif
			}
			else
			{
				outputSource << line << '\n';
			}

			/* Print possible notices under the line (using original line number for lookup). */
			if ( const auto noticeIt = m_annotations.find(originalLineNumber); noticeIt != m_annotations.cend() )
			{
				for ( const auto & [column, annotation] : noticeIt->second )
				{
					if ( m_showLineNumbers )
					{
						outputSource << std::string(static_cast< size_t >(m_showLineNumbers) + 2 + column, '~') << "^ " << annotation << '\n';
					}
					else
					{
						outputSource << std::string(column, '~') << "^ " << annotation << '\n';
					}
				}

				outputSource << '\n';
			}
		}

		/* Print possible end notices. */
		if ( !m_footAnnotations.empty() )
		{
			outputSource << '\n';

			for ( const auto & endNotice : m_footAnnotations )
			{
				outputSource << endNotice << '\n';
			}
		}

		return outputSource.str();
	}
}
